/*
 * pyui.c - the desktop side of Python's tdui module: windows, widgets and
 * dialogs for a script, made and changed on the desktop's own loop.
 *
 *   script task (py)                     desktop loop (every 10 ms)
 *   td_pyui_send() --> [request queue] --> run_requests(): td_win_create...
 *   td_pyui_recv() <-- [event queue]   <-- widget callbacks, polled values
 *
 * Each queue has one writer and one reader, so plain C11 atomics are
 * enough. The queues and the object table are allocated when a script
 * first opens a window and freed when it ends: nothing is kept in RAM
 * while no script uses tdui. Objects are numbered by the script; the
 * table maps the numbers to the desktop's windows and widgets.
 *
 * Pixels stay on the desktop too: a script's images (decoded from files or
 * made from its bytes in C on the script's task, then handed over as a
 * pointer) and each canvas page's pixel layer, 2 x 2 pixels to a cell. A
 * request draws a whole image, a part of one (a sprite sheet) or a
 * nine-slice panel of any size, so a picture costs the script one call,
 * not one per pixel. The window turns a cell's four pixels into a quadrant
 * or half-block character when it draws (td_canvas_quad_cell()), and the
 * renderer sends only the cells that changed.
 */
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "td_apps.h"
#include "pyui_cache.h"
#include "tinydesk/td_canvas.h"
#include "tinydesk/td_picture.h"

#define REQ_SIZE         4096   /* powers of two */
#define EVT_SIZE         2048
#define MAX_OBJS         104    /* 4 windows, 48 widgets, images, dialogs and spare */
#define MAX_PICS         (TD_PYUI_MAX_CELLS > 2000 ? 512 : 48) /* all cached pages */
#define TICK_MS          10
#define TICK_BUDGET      200    /* requests run per tick, so the desktop stays smooth */
#define BATCH_MS         1000   /* absolute limit, not extended by nested groups */
#define STARTUP_BATCH_MS 5000 /* one bounded first-window construction */
#define LIST_ITEMS       1000
#define LIST_BYTES       16384

#define STR_(x)    #x
#define STR(x)     STR_(x)
#define CELLS_MSG  "canvases are limited to " STR(TD_PYUI_MAX_CELLS) " cells in all"
#define PIXELS_MSG "images and canvas pixels are limited to " STR(TD_PYUI_MAX_PIXELS) " bytes in all"
#define PX_CELL    0xFFFF   /* cell_t.ch of a cell that shows its pixels */

enum
{
    LINK_OFF,
    LINK_ON,
    LINK_CLOSING
};

enum
{
    K_FREE,
    K_WINDOW,
    K_WIDGET,
    K_CANVAS,
    K_DIALOG,
    K_MENU,
    K_IMAGE,
    K_FONT
};

typedef struct
{
    uint8_t *buf;
    uint32_t cap;
    atomic_uint head;   /* written by the producer */
    atomic_uint tail;   /* written by the consumer */
} ring_t;

typedef struct
{
    uint16_t ch;
    int16_t fg, bg;     /* -1: the theme's */
} cell_t;

typedef struct
{
    uint8_t kind;
    uint8_t wtype;      /* TD_PYUI_W_* for a widget */
    bool quiet;         /* window closed by the script: no close event */
    uint16_t id;
    uint16_t win;       /* the id of the window it is in */
    uint16_t held_canvas; /* window: route release back to the pressed canvas */
    td_window_t *w;     /* window, or dialog window */
    td_widget_t *wd;
    td_widget_t *bar;   /* a list's scrollbar */
    uint32_t seen;      /* text box: hash of the text last reported; list: selection */
    /* list items: '\0'-terminated strings in one block */
    char *items;
    int items_len, items_cap;
    uint16_t *offs;
    int nitems, offs_cap;
    /* canvas: its place in the window; window: the client size last reported */
    td_rect_t rect;
    /* canvas: `pages` pictures of rect.w x rect.h cells, one after the other;
     * requests draw into page `draw`, the window shows page `show` */
    cell_t *cells;
    uint8_t pages, draw, show;
    /* canvas: `pages` pixel layers of 2w x 2h after one another (NULL until
     * the first pixel is drawn) */
    uint8_t *px;
    uint16_t *rgb;
    uint32_t rgb_serial;
    long px_bytes;
    /* image: its pixels; a mask added later is a block of its own */
    td_pyui_image_t *img;
    td_pyui_font_t *font;
    uint8_t *own_mask;
} obj_t;

typedef struct
{
    unsigned refs;
    int x, y, w, h;      /* clipped canvas pixels before the fallback is drawn */
    size_t bytes;
    uint16_t *rgb;
    uint8_t px[];
} pic_bg_t;

typedef struct
{
    unsigned refs;
    size_t bytes;
    char text[];
} pic_text_t;

typedef struct
{
    bool used;
    bool nine;
    uint8_t page;
    uint16_t canvas, img, font;
    int16_t font_bg;
    uint8_t font_flags;
    int16_t x, y, w, h;   /* cells of the canvas */
    int16_t sx, sy;       /* the image's part */
    td_pyui_picture_t part;
    uint32_t serial;      /* new for every picture: td_picture_add()'s id */
    pic_bg_t *bg;         /* shared by copied pages; charged to the pixel budget */
    pic_text_t *text;
    td_pcache_ref_t cache;
    uint16_t cache_theme;
    bool warm_attempted;
} pic_t;

typedef struct
{
    ring_t req, evt;
    obj_t objs[MAX_OBJS];
    pic_t pics[MAX_PICS];
    uint32_t pic_serial;
    int cells;          /* canvas cells in use */
    bool closing;       /* close_all() is running: post nothing */
    unsigned batch_depth;
    uint32_t batch_start;
    uint32_t batch_limit;
    bool batch_expired;
    size_t cache_limit;
    unsigned warm_cursor;
    bool cache_warm;
    char text[TD_PYUI_TEXT_MAX + 1];
} link_t;

static atomic_int s_state = LINK_OFF;
static atomic_long s_pixels;   /* bytes of TD_PYUI_MAX_PIXELS in use */
static link_t *s_link;    /* set by the script before LINK_ON, freed after LINK_OFF */
static atomic_bool s_started;
static atomic_bool s_rgb;
static atomic_uint s_raster_ms, s_decode_ms;

void td_pyui_cache_stats(uint32_t out[12])
{
    td_pcache_info(out);
    out[6] = atomic_load(&s_raster_ms);
    out[7] = atomic_load(&s_decode_ms) + td_picture_runs_ms();
    td_picture_profile(out + 8);
}

bool td_pyui_enable_rgb(void)
{
    if (TD_PYUI_MAX_CELLS <= 2000)
        return false;
    if (!atomic_load(&s_rgb) && atomic_load(&s_pixels) > 0)
        return false;
    atomic_store(&s_rgb, true);
    return true;
}

bool td_pyui_rgb_enabled(void)
{
    return atomic_load(&s_rgb);
}

/* ------------------------------------------------------------ queues */

static uint32_t ring_used(ring_t *r)
{
    return atomic_load_explicit(&r->head, memory_order_acquire) - atomic_load_explicit(&r->tail, memory_order_acquire);
}

static void ring_copy_in(ring_t *r, uint32_t at, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    for (uint32_t i = 0; i < n; i++)
        r->buf[(at + i) & (r->cap - 1)] = s[i];
}

static void ring_copy_out(ring_t *r, uint32_t at, void *dst, uint32_t n)
{
    uint8_t *d = dst;
    for (uint32_t i = 0; i < n; i++)
        d[i] = r->buf[(at + i) & (r->cap - 1)];
}

static bool ring_put(ring_t *r, const td_pyui_msg_t *m, const char *text)
{
    uint32_t need = (uint32_t)sizeof(*m) + m->len;
    if (r->cap - ring_used(r) < need)
        return false;
    uint32_t head = atomic_load_explicit(&r->head, memory_order_relaxed);
    ring_copy_in(r, head, m, sizeof(*m));
    ring_copy_in(r, head + (uint32_t)sizeof(*m), text, m->len);
    atomic_store_explicit(&r->head, head + need, memory_order_release);
    return true;
}

/* One message; its text, cut to cap - 1 bytes, is '\0'-terminated. */
static bool ring_get(ring_t *r, td_pyui_msg_t *m, char *text, int cap)
{
    if (ring_used(r) < sizeof(*m))
        return false;
    uint32_t tail = atomic_load_explicit(&r->tail, memory_order_relaxed);
    ring_copy_out(r, tail, m, sizeof(*m));
    uint32_t len = m->len;
    uint32_t n = len < (uint32_t)(cap - 1) ? len : (uint32_t)(cap - 1);
    ring_copy_out(r, tail + (uint32_t)sizeof(*m), text, n);
    text[n] = '\0';
    m->len = (uint16_t)n;
    atomic_store_explicit(&r->tail, tail + (uint32_t)sizeof(*m) + len, memory_order_release);
    return true;
}

/* ------------------------------------------------------ script side */

bool td_pyui_attach(void)
{
    if (!atomic_load(&s_started))
        return false;
    if (atomic_load(&s_state) != LINK_OFF && !td_pyui_detach_done())
        return false;
    if (atomic_load(&s_state) == LINK_ON)
        return true;
    link_t *l = calloc(1, sizeof(*l));
    uint8_t *req = malloc(REQ_SIZE);
    uint8_t *evt = malloc(EVT_SIZE);
    if (!l || !req || !evt)
    {
        free(l);
        free(req);
        free(evt);
        return false;
    }
    l->req.buf = req;
    l->req.cap = REQ_SIZE;
    l->evt.buf = evt;
    l->evt.cap = EVT_SIZE;
    s_link = l;
    l->cache_limit = TD_PYUI_MAX_CELLS > 2000 ? 131072 : 0;
    l->cache_warm = true;
    atomic_store(&s_raster_ms, 0);
    atomic_store(&s_decode_ms, 0);
    atomic_store(&s_rgb, false);
    atomic_store_explicit(&s_state, LINK_ON, memory_order_release);
    return true;
}

bool td_pyui_send(const td_pyui_msg_t *m, const char *text)
{
    if (atomic_load(&s_state) != LINK_ON)
        return true;   /* nothing to send to: drop it */
    return ring_put(&s_link->req, m, text);
}

int td_pyui_recv(td_pyui_msg_t *m, char *text, int cap)
{
    if (atomic_load(&s_state) != LINK_ON)
        return 0;
    return ring_get(&s_link->evt, m, text, cap) ? 1 : 0;
}

void td_pyui_detach_begin(void)
{
    int on = LINK_ON;
    atomic_compare_exchange_strong(&s_state, &on, LINK_CLOSING);
}

bool td_pyui_detach_done(void)
{
    if (atomic_load_explicit(&s_state, memory_order_acquire) != LINK_OFF)
        return false;
    link_t *l = s_link;
    if (l)
    {
        s_link = NULL;
        free(l->req.buf);
        free(l->evt.buf);
        free(l);
    }
    return true;
}

/* ------------------------------------------------- pixels, both sides */

bool td_pyui_pixels_take(long bytes)
{
    long used = atomic_load(&s_pixels);
    do
    {
        if (bytes < 0 || used + bytes > TD_PYUI_MAX_PIXELS)
            return false;
    } while (!atomic_compare_exchange_weak(&s_pixels, &used, used + bytes));
    return true;
}

void td_pyui_pixels_give(long bytes)
{
    atomic_fetch_sub(&s_pixels, bytes);
}

td_pyui_image_t *td_pyui_image_new(int w, int h, bool mask)
{
    return td_pyui_image_new_rgb(w, h, mask, false);
}

td_pyui_image_t *td_pyui_image_new_rgb(int w, int h, bool mask, bool rgb)
{
    if (w < 1 || h < 1 || w > 4096 || h > 4096)
        return NULL;
    size_t n = (size_t)w * (size_t)h;
    td_pyui_image_t *img = calloc(1, sizeof(*img) + n * ((mask ? 2 : 1) + (rgb ? 2 : 0)));
    if (!img)
        return NULL;
    img->w = (int16_t)w;
    img->h = (int16_t)h;
    img->rgb = rgb ? (uint16_t *)(img + 1) : NULL;
    img->px = (uint8_t *)(img + 1) + (rgb ? 2 * n : 0);
    img->mask = mask ? img->px + n : NULL;
    return img;
}

long td_pyui_image_bytes(const td_pyui_image_t *img)
{
    return (long)img->w * img->h * ((img->mask ? 2 : 1) + (img->rgb ? 2 : 0));
}

void td_pyui_send_image(uint8_t op, uint16_t id, int a, int b, td_pyui_image_t *img, void (*wait)(void))
{
    td_pyui_msg_t m = {.op = op, .id = id, .a = (int16_t)a, .b = (int16_t)b, .len = sizeof(img)};
    for (;;)
    {
        if (atomic_load(&s_state) != LINK_ON)
            break;
        if (ring_put(&s_link->req, &m, (const char *)&img))
            return;
        if (wait)
            wait();
    }
    if (op == TD_PYUI_IMAGE)
        td_pyui_pixels_give(td_pyui_image_bytes(img));
    free(img);
}

void td_pyui_send_font(uint16_t id, td_pyui_font_t *font, void (*wait)(void))
{
    td_pyui_msg_t m = {.op = TD_PYUI_FONT, .id = id, .len = sizeof(font)};
    while (atomic_load(&s_state) == LINK_ON)
    {
        if (ring_put(&s_link->req, &m, (const char *)&font))
            return;
        if (wait)
            wait();
    }
    td_pyui_pixels_give((long)font->bytes);
    free(font);
}

/* ----------------------------------------------------- desktop side */

static void post(uint8_t op, uint16_t id, int a, int b, int c, int d, const char *text)
{
    link_t *l = s_link;
    if (l->closing)
        return;
    td_pyui_msg_t m = {.op = op, .id = id, .a = (int16_t)a, .b = (int16_t)b, .c = (int16_t)c, .d = (int16_t)d};
    size_t n = text ? strlen(text) : 0;
    m.len = (uint16_t)(n > TD_PYUI_TEXT_MAX ? TD_PYUI_TEXT_MAX : n);
    ring_put(&l->evt, &m, text);   /* full: the script is not reading; drop it */
}

static obj_t *find(uint16_t id)
{
    if (!id)
        return NULL;
    for (int i = 0; i < MAX_OBJS; i++)
        if (s_link->objs[i].kind != K_FREE && s_link->objs[i].id == id)
            return &s_link->objs[i];
    return NULL;
}

static obj_t *new_obj(uint8_t kind, uint16_t id)
{
    for (int i = 0; i < MAX_OBJS; i++)
    {
        obj_t *o = &s_link->objs[i];
        if (o->kind == K_FREE)
        {
            memset(o, 0, sizeof(*o));
            o->kind = kind;
            o->id = id;
            return o;
        }
    }
    post(TD_PYUI_EV_ERROR, id, 0, 0, 0, 0, "too many windows, widgets and dialogs");
    return NULL;
}

/* A canvas's cells, every page counted. */
static int canvas_cells(const obj_t *o)
{
    return o->rect.w * o->rect.h * o->pages;
}

static cell_t *canvas_page(const obj_t *o, int page)
{
    return o->cells + (size_t)page * (size_t)(o->rect.w * o->rect.h);
}

static void release_picture(pic_t *p)
{
    if (p->text && --p->text->refs == 0)
    {
        td_pyui_pixels_give((long)p->text->bytes);
        free(p->text);
    }
    p->text = NULL;
    if (p->bg && --p->bg->refs == 0)
    {
        td_pyui_pixels_give((long)p->bg->bytes);
        free(p->bg);
    }
    p->bg = NULL;
    p->used = false;
}

static void forget_pictures(const obj_t *o)
{
    for (int i = 0; i < MAX_PICS; i++)
    {
        pic_t *p = &s_link->pics[i];
        if (p->used && ((o->kind == K_CANVAS && p->canvas == o->id) || (o->kind == K_IMAGE && p->img == o->id) || (o->kind == K_FONT && p->font == o->id)))
            release_picture(p);
    }
}

static void free_obj(obj_t *o)
{
    if (o->kind == K_CANVAS || o->kind == K_IMAGE || o->kind == K_FONT)
        forget_pictures(o);
    if (o->kind == K_CANVAS)
        s_link->cells -= canvas_cells(o);
    if (o->px)
        td_pyui_pixels_give(o->px_bytes);
    free(o->px);
    if (o->img)
        td_pyui_pixels_give(td_pyui_image_bytes(o->img));
    free(o->img);
    if (o->font)
        td_pyui_pixels_give((long)o->font->bytes);
    free(o->font);
    free(o->own_mask);
    free(o->items);
    free(o->offs);
    free(o->cells);
    memset(o, 0, sizeof(*o));
}

static uint32_t hash(const char *s)
{
    uint32_t h = 2166136261u;
    while (*s)
        h = (h ^ (uint8_t)*s++) * 16777619u;
    return h;
}

/* Windows the user (or the script) resized, maximised or restored: the
 * desktop has no callback for it, so compare the client size each tick. */
static void check_sizes(void)
{
    for (int i = 0; i < MAX_OBJS; i++)
    {
        obj_t *o = &s_link->objs[i];
        if (o->kind != K_WINDOW)
            continue;
        td_rect_t c = td_win_client(o->w);
        if (c.w != o->rect.w || c.h != o->rect.h)
        {
            o->rect.w = c.w;
            o->rect.h = c.h;
            post(TD_PYUI_EV_RESIZE, o->id, c.w, c.h, 0, 0, NULL);
        }
    }
}

/* Text box and list changes the script has not heard of yet: sent before
 * any click, so a handler reads the current text. */
static void poll_values(void)
{
    for (int i = 0; i < MAX_OBJS; i++)
    {
        obj_t *o = &s_link->objs[i];
        if (o->kind != K_WIDGET)
            continue;
        if (o->wtype == TD_PYUI_W_TEXTBOX)
        {
            const char *t = td_widget_text(o->wd);
            uint32_t h = hash(t);
            if (h != o->seen)
            {
                o->seen = h;
                post(TD_PYUI_EV_CHANGE, o->id, 0, 0, 0, 0, t);
            }
        }
        else if (o->wtype == TD_PYUI_W_LIST)
        {
            int sel = td_list_selected(o->wd);
            if ((uint32_t)sel != o->seen)
            {
                o->seen = (uint32_t)sel;
                post(TD_PYUI_EV_CHANGE, o->id, sel, 0, 0, 0, NULL);
            }
        }
    }
}

static void on_click(td_widget_t *w, void *user)
{
    obj_t *o = user;
    poll_values();
    post(TD_PYUI_EV_CLICK, o->id, o->wtype == TD_PYUI_W_LIST ? td_list_selected(w) : 0, 0, 0, 0, NULL);
}

static void on_check(td_widget_t *w, void *user)
{
    obj_t *o = user;
    poll_values();
    post(TD_PYUI_EV_CHANGE, o->id, td_checkbox_get(w), 0, 0, 0, NULL);
}

static const char *list_item(td_widget_t *w, int index, int *fg, void *user)
{
    (void)w;
    (void)fg;
    obj_t *o = user;
    return index >= 0 && index < o->nitems ? o->items + o->offs[index] : "";
}

static void cell_pixels(const obj_t *o, int page, int x, int y, uint8_t q[4]);
static uint8_t main_colour(const uint8_t q[4]);

/* A canvas's cells: text, or the cell's four pixels as one quadrant
 * character (Fine pictures) or an upper half block (two of them). */
static void show_pictures(const obj_t *o, td_rect_t client);

static void draw_canvas(const obj_t *o, const td_theme_t *t, td_rect_t client)
{
    bool quad = td_canvas_detail() == TD_CANVAS_QUAD && !td_get_ascii_mode();
    const cell_t *cells = canvas_page(o, o->show);
    for (int y = 0; y < o->rect.h; y++)
    {
        for (int x = 0; x < o->rect.w; x++)
        {
            const cell_t *c = &cells[y * o->rect.w + x];
            uint8_t q[4];
            bool px = o->px != NULL && (c->ch == PX_CELL || c->bg == TD_PYUI_CLEAR);
            if (px)
                cell_pixels(o, o->show, x, y, q);
            if (c->ch == PX_CELL && px)
            {
                uint32_t ch = 0x2580;
                uint8_t fg = q[0], bg = q[2];
                if (quad)
                    td_canvas_quad_cell(q, &ch, &fg, &bg);
                td_putc(o->rect.x + x, o->rect.y + y, ch, fg, bg, 0);
                continue;
            }
            uint8_t bg = c->bg >= 0 ? (uint8_t)c->bg : px ? main_colour(q)
                                                          : t->win_bg;
            td_putc(o->rect.x + x, o->rect.y + y, c->ch && c->ch != PX_CELL ? c->ch : ' ',
                    c->fg < 0 ? t->win_fg : (uint8_t)c->fg, bg, 0);
        }
    }
    show_pictures(o, client);
}

static void win_draw(td_window_t *win, int cw, int ch)
{
    (void)cw;
    (void)ch;
    obj_t *wo = win->user;
    const td_theme_t *t = td_theme();
    for (int i = 0; i < MAX_OBJS; i++)
    {
        obj_t *o = &s_link->objs[i];
        if (o->kind == K_CANVAS && o->win == wo->id)
            draw_canvas(o, t, td_win_client(win));
    }
}

static bool win_event(td_window_t *win, const td_event_t *ev)
{
    obj_t *wo = win->user;
    if (ev->type == TD_EV_KEY)
    {
        if (ev->key == 'c' && ev->mods == TD_MOD_CTRL)
        {
            td_terminal_send("\x03", 1);   /* stops the script, as in the Terminal */
            return true;
        }
        poll_values();
        post(TD_PYUI_EV_KEY, wo->id, (int)(ev->key & 0xFFFFu), (int)(ev->key >> 16), ev->mods, 0, NULL);
        return ev->key != TD_KEY_TAB;   /* Tab still moves between widgets */
    }
    if (ev->type != TD_EV_MOUSE || ev->action == TD_MOUSE_MOVE)
        return false;
    if (ev->action == TD_MOUSE_RELEASE && ev->button == 0 && wo->held_canvas)
    {
        obj_t *o = find(wo->held_canvas);
        wo->held_canvas = 0;
        if (o && o->kind == K_CANVAS && o->win == wo->id)
        {
            post(TD_PYUI_EV_MOUSE, o->id, ev->x - o->rect.x, ev->y - o->rect.y, ev->button, ev->action, NULL);
            return true;
        }
    }
    for (int i = 0; i < MAX_OBJS; i++)
    {
        obj_t *o = &s_link->objs[i];
        if (o->kind == K_CANVAS && o->win == wo->id && td_rect_contains(o->rect, ev->x, ev->y))
        {
            if (ev->action == TD_MOUSE_PRESS && ev->button == 0)
                wo->held_canvas = o->id;
            post(TD_PYUI_EV_MOUSE, o->id, ev->x - o->rect.x, ev->y - o->rect.y, ev->button, ev->action, NULL);
            return true;
        }
    }
    return false;
}

/* The window is going (closed by the user, the script or a user switch):
 * forget it and everything in it. Its widgets are freed by the desktop. */
static void win_close(td_window_t *win)
{
    obj_t *wo = win->user;
    if (!s_link || wo->kind != K_WINDOW)
        return;
    for (int i = 0; i < MAX_OBJS; i++)
    {
        obj_t *o = &s_link->objs[i];
        if ((o->kind == K_WIDGET || o->kind == K_CANVAS) && o->win == wo->id)
            free_obj(o);
    }
    if (!wo->quiet)
        post(TD_PYUI_EV_CLOSE, wo->id, 0, 0, 0, 0, NULL);
    free_obj(wo);
}

static void on_msgbox(int button, void *user)
{
    obj_t *o = user;
    post(TD_PYUI_EV_RESULT, o->id, button, 0, 0, 0, NULL);
    free_obj(o);
}

static void on_input(const char *text, void *user)
{
    obj_t *o = user;
    post(TD_PYUI_EV_RESULT, o->id, 0, 0, 0, 0, text);
    free_obj(o);
}

static void on_menu(int item, void *user)
{
    obj_t *o = user;
    post(TD_PYUI_EV_RESULT, o->id, item, 0, 0, 0, NULL);
    free_obj(o);
}

/* An input box closed with Cancel or Esc, a menu dismissed: no callback
 * says so, so look. */
static void check_dialogs(void)
{
    for (int i = 0; i < MAX_OBJS; i++)
    {
        obj_t *o = &s_link->objs[i];
        if ((o->kind == K_DIALOG && !td_win_is_open(o->w)) || (o->kind == K_MENU && !td_menu_is_open()))
        {
            post(TD_PYUI_EV_RESULT, o->id, -1, 0, 0, 0, NULL);
            free_obj(o);
        }
    }
}

/* Split "a\0b\0c" (len bytes) into up to max parts. */
static int split(char *text, int len, const char **parts, int max)
{
    int n = 0;
    int i = 0;
    while (n < max && i <= len)
    {
        parts[n++] = text + i;
        i += (int)strlen(text + i) + 1;
    }
    return n;
}

static bool list_add(obj_t *o, const char *text, int len)
{
    const char *p = text;
    const char *end = text + len;
    while (p <= end && o->nitems < LIST_ITEMS)
    {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        int n = nl ? (int)(nl - p) : (int)(end - p);
        if (o->items_len + n + 1 > LIST_BYTES)
            return false;
        if (o->items_len + n + 1 > o->items_cap)
        {
            int cap = o->items_cap ? o->items_cap * 2 : 256;
            while (cap < o->items_len + n + 1)
                cap *= 2;
            char *b = realloc(o->items, (size_t)cap);
            if (!b)
                return false;
            o->items = b;
            o->items_cap = cap;
        }
        if (o->nitems == o->offs_cap)
        {
            int cap = o->offs_cap ? o->offs_cap * 2 : 16;
            uint16_t *b = realloc(o->offs, (size_t)cap * sizeof(*b));
            if (!b)
                return false;
            o->offs = b;
            o->offs_cap = cap;
        }
        o->offs[o->nitems++] = (uint16_t)o->items_len;
        memcpy(o->items + o->items_len, p, (size_t)n);
        o->items_len += n;
        o->items[o->items_len++] = '\0';
        if (!nl)
            break;
        p = nl + 1;
    }
    return true;
}

/* A blank with no background of its own shows the cell's pixels. */
static void set_cell(cell_t *c, uint32_t cp, int fg, int bg)
{
    c->ch = (uint16_t)(cp == ' ' && bg == TD_PYUI_CLEAR ? PX_CELL : cp > 0xFFFE ? '?'
                                                                                : cp);
    c->fg = (int16_t)fg;
    c->bg = (int16_t)bg;
}

static void canvas_put(obj_t *o, int x, int y, const char *s, int fg, int bg)
{
    while (*s && y < o->rect.h)
    {
        uint32_t cp = td_utf8_next(&s);
        if (cp == '\n')
        {
            y++;
            x = 0;
            continue;
        }
        if (x >= 0 && y >= 0 && x < o->rect.w)
            set_cell(&canvas_page(o, o->draw)[y * o->rect.w + x], cp, fg, bg);
        x++;
    }
}

static void canvas_fill(obj_t *o, int x0, int y0, int w, int h, uint32_t cp, int fg, int bg)
{
    cell_t *cells = canvas_page(o, o->draw);
    for (int y = y0 < 0 ? 0 : y0; y < y0 + h && y < o->rect.h; y++)
    {
        for (int x = x0 < 0 ? 0 : x0; x < x0 + w && x < o->rect.w; x++)
            set_cell(&cells[y * o->rect.w + x], cp, fg, bg);
    }
}

/* Big characters for canvas.big(): 3x5 "pixels" each ('.' and ':' 1x5),
 * two pixel rows to a cell with the half blocks, so a pixel is square;
 * `scale` makes each pixel scale x scale. Letters are upper case. */
typedef struct
{
    char ch;
    const char *rows[5];
} big_glyph_t;

static const big_glyph_t s_big[] = {
    {' ', {"...", "...", "...", "...", "..."}},
    {'0', {"###", "#.#", "#.#", "#.#", "###"}},
    {'1', {".#.", "##.", ".#.", ".#.", "###"}},
    {'2', {"###", "..#", "###", "#..", "###"}},
    {'3', {"###", "..#", ".##", "..#", "###"}},
    {'4', {"#.#", "#.#", "###", "..#", "..#"}},
    {'5', {"###", "#..", "###", "..#", "###"}},
    {'6', {"###", "#..", "###", "#.#", "###"}},
    {'7', {"###", "..#", ".#.", ".#.", ".#."}},
    {'8', {"###", "#.#", "###", "#.#", "###"}},
    {'9', {"###", "#.#", "###", "..#", "###"}},
    {'-', {"...", "...", "###", "...", "..."}},
    {'+', {"...", ".#.", "###", ".#.", "..."}},
    {'%', {"#.#", "..#", ".#.", "#..", "#.#"}},
    {'/', {"..#", "..#", ".#.", "#..", "#.."}},
    {'.', {".", ".", ".", ".", "#"}},
    {':', {".", "#", ".", "#", "."}},
    {'?', {"##.", "..#", ".#.", "...", ".#."}},
    {'A', {".#.", "#.#", "###", "#.#", "#.#"}},
    {'B', {"##.", "#.#", "##.", "#.#", "##."}},
    {'C', {".##", "#..", "#..", "#..", ".##"}},
    {'D', {"##.", "#.#", "#.#", "#.#", "##."}},
    {'E', {"###", "#..", "##.", "#..", "###"}},
    {'F', {"###", "#..", "##.", "#..", "#.."}},
    {'G', {".##", "#..", "#.#", "#.#", ".##"}},
    {'H', {"#.#", "#.#", "###", "#.#", "#.#"}},
    {'I', {"###", ".#.", ".#.", ".#.", "###"}},
    {'J', {"..#", "..#", "..#", "#.#", ".#."}},
    {'K', {"#.#", "#.#", "##.", "#.#", "#.#"}},
    {'L', {"#..", "#..", "#..", "#..", "###"}},
    {'M', {"#.#", "###", "###", "#.#", "#.#"}},
    {'N', {"##.", "#.#", "#.#", "#.#", "#.#"}},
    {'O', {".#.", "#.#", "#.#", "#.#", ".#."}},
    {'P', {"##.", "#.#", "##.", "#..", "#.."}},
    {'Q', {".#.", "#.#", "#.#", "##.", ".##"}},
    {'R', {"##.", "#.#", "##.", "#.#", "#.#"}},
    {'S', {".##", "#..", ".#.", "..#", "##."}},
    {'T', {"###", ".#.", ".#.", ".#.", ".#."}},
    {'U', {"#.#", "#.#", "#.#", "#.#", "###"}},
    {'V', {"#.#", "#.#", "#.#", "#.#", ".#."}},
    {'W', {"#.#", "#.#", "###", "###", "#.#"}},
    {'X', {"#.#", "#.#", ".#.", "#.#", "#.#"}},
    {'Y', {"#.#", "#.#", ".#.", ".#.", ".#."}},
    {'Z', {"###", "..#", ".#.", "#..", "###"}},
};

#define BIG_MAX 64   /* characters drawn by one request */

static const big_glyph_t *big_glyph(uint32_t cp)
{
    if (cp >= 'a' && cp <= 'z')
        cp -= 'a' - 'A';
    for (size_t i = 0; i < sizeof(s_big) / sizeof(s_big[0]); i++)
        if ((uint32_t)(unsigned char)s_big[i].ch == cp)
            return &s_big[i];
    return big_glyph('?');
}

static void canvas_set(obj_t *o, int x, int y, uint32_t cp, int fg, int bg)
{
    if (x < 0 || y < 0 || x >= o->rect.w || y >= o->rect.h)
        return;
    set_cell(&canvas_page(o, o->draw)[y * o->rect.w + x], cp, fg, bg);
}

/* Text in big characters at x, y. width > 0: right-aligned in that many
 * characters, blanks before it (so a shorter number clears a longer one). */
static void canvas_big(obj_t *o, int x, int y, const char *s, int fg, int bg, int scale, int width)
{
    static const uint32_t half[4] = {' ', 0x2580, 0x2584, 0x2588};   /* none, upper, lower, both */
    const big_glyph_t *g[BIG_MAX];
    int n = 0;
    while (*s && n < BIG_MAX)
        g[n++] = big_glyph(td_utf8_next(&s));
    scale = scale < 1 ? 1 : scale > 4 ? 4
                                      : scale;
    int first = 0, pad = 0;
    if (width > BIG_MAX)
        width = BIG_MAX;
    if (width > 0 && n > width)
        first = n - width;
    else if (width > 0)
        pad = width - n;
    int rows = 5 * scale, cx = x;
    for (int i = 0; i < pad + n - first; i++)
    {
        const big_glyph_t *gl = i < pad ? big_glyph(' ') : g[first + i - pad];
        int cw = (int)strlen(gl->rows[0]) * scale;
        for (int r = 0; r < rows; r += 2)
        {
            for (int c = 0; c < cw; c++)
            {
                int gc = c / scale;
                int top = gl->rows[r / scale][gc] == '#';
                int bot = r + 1 < rows && gl->rows[(r + 1) / scale][gc] == '#';
                canvas_set(o, cx + c, y + r / 2, half[top | bot << 1], fg, bg);
            }
            if (i + 1 < pad + n - first)   /* the gap to the next character */
                for (int c = cw; c < cw + scale; c++)
                    canvas_set(o, cx + c, y + r / 2, ' ', fg, bg);
        }
        cx += cw + scale;
    }
}

/* ------------------------------------------------------------ pixels */

/* A cell's background as a palette index, for the pixels under it. */
static uint8_t cell_bg(const cell_t *c)
{
    return c->bg >= 0 ? (uint8_t)c->bg : td_theme()->win_bg;
}

static long layer_size(const obj_t *o)
{
    return 4L * o->rect.w * o->rect.h;
}

static uint8_t *canvas_layer(const obj_t *o, int page)
{
    return o->px + (size_t)page * (size_t)layer_size(o);
}

static uint16_t *canvas_rgb(const obj_t *o, int page)
{
    return o->rgb ? o->rgb + (size_t)page * layer_size(o) : NULL;
}

/* Give each cell's four pixels the cell's background. */
static void layer_from_cells(obj_t *o, int page)
{
    const cell_t *cells = canvas_page(o, page);
    uint8_t *px = canvas_layer(o, page);
    int pw = 2 * o->rect.w;
    for (int y = 0; y < o->rect.h; y++)
    {
        for (int x = 0; x < o->rect.w; x++)
        {
            uint8_t c = cell_bg(&cells[y * o->rect.w + x]);
            uint8_t *p = px + (2 * y) * pw + 2 * x;
            p[0] = p[1] = p[pw] = p[pw + 1] = c;
            if (o->rgb)
            {
                uint16_t *q = canvas_rgb(o, page) + 2 * y * pw + 2 * x;
                q[0] = q[1] = q[pw] = q[pw + 1] = td_palette_rgb565(c);
            }
        }
    }
}

/* The canvas's pixel layers, allocated by the first pixel drawn. */
static bool canvas_pixels(obj_t *o)
{
    if (o->px)
        return true;
    long n = layer_size(o) * o->pages;
    long bytes = n * (td_pyui_rgb_enabled() ? 3 : 1);
    if (!td_pyui_pixels_take(bytes))
    {
        post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, PIXELS_MSG);
        return false;
    }
    o->px = malloc((size_t)bytes);
    if (!o->px)
    {
        td_pyui_pixels_give(bytes);
        post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "out of memory");
        return false;
    }
    o->px_bytes = bytes;
    o->rgb = td_pyui_rgb_enabled() ? (uint16_t *)(o->px + n) : NULL;
    o->rgb_serial = ++s_link->pic_serial;
    for (int p = 0; p < o->pages; p++)
        layer_from_cells(o, p);
    return true;
}

/* What pixel requests draw into: a canvas's draw page or an image. */
typedef struct
{
    uint8_t *px, *mask;
    int w, h;
    cell_t *cells;      /* canvas: its cells (turned into pixel cells) */
    int cols;
    uint16_t *rgb;
} surf_t;

static bool surface(obj_t *o, surf_t *s)
{
    memset(s, 0, sizeof(*s));
    if (o->kind == K_IMAGE)
    {
        s->px = o->img->px;
        s->mask = o->img->mask;
        s->w = o->img->w;
        s->h = o->img->h;
        s->rgb = o->img->rgb;
        return true;
    }
    if (o->kind != K_CANVAS || !canvas_pixels(o))
        return false;
    s->px = canvas_layer(o, o->draw);
    s->w = 2 * o->rect.w;
    s->h = 2 * o->rect.h;
    s->cells = canvas_page(o, o->draw);
    s->cols = o->rect.w;
    s->rgb = canvas_rgb(o, o->draw);
    return true;
}

/* An image that is to get a transparent pixel needs a mask. */
static bool image_mask(obj_t *o, surf_t *s)
{
    if (s->mask || o->kind != K_IMAGE)
        return true;
    long n = (long)o->img->w * o->img->h;
    if (!td_pyui_pixels_take(n))
    {
        post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, PIXELS_MSG);
        return false;
    }
    o->own_mask = calloc(1, (size_t)n);
    if (!o->own_mask)
    {
        td_pyui_pixels_give(n);
        post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "out of memory");
        return false;
    }
    o->img->mask = s->mask = o->own_mask;
    return true;
}

/* One pixel. On a canvas, a cell that showed text shows its pixels from
 * now on, its other pixels in the text's background; TD_PYUI_CLEAR there
 * is the window's background. In an image, TD_PYUI_CLEAR is transparent. */
static void plot(surf_t *s, int x, int y, int color)
{
    if (x < 0 || y < 0 || x >= s->w || y >= s->h)
        return;
    if (s->rgb && !s->px)
    {
        if (color >= 0)
            s->rgb[y * s->w + x] = td_palette_rgb565((uint8_t)color);
        return;
    }
    if (s->cells)
    {
        cell_t *c = &s->cells[(y >> 1) * s->cols + (x >> 1)];
        if (c->ch != PX_CELL)
        {
            uint8_t b = cell_bg(c);
            uint8_t *p = s->px + (y & ~1) * s->w + (x & ~1);
            p[0] = p[1] = p[s->w] = p[s->w + 1] = b;
            if (s->rgb)
            {
                uint16_t *q = s->rgb + (y & ~1) * s->w + (x & ~1);
                q[0] = q[1] = q[s->w] = q[s->w + 1] = td_palette_rgb565(b);
            }
            c->ch = PX_CELL;
            c->fg = -1;
            c->bg = TD_PYUI_CLEAR;
        }
        s->px[y * s->w + x] = color < 0 ? td_theme()->win_bg : (uint8_t)color;
        if (s->rgb)
            s->rgb[y * s->w + x] = td_palette_rgb565(s->px[y * s->w + x]);
        return;
    }
    size_t i = (size_t)y * (size_t)s->w + (size_t)x;
    if (s->mask)
        s->mask[i] = color == TD_PYUI_CLEAR;
    if (color >= 0)
        s->px[i] = (uint8_t)color;
    if (s->rgb && color >= 0)
        s->rgb[i] = td_palette_rgb565((uint8_t)color);
}

static void fill_rect(surf_t *s, int x0, int y0, int w, int h, int color)
{
    int x1 = x0 + w > s->w ? s->w : x0 + w, y1 = y0 + h > s->h ? s->h : y0 + h;
    if (s->rgb && !s->px && !s->cells && !s->mask)
    {
        if (color < 0)
            return;
        uint16_t value = td_palette_rgb565((uint8_t)color);
        for (int y = y0 < 0 ? 0 : y0; y < y1; y++)
            for (int x = x0 < 0 ? 0 : x0; x < x1; x++)
                s->rgb[(size_t)y * s->w + x] = value;
        return;
    }
    for (int y = y0 < 0 ? 0 : y0; y < y1; y++)
        for (int x = x0 < 0 ? 0 : x0; x < x1; x++)
            plot(s, x, y, color);
}

static void line(surf_t *s, int x0, int y0, int x1, int y1, int color)
{
    int dx = abs(x1 - x0), dy = -abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (int n = 0; n < 20000; n++)   /* no line on a canvas is longer */
    {
        plot(s, x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

/* Is pixel (x, y) of a w x h rectangle with corners of radius r inside? */
static bool in_round_rect(int x, int y, int w, int h, int r)
{
    if (x < 0 || y < 0 || x >= w || y >= h)
        return false;
    int cx = x < r ? r : x >= w - r ? w - r - 1
                                    : x;
    int cy = y < r ? r : y >= h - r ? h - r - 1
                                    : y;
    int dx = x - cx, dy = y - cy;
    return dx * dx + dy * dy <= r * r + r;
}

static void round_rect(surf_t *s, int x0, int y0, int w, int h, int r, int color, bool outline)
{
    if (r > w / 2)
        r = w / 2;
    if (r > h / 2)
        r = h / 2;
    /* On a canvas whose cells it covers whole, both pixels of a column pair
     * get the shape of the inner one: a cell then looks the same whether it
     * shows both (Fine pictures) or only its left pixel (Normal), and the
     * left side is rounded like the right. */
    bool pairs = s->cells && !(x0 & 1) && !(w & 1);
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            int ex = pairs ? (x < w / 2 ? x | 1 : x & ~1) : x;
            if (!in_round_rect(ex, y, w, h, r))
                continue;
            if (outline && in_round_rect(ex - (pairs ? 2 : 1), y, w, h, r) &&
                in_round_rect(ex + (pairs ? 2 : 1), y, w, h, r) && in_round_rect(ex, y - 1, w, h, r) &&
                in_round_rect(ex, y + 1, w, h, r))
                continue;
            plot(s, x0 + x, y0 + y, color);
        }
    }
}

static uint8_t gradient_color(const uint8_t *g, int x, int y, int w, int h)
{
    int n = (g[6] ? w : h) - 1, t = g[6] ? x : y;
    if (n < 1)
        n = 1;
    uint8_t rgb[3];
    for (int k = 0; k < 3; k++)
        rgb[k] = (uint8_t)(g[k] + (g[k + 3] - g[k]) * t / n);
    return td_rgb256(rgb[0], rgb[1], rgb[2]);
}
static void gradient_shape(surf_t *s, const td_pyui_msg_t *m, const uint8_t *g, bool disk)
{
    int w = disk ? 2 * m->c + 1 : m->c, h = disk ? w : m->d;
    int ox = disk ? m->a - m->c : m->a, oy = disk ? m->b - m->c : m->b, r = disk ? m->c : m->f;
    if (w <= 0 || h <= 0 || r < 0)
        return;
    if (r > w / 2)
        r = w / 2;
    if (r > h / 2)
        r = h / 2;
    int y0 = oy < 0 ? -oy : 0, y1 = h < s->h - oy ? h : s->h - oy, x0 = ox < 0 ? -ox : 0, x1 = w < s->w - ox ? w : s->w - ox;
    bool pairs = !disk && s->cells && !(ox & 1) && !(w & 1);
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
        {
            int ex = pairs ? (x < w / 2 ? x | 1 : x & ~1) : x;
            bool inside = disk ? (x - r) * (x - r) + (y - r) * (y - r) <= r * r + r : in_round_rect(ex, y, w, h, r);
            if (!inside)
                continue;
            if (m->flags & 1)
            {
                bool inner = disk ? (x - r - 1) * (x - r - 1) + (y - r) * (y - r) <= r * r + r && (x - r + 1) * (x - r + 1) + (y - r) * (y - r) <= r * r + r && (x - r) * (x - r) + (y - r - 1) * (y - r - 1) <= r * r + r && (x - r) * (x - r) + (y - r + 1) * (y - r + 1) <= r * r + r : in_round_rect(ex - (pairs ? 2 : 1), y, w, h, r) && in_round_rect(ex + (pairs ? 2 : 1), y, w, h, r) && in_round_rect(ex, y - 1, w, h, r) && in_round_rect(ex, y + 1, w, h, r);
                if (inner)
                    continue;
            }
            plot(s, ox + x, oy + y, gradient_color(g, x, y, w, h));
            if (s->rgb)
            {
                int n = (g[6] ? w : h) - 1, t = g[6] ? x : y;
                if (n < 1)
                    n = 1;
                s->rgb[(oy + y) * s->w + ox + x] = td_rgb565((uint8_t)(g[0] + (g[3] - g[0]) * t / n), (uint8_t)(g[1] + (g[4] - g[1]) * t / n), (uint8_t)(g[2] + (g[5] - g[2]) * t / n));
            }
        }
}

/* A complete bar in one pass. Keep the corners' background and paint the
 * whole track, including the old filled area when the value falls. */
static void pixel_bar(surf_t *s, const td_pyui_msg_t *m, const char *text)
{
    if ((m->len != 3 && m->len != 10) || m->c <= 0 || m->d <= 0)
        return;
    const uint8_t *colors = (const uint8_t *)text;
    int w = m->c, h = m->d, r = m->f < 0 ? 0 : m->f;
    if (r > w / 2)
        r = w / 2;
    if (r > h / 2)
        r = h / 2;
    int value = m->e < 0 ? 0 : m->e > 1000 ? 1000
                                           : m->e;
    bool pairs = s->cells && !(m->a & 1) && !(w & 1);
    int filled = pairs ? 2 * ((w / 2 * value + 500) / 1000) : (w * value + 500) / 1000;
    int x0 = m->a < 0 ? -m->a : 0, y0 = m->b < 0 ? -m->b : 0;
    int x1 = w < s->w - m->a ? w : s->w - m->a;
    int y1 = h < s->h - m->b ? h : s->h - m->b;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++)
        {
            int ex = pairs ? (x < w / 2 ? x | 1 : x & ~1) : x;
            if (in_round_rect(ex, y, w, h, r))
            {
                plot(s, m->a + x, m->b + y,
                     x >= filled ? colors[1] : (m->flags & 1) && y == 0 ? colors[2]
                                           : m->len == 10               ? gradient_color(colors + 3, x, y, w, h)
                                                                        : colors[0]);
                if (s->rgb && m->len == 10 && x < filled && !((m->flags & 1) && y == 0))
                {
                    const uint8_t *g = colors + 3;
                    int n = (g[6] ? w : h) - 1, t = g[6] ? x : y;
                    if (n < 1)
                        n = 1;
                    s->rgb[(m->b + y) * s->w + m->a + x] = td_rgb565((uint8_t)(g[0] + (g[3] - g[0]) * t / n), (uint8_t)(g[1] + (g[4] - g[1]) * t / n), (uint8_t)(g[2] + (g[5] - g[2]) * t / n));
                }
            }
        }
}

static void circle(surf_t *s, int cx, int cy, int r, int color, bool outline)
{
    int rr = r * r + r;
    for (int y = -r; y <= r; y++)
    {
        for (int x = -r; x <= r; x++)
        {
            if (x * x + y * y > rr)
                continue;
            if (outline && (x - 1) * (x - 1) + y * y <= rr && (x + 1) * (x + 1) + y * y <= rr &&
                x * x + (y - 1) * (y - 1) <= rr && x * x + (y + 1) * (y + 1) <= rr)
                continue;
            plot(s, cx + x, cy + y, color);
        }
    }
}

/* From one RGB colour to another, dithered to the palette. */
static void gradient(surf_t *s, int x0, int y0, int w, int h, const uint8_t *rgb, bool across)
{
    int n = (across ? w : h) - 1;
    if (n < 1)
        n = 1;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            int t = across ? x : y, px = x0 + x, py = y0 + y;
            if (px < 0 || py < 0 || px >= s->w || py >= s->h)
                continue;
            uint8_t v[3];
            for (int k = 0; k < 3; k++)
                v[k] = (uint8_t)(rgb[k] + (rgb[3 + k] - rgb[k]) * t / n);
            plot(s, px, py, td_rgb256_dither(v[0], v[1], v[2], px, py));
            if (s->rgb)
                s->rgb[py * s->w + px] = td_rgb565(v[0], v[1], v[2]);
        }
    }
}

/* Part (sx, sy, sw, sh) of an image into (dx, dy, dw, dh), stretched or
 * shrunk to it (nearest pixel); transparent pixels leave what is there. */
static void blit(surf_t *d, int dx, int dy, int dw, int dh, const td_pyui_image_t *img, int sx, int sy, int sw,
                 int sh)
{
    if (dw <= 0 || dh <= 0 || sw <= 0 || sh <= 0)
        return;
    if (d->rgb && !d->px && !d->cells && !d->mask && img->rgb && d->rgb != img->rgb)
    {
        int x0 = dx < 0 ? -dx : 0, x1 = dw < d->w - dx ? dw : d->w - dx;
        int y0 = dy < 0 ? -dy : 0, y1 = dh < d->h - dy ? dh : d->h - dy;
        if (x1 <= x0 || y1 <= y0)
            return;
        int previous = -1;
        bool opaque = false;
        uint16_t *last = NULL;
        for (int y = y0; y < y1; y++)
        {
            int iy = sy + (int)((long)y * sh / dh);
            const uint16_t *src = img->rgb + (size_t)iy * img->w;
            const uint8_t *mask = img->mask ? img->mask + (size_t)iy * img->w : NULL;
            uint16_t *dst = d->rgb + (size_t)(dy + y) * d->w + dx + x0;
            if (iy != previous)
            {
                opaque = true;
                if (mask)
                    for (int x = sx; x < sx + sw; x++)
                        if (mask[x])
                        {
                            opaque = false;
                            break;
                        }
            }
            if (iy == previous && last && opaque)
                memcpy(dst, last, (size_t)(x1 - x0) * sizeof(*dst));
            else if (opaque && sw == dw)
                memcpy(dst, src + sx + x0, (size_t)(x1 - x0) * sizeof(*dst));
            else
                for (int x = x0; x < x1; x++)
                {
                    int ix = sx + (int)((long)x * sw / dw);
                    if (opaque || !mask[ix])
                        dst[x - x0] = src[ix];
                }
            previous = iy;
            last = dst;
        }
        return;
    }
    if (!d->rgb && !d->cells && !d->mask && !img->mask && d->px != img->px)
    {
        int x0 = dx < 0 ? -dx : 0, x1 = dw < d->w - dx ? dw : d->w - dx;
        int y0 = dy < 0 ? -dy : 0, y1 = dh < d->h - dy ? dh : d->h - dy;
        if (x1 <= x0 || y1 <= y0)
            return;
        if (sw == 1 && sh == 1)
        {
            uint8_t c = img->px[(size_t)sy * img->w + sx];
            for (int y = y0; y < y1; y++)
                memset(d->px + (size_t)(dy + y) * d->w + dx + x0, c, (size_t)(x1 - x0));
            return;
        }
        int previous = -1;
        uint8_t *last = NULL;
        for (int y = y0; y < y1; y++)
        {
            int iy = sy + (int)((long)y * sh / dh);
            uint8_t *dst = d->px + (size_t)(dy + y) * d->w + dx + x0;
            if (iy == previous && last)
                memcpy(dst, last, (size_t)(x1 - x0));
            else if (sw == dw)
                memcpy(dst, img->px + (size_t)iy * img->w + sx + x0, (size_t)(x1 - x0));
            else
                for (int x = x0; x < x1; x++)
                    dst[x - x0] = img->px[(size_t)iy * img->w + sx + (int)((long)x * sw / dw)];
            previous = iy;
            last = dst;
        }
        return;
    }
    int y0 = dy < 0 ? -dy : 0, y1 = dh < d->h - dy ? dh : d->h - dy;
    int x0 = dx < 0 ? -dx : 0, x1 = dw < d->w - dx ? dw : d->w - dx;
    for (int y = y0; y < y1; y++)
    {
        int ty = dy + y;
        if (ty < 0 || ty >= d->h)
            continue;
        int iy = sy + (int)((long)y * sh / dh);
        const uint8_t *row = img->px + (size_t)iy * (size_t)img->w;
        const uint8_t *mrow = img->mask ? img->mask + (size_t)iy * (size_t)img->w : NULL;
        for (int x = x0; x < x1; x++)
        {
            int ix = sx + (int)((long)x * sw / dw);
            if (!mrow || !mrow[ix])
            {
                plot(d, dx + x, ty, row[ix]);
                if (d->rgb && img->rgb && dx + x >= 0 && dx + x < d->w)
                    d->rgb[(size_t)ty * d->w + dx + x] = img->rgb[(size_t)iy * img->w + ix];
            }
        }
    }
}

/* A nine-slice panel: the corners as they are, the edges stretched along
 * and the middle both ways, so one small picture makes a button or frame
 * of any size. */
static void nine(surf_t *d, int dx, int dy, int dw, int dh, const td_pyui_image_t *img, int sx, int sy, int sw,
                 int sh, int b)
{
    int lim[4] = {dw / 2, dh / 2, sw / 2, sh / 2};
    for (int i = 0; i < 4; i++)
        if (b > lim[i])
            b = lim[i];
    if (b <= 0)
    {
        blit(d, dx, dy, dw, dh, img, sx, sy, sw, sh);
        return;
    }
    int sxs[4] = {sx, sx + b, sx + sw - b, sx + sw}, dxs[4] = {dx, dx + b, dx + dw - b, dx + dw};
    int sys[4] = {sy, sy + b, sy + sh - b, sy + sh}, dys[4] = {dy, dy + b, dy + dh - b, dy + dh};
    for (int j = 0; j < 3; j++)
        for (int i = 0; i < 3; i++)
            blit(d, dxs[i], dys[j], dxs[i + 1] - dxs[i], dys[j + 1] - dys[j], img, sxs[i], sys[j],
                 sxs[i + 1] - sxs[i], sys[j + 1] - sys[j]);
}

/* A pointer passed in a request's text (TD_PYUI_IMAGE, _IMAGE_DATA). */
static td_pyui_image_t *msg_image(const td_pyui_msg_t *m, const char *text)
{
    td_pyui_image_t *img = NULL;
    if (m->len == sizeof(img))
        memcpy(&img, text, sizeof(img));
    return img;
}

static void draw_image(obj_t *o, const td_pyui_msg_t *m, const char *text)
{
    obj_t *io = find(m->win);
    surf_t s;
    if (!io || io->kind != K_IMAGE || io == o)
    {
        post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "no such image");
        return;
    }
    if (!surface(o, &s))
        return;
    const td_pyui_image_t *img = io->img;
    td_pyui_part_t part = {0, 0, 0};
    if (m->len >= sizeof(part))
        memcpy(&part, text, sizeof(part));
    int sx = m->e < 0 ? 0 : m->e, sy = m->f < 0 ? 0 : m->f;
    if (sx >= img->w || sy >= img->h)
        return;
    int sw = part.w > 0 && sx + part.w <= img->w ? part.w : img->w - sx;
    int sh = part.h > 0 && sy + part.h <= img->h ? part.h : img->h - sy;
    int dw = m->c > 0 ? m->c : sw, dh = m->d > 0 ? m->d : sh;
    if (m->flags & 1)
        nine(&s, m->a, m->b, dw, dh, img, sx, sy, sw, sh, part.border);
    else
        blit(&s, m->a, m->b, dw, dh, img, sx, sy, sw, sh);
}

/* Text in pixels: the 8x16 font, or (small) the 3x5 one of big(), each
 * pixel scale x scale. bg TD_PYUI_CLEAR leaves what is behind the text. */
static void pixel_text(surf_t *s, int x0, int y0, const char *text, int fg, int bg, int scale, bool small)
{
    scale = scale < 1 ? 1 : scale > 8 ? 8
                                      : scale;
    int x = x0, y = y0;
    while (*text)
    {
        uint32_t cp = td_utf8_next(&text);
        if (cp == '\n')
        {
            x = x0;
            y += (small ? 6 : 16) * scale;
            continue;
        }
        int gw, gh = small ? 5 : 16;
        const uint8_t *rows = NULL;
        const big_glyph_t *g = NULL;
        if (small)
        {
            g = big_glyph(cp);
            gw = (int)strlen(g->rows[0]);
        }
        else
        {
            rows = td_pyui_glyph(cp);
            if (!rows)
                rows = td_pyui_glyph('?');
            gw = 8;
        }
        for (int r = 0; r < gh; r++)
        {
            for (int c = 0; c < gw; c++)
            {
                bool on = small ? g->rows[r][c] == '#' : (rows[r] >> (7 - c)) & 1;
                if (!on && bg == TD_PYUI_CLEAR)
                    continue;
                fill_rect(s, x + c * scale, y + r * scale, scale, scale, on ? fg : bg);
            }
        }
        x += (small ? gw + 1 : gw) * scale;
    }
}

/* One pixel request on a canvas or an image. */
static void pixel_request(obj_t *o, const td_pyui_msg_t *m, const char *text)
{
    surf_t s;
    switch (m->op)
    {
    case TD_PYUI_DRAW_IMAGE:
        draw_image(o, m, text);
        break;
    case TD_PYUI_IMAGE_DATA:
    {
        td_pyui_image_t *img = msg_image(m, text);
        if (img && surface(o, &s))
            blit(&s, m->a, m->b, img->w, img->h, img, 0, 0, img->w, img->h);
        free(img);
        break;
    }
    default:
        if (!surface(o, &s))
            return;
        if (m->op != TD_PYUI_GRADIENT && m->op != TD_PYUI_BAR && (m->op == TD_PYUI_PTEXT ? m->c == TD_PYUI_CLEAR : m->e == TD_PYUI_CLEAR) &&
            !image_mask(o, &s))
            return;
        if ((m->op == TD_PYUI_RECT || m->op == TD_PYUI_CIRCLE) && (m->flags & 2) && m->len == 7)
            gradient_shape(&s, m, (const uint8_t *)text, m->op == TD_PYUI_CIRCLE);
        else if (m->op == TD_PYUI_RECT && m->f > 0)
            round_rect(&s, m->a, m->b, m->c, m->d, m->f, m->e, m->flags & 1);
        else if (m->op == TD_PYUI_RECT && (m->flags & 1) && m->c > 0 && m->d > 0)
        {
            int x1 = m->a + m->c - 1, y1 = m->b + m->d - 1;
            line(&s, m->a, m->b, x1, m->b, m->e);
            line(&s, m->a, y1, x1, y1, m->e);
            line(&s, m->a, m->b, m->a, y1, m->e);
            line(&s, x1, m->b, x1, y1, m->e);
        }
        else if (m->op == TD_PYUI_RECT)
            fill_rect(&s, m->a, m->b, m->c, m->d, m->e);
        else if (m->op == TD_PYUI_LINE)
            line(&s, m->a, m->b, m->c, m->d, m->e);
        else if (m->op == TD_PYUI_CIRCLE)
            circle(&s, m->a, m->b, m->c, m->e, m->flags & 1);
        else if (m->op == TD_PYUI_GRADIENT && m->len >= 6)
            gradient(&s, m->a, m->b, m->c, m->d, (const uint8_t *)text, m->flags & 1);
        else if (m->op == TD_PYUI_PTEXT)
            pixel_text(&s, m->a, m->b, text, m->c, m->d, m->e, m->flags & 1);
        else if (m->op == TD_PYUI_BAR)
            pixel_bar(&s, m, text);
        break;
    }
    if (o->kind == K_CANVAS && o->draw == o->show)
        td_win_invalidate(NULL);
    if (o->kind == K_IMAGE)
    {
        for (int i = 0; i < MAX_PICS; i++)
            if (s_link->pics[i].used && s_link->pics[i].img == o->id)
            {
                s_link->pics[i].serial = ++s_link->pic_serial;
                s_link->pics[i].cache.generation = 0;
                s_link->pics[i].warm_attempted = false;
            }
        td_win_invalidate(NULL);
    }
}

/* An image the script made: the desktop keeps it under its id. */
static void add_image(const td_pyui_msg_t *m, const char *text)
{
    td_pyui_image_t *img = msg_image(m, text);
    if (!img)
        return;
    obj_t *o = new_obj(K_IMAGE, m->id);
    if (!o)
    {
        td_pyui_pixels_give(td_pyui_image_bytes(img));
        free(img);
        return;
    }
    o->img = img;
}

static void add_font(const td_pyui_msg_t *m, const char *text)
{
    td_pyui_font_t *font = NULL;
    if (m->len == sizeof(font))
        memcpy(&font, text, sizeof(font));
    if (!font)
        return;
    obj_t *o = new_obj(K_FONT, m->id);
    if (!o)
    {
        td_pyui_pixels_give((long)font->bytes);
        free(font);
        return;
    }
    o->font = font;
}

static pic_text_t *picture_text(const char *text)
{
    if (!*text)
        return NULL;
    size_t bytes = sizeof(pic_text_t) + strlen(text) + 1;
    if (!td_pyui_pixels_take((long)bytes))
        return NULL;
    pic_text_t *copy = malloc(bytes);
    if (!copy)
    {
        td_pyui_pixels_give((long)bytes);
        return NULL;
    }
    copy->refs = 1;
    copy->bytes = bytes;
    memcpy(copy->text, text, strlen(text) + 1);
    return copy;
}

/* The four pixels of cell (x, y) of a canvas page: upper left, upper
 * right, lower left, lower right. */
static void cell_pixels(const obj_t *o, int page, int x, int y, uint8_t q[4])
{
    const uint8_t *p = canvas_layer(o, page) + (2 * y) * (2 * o->rect.w) + 2 * x;
    q[0] = p[0];
    q[1] = p[1];
    q[2] = p[2 * o->rect.w];
    q[3] = p[2 * o->rect.w + 1];
}

/* The colour most of four pixels have (the first of a tie), for text that
 * stands on them. */
static uint8_t main_colour(const uint8_t q[4])
{
    int best = 0, best_n = 0;
    for (int i = 0; i < 4; i++)
    {
        int n = 0;
        for (int j = 0; j < 4; j++)
            n += q[j] == q[i];
        if (n > best_n)
        {
            best = i;
            best_n = n;
        }
    }
    return q[best];
}

/* ---------------------------------------------------------- pictures */

/* A picture on a canvas page (TD_PYUI_PICTURE): drawn by terminals that
 * can at their own resolution (td_picture.h), and into the canvas's
 * pixels for the others. Anything drawn over its cells removes it. */
static pic_t *pictures(void)
{
    return s_link->pics;
}

static void drop_picture(pic_t *p)
{
    release_picture(p);
    td_win_invalidate(NULL);
}

/* Forget the pictures of a canvas page that overlap cells x0..x1, y0..y1
 * (exclusive ends); page -1: every page; o NULL: those of image img. */
static void cut_picture_region(const obj_t *o, int page, int x0, int y0, int x1, int y1, bool overlay)
{
    for (int i = 0; i < MAX_PICS; i++)
    {
        pic_t *p = &pictures()[i];
        if (!p->used || p->canvas != o->id || (page >= 0 && p->page != page))
            continue;
        if (p->x < x1 && x0 < p->x + p->w && p->y < y1 && y0 < p->y + p->h)
        {
            /* A native image can cover only part of a panel's corner/edge.
             * Retain that unlabelled background behind it; clearing/drawing
             * primitives and replacing a complete rectangle still erase it. */
            if (overlay && !p->text && !(x0 <= p->x && y0 <= p->y && x1 >= p->x + p->w && y1 >= p->y + p->h))
                continue;
            drop_picture(p);
        }
    }
}

static void cut_pictures(const obj_t *o, int page, int x0, int y0, int x1, int y1)
{
    cut_picture_region(o, page, x0, y0, x1, y1, false);
}

static void drop_image_pictures(uint16_t img)
{
    for (int i = 0; i < MAX_PICS; i++)
        if (pictures()[i].used && pictures()[i].img == img)
            drop_picture(&pictures()[i]);
}

/* The part of the image a picture shows, clipped to it. */
static void picture_part(const pic_t *p, const td_pyui_image_t *img, int *sx, int *sy, int *sw, int *sh)
{
    *sx = p->sx < 0 ? 0 : p->sx >= img->w ? img->w - 1
                                          : p->sx;
    *sy = p->sy < 0 ? 0 : p->sy >= img->h ? img->h - 1
                                          : p->sy;
    *sw = p->part.w > 0 && *sx + p->part.w <= img->w ? p->part.w : img->w - *sx;
    *sh = p->part.h > 0 && *sy + p->part.h <= img->h ? p->part.h : img->h - *sy;
}

/* The label's size: as asked, or the largest that fits well. */
static int label_scale(const pic_t *p, int w, int h)
{
    int n = td_utf8_len(p->text ? p->text->text : "");
    if (p->part.scale > 0)
        return p->part.scale;
    int s = 1;
    while (16 * (s + 1) * 10 <= h * 6 && 8 * (s + 1) * n * 10 <= w * 9 && s < 8)
        s++;
    return s;
}

/* td_picture_render_fn: the canvas under the picture, the image over it,
 * then the label, all at the terminal's pixels. */
static void render_picture(void *user, uint8_t *px, int w, int h)
{
    const pic_t *p = user;
    const obj_t *co = find(p->canvas), *io = find(p->img);
    uint8_t bg = p->font && !p->img && p->font_bg >= 0 ? (uint8_t)p->font_bg : td_theme()->win_bg;
    bool opaque = io && io->kind == K_IMAGE && !io->img->mask;
    bool solid = p->font && !p->img && p->font_bg != TD_PYUI_CLEAR;
    if (solid)
        memset(px, bg, (size_t)w * h);
    for (int y = 0; !opaque && !solid && y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            uint8_t c = bg;
            int cx = 2 * p->x + x * 2 * p->w / w, cy = 2 * p->y + y * 2 * p->h / h;
            if (p->font && !p->img && p->font_bg != TD_PYUI_CLEAR)
                c = bg;
            else if (p->bg && cx >= p->bg->x && cy >= p->bg->y &&
                     cx < p->bg->x + p->bg->w && cy < p->bg->y + p->bg->h)
                c = p->bg->px[(cy - p->bg->y) * p->bg->w + cx - p->bg->x];
            else if (co && co->px && cx >= 0 && cy >= 0 && cx < 2 * co->rect.w && cy < 2 * co->rect.h)
            {
                c = canvas_layer(co, co->show)[cy * 2 * co->rect.w + cx];
            }
            px[y * w + x] = c;
        }
    }
    surf_t s = {px, NULL, w, h, NULL, 0, NULL};
    if (io && io->kind == K_IMAGE)
    {
        int sx, sy, sw, sh;
        picture_part(p, io->img, &sx, &sy, &sw, &sh);
        if (p->nine)
            nine(&s, 0, 0, w, h, io->img, sx, sy, sw, sh, p->part.border);
        else
            blit(&s, 0, 0, w, h, io->img, sx, sy, sw, sh);
    }
    if (p->text && p->text->text[0])
    {
        const obj_t *fo = find(p->font);
        if (fo && fo->kind == K_FONT)
            td_font_draw(&fo->font->face, p->text->text, px, w, h, (uint8_t)p->part.fg,
                         p->font_flags & 3, (p->font_flags >> 2) & 3, (p->font_flags & 16) != 0);
        else
        {
            int sc = label_scale(p, w, h);
            int tw = 8 * sc * td_utf8_len(p->text->text), th = 16 * sc;
            pixel_text(&s, (w - tw) / 2, (h - th) / 2, p->text->text, p->part.fg, TD_PYUI_CLEAR, sc, false);
        }
    }
}

static void paint_picture_rgb(void *user, uint16_t *px, int w, int h)
{
    const pic_t *p = user;
    const obj_t *co = find(p->canvas), *io = find(p->img);
    uint16_t bg = td_palette_rgb565(p->font && !p->img && p->font_bg >= 0 ? (uint8_t)p->font_bg : td_theme()->win_bg);
    int previous = -1;
    for (int y = 0; y < h; y++)
    {
        int cy = 2 * p->y + y * 2 * p->h / h;
        if (cy == previous)
        {
            memcpy(px + (size_t)y * w, px + (size_t)(y - 1) * w, (size_t)w * sizeof(*px));
            continue;
        }
        previous = cy;
        for (int x = 0; x < w; x++)
        {
            int cx = 2 * p->x + x * 2 * p->w / w, cy = 2 * p->y + y * 2 * p->h / h;
            uint16_t c = bg;
            if (p->font && !p->img && p->font_bg != TD_PYUI_CLEAR)
                c = bg;
            else if (p->bg && cx >= p->bg->x && cy >= p->bg->y && cx < p->bg->x + p->bg->w && cy < p->bg->y + p->bg->h)
            {
                size_t at = (size_t)(cy - p->bg->y) * p->bg->w + cx - p->bg->x;
                c = p->bg->rgb ? p->bg->rgb[at] : td_palette_rgb565(p->bg->px[at]);
            }
            else if (co && co->px && cx >= 0 && cy >= 0 && cx < 2 * co->rect.w && cy < 2 * co->rect.h)
            {
                size_t at = (size_t)cy * 2 * co->rect.w + cx;
                c = co->rgb ? canvas_rgb(co, p->page)[at] : td_palette_rgb565(canvas_layer(co, p->page)[at]);
            }
            px[(size_t)y * w + x] = c;
        }
    }
    surf_t s = {NULL, NULL, w, h, NULL, 0, px};
    if (io && io->kind == K_IMAGE)
    {
        int sx, sy, sw, sh;
        picture_part(p, io->img, &sx, &sy, &sw, &sh);
        if (p->nine)
            nine(&s, 0, 0, w, h, io->img, sx, sy, sw, sh, p->part.border);
        else
            blit(&s, 0, 0, w, h, io->img, sx, sy, sw, sh);
    }
    if (p->text && p->text->text[0])
    {
        const obj_t *fo = find(p->font);
        if (fo && fo->kind == K_FONT)
            td_font_draw_rgb(&fo->font->face, p->text->text, px, w, h, (uint8_t)p->part.fg, p->font_flags & 3, (p->font_flags >> 2) & 3, (p->font_flags & 16) != 0);
        else
        {
            int sc = label_scale(p, w, h), tw = 8 * sc * td_utf8_len(p->text->text);
            pixel_text(&s, (w - tw) / 2, (h - 16 * sc) / 2, p->text->text, p->part.fg, TD_PYUI_CLEAR, sc, false);
        }
    }
}

static bool ensure_render_cache(void)
{
    if (td_pcache_ready())
        return true;
    if (!s_link->cache_limit || !td_pyui_rgb_enabled())
        return false;
    const td_sysinfo_t *si = td_sysinfo();
    if (si && si->psram_free && si->psram_free() < 786432)
        return false;
    td_pcache_config(s_link->cache_limit);
    return td_pcache_ready();
}

static void render_picture_rgb(void *user, uint16_t *px, int w, int h)
{
    pic_t *p = user;
    uint16_t theme = (uint16_t)((uint16_t)td_theme()->win_bg << 8 | td_theme()->win_fg);
    if (p->cache_theme != theme)
    {
        p->cache.generation = 0;
        p->cache_theme = theme;
    }
    uint32_t start = td_millis();
    if (ensure_render_cache() && td_pcache_load(p->cache, px, w, h))
    {
        atomic_fetch_add(&s_decode_ms, td_millis() - start);
        return;
    }
    paint_picture_rgb(user, px, w, h);
    atomic_fetch_add(&s_raster_ms, td_millis() - start);
    const obj_t *io = find(p->img);
    if (p->bg || (io && io->img && !io->img->mask) || (p->font && p->font_bg != TD_PYUI_CLEAR))
        td_pcache_store(&p->cache, px, w, h);
    p->warm_attempted = true;
}

static bool cached_picture_runs(void *user, int w, int h, const uint16_t **runs, size_t *words)
{
    pic_t *p = user;
    uint16_t theme = (uint16_t)((uint16_t)td_theme()->win_bg << 8 | td_theme()->win_fg);
    return p->cache_theme == theme && td_pcache_view(p->cache, w, h, runs, words);
}

static void warm_render_cache(void)
{
    if (!s_link->cache_warm || !td_pcache_ready() || s_link->batch_depth || ring_used(&s_link->req) || td_wm_needs_redraw())
        return;
    int cw, ch;
    td_term_cell_px(&cw, &ch);
    if (cw < 1 || ch < 1)
        return;
    for (int n = 0; n < MAX_PICS; n++)
    {
        pic_t *p = &s_link->pics[s_link->warm_cursor++ % MAX_PICS];
        if (!p->used || p->warm_attempted)
            continue;
        obj_t *co = find(p->canvas), *wo = co ? find(co->win) : NULL;
        if (!co || co->show == p->page || !wo || wo->w != td_win_focused())
            continue;
        p->warm_attempted = true;
        size_t bytes = (size_t)p->w * cw * p->h * ch * sizeof(uint16_t);
        if (bytes > 163840)
            continue;
        uint16_t *rgb = malloc(bytes);
        if (!rgb)
            return;
        render_picture_rgb(p, rgb, p->w * cw, p->h * ch);
        free(rgb);
        td_pcache_warmed();
        return;
    }
}

static void cover_picture(const obj_t *o, const pic_t *p, int row, uint8_t covered[TD_MAX_COLS])
{
    if (!p->used || p->canvas != o->id || p->page != o->show || p->y >= row + 2 || p->y + p->h <= row)
        return;
    if (p->x < 0 || p->y < 0 || p->x + p->w > o->rect.w || p->y + p->h > o->rect.h)
        return;
    unsigned bits = (p->y <= row && p->y + p->h > row ? 1u : 0u) | (p->y <= row + 1 && p->y + p->h > row + 1 ? 2u : 0u);
    int end = p->x + p->w;
    if (end > o->rect.w)
        end = o->rect.w;
    if (end > TD_MAX_COLS)
        end = TD_MAX_COLS;
    for (int x = p->x < 0 ? 0 : p->x; x < end; x++)
        covered[x] |= (uint8_t)bits;
}
static void canvas_picture_coverage(const obj_t *o, int row, uint8_t covered[TD_MAX_COLS])
{
    memset(covered, 0, TD_MAX_COLS);
    for (int i = 0; i < MAX_PICS; i++)
        cover_picture(o, &s_link->pics[i], row, covered);
}
static void ordered_coverage(const obj_t *o, pic_t *const *pics, int count, int row, uint8_t covered[TD_MAX_COLS])
{
    memset(covered, 0, TD_MAX_COLS);
    for (int i = 0; i < count; i++)
        cover_picture(o, pics[i], row, covered);
}

static void render_canvas_rgb_region(void *user, uint16_t *px, int w, int h, bool check_coverage)
{
    uintptr_t token = (uintptr_t)user;
    const obj_t *o = find((uint16_t)token);
    int row = (int)((token >> 16) & 255u), col = (int)((token >> 24) & 255u), cw, ch;
    td_term_cell_px(&cw, &ch);
    const uint16_t *rgb = canvas_rgb(o, o->show);
    const cell_t *cells = canvas_page(o, o->show);
    int previous = -1;
    for (int y = 0; y < h; y++)
    {
        int cy = 2 * row + y * 2 / ch;
        uint16_t *line = px + (size_t)y * w;
        if (cy == previous)
        {
            memcpy(line, line - w, (size_t)w * sizeof(*line));
            continue;
        }
        previous = cy;
        for (int cx = 2 * col; cx < 2 * col + 2 * w / cw; cx++)
        {
            const cell_t *c = &cells[(cy / 2) * o->rect.w + cx / 2];
            uint16_t color = c->ch != PX_CELL && c->bg != TD_PYUI_CLEAR ? td_palette_rgb565(cell_bg(c)) : rgb[cy * 2 * o->rect.w + cx];
            int end = (cx - 2 * col + 1) * cw / 2;
            for (int x = (cx - 2 * col) * cw / 2; x < end; x++)
                line[x] = color;
        }
    }
    surf_t s = {NULL, NULL, w, h, NULL, 0, px};
    uint8_t covered[TD_MAX_COLS] = {0};
    if (check_coverage)
        canvas_picture_coverage(o, row, covered);
    for (int y = row; y < o->rect.h && y < row + h / ch; y++)
        for (int x = col; x < col + w / cw; x++)
        {
            if (x < TD_MAX_COLS && (covered[x] & (1u << (y - row))))
                continue;
            const cell_t *c = &cells[y * o->rect.w + x];
            if (c->ch && c->ch != ' ' && c->ch != PX_CELL)
            {
                uint8_t text[5] = {0};
                td_utf8_encode(c->ch, text);
                pixel_text(&s, (x - col) * cw + (cw - 8) / 2, (y - row) * ch + (ch - 16) / 2, (const char *)text, c->fg < 0 ? td_theme()->win_fg : c->fg, TD_PYUI_CLEAR, 1, false);
            }
        }
}
static void render_canvas_rgb(void *user, uint16_t *px, int w, int h)
{
    render_canvas_rgb_region(user, px, w, h, true);
}
static void render_canvas_sparse_rgb(void *user, uint16_t *px, int w, int h)
{
    render_canvas_rgb_region(user, px, w, h, false);
}

static void render_canvas_fallback(void *user, uint8_t *px, int w, int h)
{
    uintptr_t token = (uintptr_t)user;
    const obj_t *o = find((uint16_t)token);
    int row = (int)((token >> 16) & 255u), col = (int)((token >> 24) & 255u), cw, ch;
    td_term_cell_px(&cw, &ch);
    const uint8_t *src = canvas_layer(o, o->show);
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            px[y * w + x] = src[(2 * row + y * 2 / ch) * 2 * o->rect.w + 2 * col + x * 2 / cw];
}

/* A picture in a canvas's pixels (dx, dy, dw x dh of them): drawn at FB
 * pixels to each canvas pixel (as a screen would), then each block averaged
 * to the nearest palette colour, so curves and gradients come out clean
 * and dithering does not speckle. Mostly transparent blocks leave the
 * canvas as it is. border < 0: stretched, not nine-slice. */
#define FB 8
static void picture_fallback(surf_t *d, int dx, int dy, int dw, int dh, const td_pyui_image_t *img, int sx, int sy,
                             int sw, int sh, int border)
{
    int w = dw * FB, h = dh * FB;
    /* Downsample one logical row at a time. A full temporary enlarged
     * image can otherwise exceed the remaining PSRAM during a page change. */
    size_t count = (size_t)w * FB;
    uint8_t *allocation = malloc(count * (d->rgb ? 4 : 2));
    if (!allocation)
        return;
    uint8_t *px = allocation + (d->rgb ? count * 2 : 0);
    uint8_t *mask = px + count;
    surf_t s = {px, mask, w, FB, NULL, 0, d->rgb ? (uint16_t *)allocation : NULL};
    for (int ty = 0; ty < dh; ty++)
    {
        memset(mask, 1, count);
        if (border >= 0)
            nine(&s, 0, -ty * FB, w, h, img, sx, sy, sw, sh, border);
        else
            blit(&s, 0, -ty * FB, w, h, img, sx, sy, sw, sh);
        for (int tx = 0; tx < dw; tx++)
        {
            int sum[3] = {0, 0, 0}, n = 0;
            for (int y = 0; y < FB; y++)
                for (int x = tx * FB; x < (tx + 1) * FB; x++)
                {
                    size_t i = (size_t)y * (size_t)w + (size_t)x;
                    if (mask[i])
                        continue;
                    uint8_t rgb[3];
                    if (s.rgb)
                        td_rgb565_rgb(s.rgb[i], rgb);
                    else
                        td_palette_rgb(px[i], rgb);
                    for (int k = 0; k < 3; k++)
                        sum[k] += rgb[k];
                    n++;
                }
            if (n * 2 >= FB * FB)
            {
                plot(d, dx + tx, dy + ty, td_rgb256((uint8_t)(sum[0] / n), (uint8_t)(sum[1] / n), (uint8_t)(sum[2] / n)));
                if (d->rgb && dx + tx >= 0 && dy + ty >= 0 && dx + tx < d->w && dy + ty < d->h)
                    d->rgb[(dy + ty) * d->w + dx + tx] = td_rgb565((uint8_t)(sum[0] / n), (uint8_t)(sum[1] / n), (uint8_t)(sum[2] / n));
            }
        }
    }
    free(allocation);
}

/* Terminal fallback stays inside the picture. Native pictures cover only
 * that rectangle, so an unbounded label leaves stray characters beside it. */
static void picture_label(obj_t *o, int x, int y, int w, int h, const char *text, int fg, int align, bool wrap)
{
    int row = 0;
    while (*text && row < h)
    {
        const char *end = text;
        int count = 0;
        while (*end && *end != '\n')
        {
            td_utf8_next(&end);
            count++;
        }
        if (!count)
            row++;
        while (count && row < h)
        {
            int n = count < w ? count : w;
            int col = align == 1 ? (w - n) / 2 : align == 2 ? w - n
                                                            : 0;
            for (int i = 0; i < n; i++)
                canvas_set(o, x + col + i, y + row, td_utf8_next(&text), fg, TD_PYUI_CLEAR);
            row++;
            count -= n;
            if (!wrap)
                break;
        }
        text = *end ? end + 1 : end;
    }
}

static void picture_request(obj_t *o, const td_pyui_msg_t *m, const char *text)
{
    bool text_only = m->op == TD_PYUI_FONT_TEXT;
    obj_t *io = find(m->win);
    if (!io || io->kind != (text_only ? K_FONT : K_IMAGE))
    {
        post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "no such image");
        return;
    }
    if (o->kind != K_CANVAS || m->c <= 0 || m->d <= 0)
        return;
    td_pyui_picture_t part = {0, 0, 0, 231, 0};
    const char *label = text_only ? text : "";
    uint16_t font = text_only ? m->win : 0;
    if (text_only)
        part.fg = m->e;
    else if (m->len >= sizeof(part))
    {
        memcpy(&part, text, sizeof(part));
        label = text + sizeof(part);
        if (m->flags & 2)
        {
            if (m->len < sizeof(part) + 2)
                return;
            font = (uint16_t)((uint8_t)label[0] | (uint16_t)(uint8_t)label[1] << 8);
            label += 2;
            obj_t *fo = find(font);
            if (!fo || fo->kind != K_FONT)
            {
                post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "no such font");
                return;
            }
        }
    }
    surf_t s;
    if (!surface(o, &s))
        return;
    pic_bg_t *bg = NULL;
    /* The low-resolution fallback already contains the sprite. Sampling
     * it for transparent Sixel corners gives a blocky halo. Keep only the
     * small underlying canvas region, and reuse it when a button redraws. */
    if ((!text_only && io->img->mask) || (text_only && m->f == TD_PYUI_CLEAR))
    {
        for (int i = 0; i < MAX_PICS; i++)
        {
            pic_t *p = &pictures()[i];
            if (p->used && p->canvas == o->id && p->page == o->draw && p->bg &&
                p->x == m->a && p->y == m->b && p->w == m->c && p->h == m->d)
            {
                bg = p->bg;
                bg->refs++;
                break;
            }
        }
        if (!bg)
        {
            int x0 = m->a > 0 ? 2 * m->a : 0, y0 = m->b > 0 ? 2 * m->b : 0;
            int x1 = 2 * (m->a + m->c), y1 = 2 * (m->b + m->d);
            if (x1 > s.w)
                x1 = s.w;
            if (y1 > s.h)
                y1 = s.h;
            if (x1 <= x0 || y1 <= y0)
                return;
            long n = (long)(x1 - x0) * (y1 - y0);
            long bytes = s.rgb ? ((n + 1) & ~1L) + 2 * n : n;
            if (!td_pyui_pixels_take(bytes))
            {
                post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, PIXELS_MSG);
                return;
            }
            bg = malloc(sizeof(*bg) + (size_t)bytes);
            if (!bg)
            {
                td_pyui_pixels_give(bytes);
                post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "picture background: out of memory");
                return;
            }
            *bg = (pic_bg_t){.refs = 1, .x = x0, .y = y0, .w = x1 - x0, .h = y1 - y0, .bytes = (size_t)bytes};
            bg->rgb = s.rgb ? (uint16_t *)(bg->px + ((n + 1) & ~1L)) : NULL;
            for (int y = 0; y < bg->h; y++)
                for (int x = 0; x < bg->w; x++)
                {
                    const cell_t *cell = &s.cells[((y0 + y) / 2) * s.cols + (x0 + x) / 2];
                    bg->px[y * bg->w + x] = cell->ch == PX_CELL || cell->bg == TD_PYUI_CLEAR
                                                ? s.px[(y0 + y) * s.w + x0 + x]
                                                : cell_bg(cell);
                    if (bg->rgb)
                        bg->rgb[y * bg->w + x] = cell->ch == PX_CELL || cell->bg == TD_PYUI_CLEAR ? s.rgb[(y0 + y) * s.w + x0 + x] : td_palette_rgb565(cell_bg(cell));
                }
        }
    }
    pic_text_t *text_copy = picture_text(label);
    if (*label && !text_copy)
    {
        if (bg)
        {
            pic_t unused = {.bg = bg};
            release_picture(&unused);
        }
        post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "font label: out of memory");
        return;
    }
    cut_picture_region(o, o->draw, m->a, m->b, m->a + m->c, m->b + m->d, o->rgb != NULL);

    /* For terminals without pictures: the same in the canvas's pixels, its
     * border scaled down as the picture is (a cell is 2 x 2 pixels here). */
    pic_t tmp = {.sx = m->e, .sy = m->f, .part = part};
    int sx, sy, sw, sh;
    if (!text_only)
    {
        picture_part(&tmp, io->img, &sx, &sy, &sw, &sh);
        picture_fallback(&s, 2 * m->a, 2 * m->b, 2 * m->c, 2 * m->d, io->img, sx, sy, sw, sh,
                         (m->flags & 1) ? part.border : -1);
    }
    else if (m->f != TD_PYUI_CLEAR)
        round_rect(&s, 2 * m->a, 2 * m->b, 2 * m->c, 2 * m->d, 0, m->f < 0 ? td_theme()->win_bg : m->f, false);
    if (label[0])
    {
        int row = text_only ? 0 : m->d / 2;
        picture_label(o, m->a, m->b + row, m->c, m->d - row, label, part.fg,
                      text_only ? m->flags & 3 : 1, text_only && (m->flags & 16));
    }

    for (int i = 0; i < MAX_PICS; i++)
    {
        pic_t *p = &pictures()[i];
        if (p->used)
            continue;
        *p = (pic_t){.used = true,
                     .canvas = o->id,
                     .img = text_only ? 0 : m->win,
                     .font = font,
                     .font_bg = text_only ? m->f : TD_PYUI_CLEAR,
                     .font_flags = text_only ? m->flags : 5,
                     .page = o->draw,
                     .nine = (m->flags & 1) != 0,
                     .x = m->a,
                     .y = m->b,
                     .w = m->c,
                     .h = m->d,
                     .sx = m->e,
                     .sy = m->f,
                     .part = part,
                     .bg = bg,
                     .serial = ++s_link->pic_serial};
        bg = NULL;
        p->text = text_copy;
        text_copy = NULL;
        break;
    }
    if (bg)
    {
        pic_t unused = {.bg = bg};
        release_picture(&unused);
    }
    if (text_copy)
    {
        td_pyui_pixels_give((long)text_copy->bytes);
        free(text_copy);
    }
    if (o->draw == o->show)
        td_win_invalidate(NULL);
}

/* Page src's pictures onto page dst (whose own are dropped). */
static void copy_pictures(obj_t *o, int src, int dst)
{
    cut_pictures(o, dst, -32768, -32768, 32767, 32767);
    for (int i = 0; i < MAX_PICS; i++)
    {
        pic_t *p = &pictures()[i];
        if (!p->used || p->canvas != o->id || p->page != src)
            continue;
        for (int j = 0; j < MAX_PICS; j++)
        {
            pic_t *q = &pictures()[j];
            if (q->used)
                continue;
            *q = *p;
            if (q->bg)
                q->bg->refs++;
            if (q->text)
                q->text->refs++;
            q->page = (uint8_t)dst;
            q->serial = ++s_link->pic_serial;
            break;
        }
    }
}

static void canvas_backdrop(const obj_t *o, td_rect_t client, int col, int row, int cols, int rows, bool sparse)
{
    uint32_t id = 2166136261u ^ ((uint32_t)o->id << 16) ^ ((uint32_t)o->show << 8) ^ (uint32_t)row ^ ((uint32_t)col << 24);
    const uint16_t *rgb = canvas_rgb(o, o->show);
    const cell_t *cells = canvas_page(o, o->show);
    for (int y = 2 * row; y < 2 * (row + rows); y++)
        for (int x = 2 * col; x < 2 * (col + cols); x++)
            id = (id ^ rgb[y * 2 * o->rect.w + x]) * 16777619u;
    for (int y = row; y < row + rows; y++)
        for (int x = col; x < col + cols; x++)
        {
            const cell_t *c = &cells[y * o->rect.w + x];
            id = (id ^ c->ch) * 16777619u;
            id = (id ^ (uint16_t)c->fg) * 16777619u;
            id = (id ^ (uint16_t)c->bg) * 16777619u;
        }
    if (!sparse)
    {
        uint8_t covered[TD_MAX_COLS];
        canvas_picture_coverage(o, row, covered);
        for (int x = col; x < col + cols && x < TD_MAX_COLS; x++)
            id = (id ^ covered[x]) * 16777619u;
    }
    void *token = (void *)((uintptr_t)o->id | ((uintptr_t)row << 16) | ((uintptr_t)col << 24));
    td_picture_add_rgb(client.x + o->rect.x + col, client.y + o->rect.y + row, cols, rows, id, render_canvas_fallback, sparse ? render_canvas_sparse_rgb : render_canvas_rgb, token);
}

/* Keep the canvas fallback underneath while the terminal receives Sixel.
 * Clearing it to black first exposes a black rectangle on streaming LCDs. */
static void show_pictures(const obj_t *o, td_rect_t client)
{
    if (!td_term_images())
        return;
    pic_t *ordered[MAX_PICS];
    int count = 0;
    for (int i = 0; i < MAX_PICS; i++)
    {
        pic_t *p = &pictures()[i];
        if (!p->used || p->canvas != o->id || p->page != o->show)
            continue;
        int at = count;
        while (at > 0 && ordered[at - 1]->serial > p->serial)
        {
            ordered[at] = ordered[at - 1];
            at--;
        }
        ordered[at] = p;
        count++;
    }
    /* Local pictures already contain their exact underlay. Avoid expanding
     * and copying a whole canvas only to overwrite most of it with panels.
     * Preflight every span against the native-picture capacity; when it
     * cannot fit, retain the bounded two-row background path. */
    bool sparse = o->rgb && td_picture_local_rgb() && o->rect.w <= 255 && o->rect.h <= 255 &&
                  client.x + o->rect.x >= 0 && client.y + o->rect.y >= 0 &&
                  client.x + o->rect.x + o->rect.w <= td_stats()->cols && client.y + o->rect.y + o->rect.h <= td_stats()->rows;
    if (sparse)
    {
        int spans = 0, uncovered_cells = 0;
        for (int row = 0; row < o->rect.h; row++)
        {
            uint8_t covered[TD_MAX_COLS];
            ordered_coverage(o, ordered, count, row, covered);
            bool was = false;
            for (int x = 0; x < o->rect.w; x++)
            {
                bool uncovered = !(covered[x] & 1u);
                if (uncovered)
                    uncovered_cells++;
                if (uncovered && !was)
                    spans++;
                was = uncovered;
            }
        }
        if (spans + count > td_pictures_available() || uncovered_cells * 4 > o->rect.w * o->rect.h * 3)
            sparse = false;
    }
    if (sparse)
    {
        for (int row = 0; row < o->rect.h; row++)
        {
            uint8_t covered[TD_MAX_COLS];
            ordered_coverage(o, ordered, count, row, covered);
            for (int col = 0; col < o->rect.w;)
            {
                while (col < o->rect.w && (covered[col] & 1u))
                    col++;
                int begin = col;
                while (col < o->rect.w && !(covered[col] & 1u))
                    col++;
                if (col > begin)
                    canvas_backdrop(o, client, begin, row, col - begin, 1, true);
            }
        }
    }
    else if (o->rgb)
        for (int row = 0; row < o->rect.h; row += 2)
            canvas_backdrop(o, client, 0, row, o->rect.w, o->rect.h - row < 2 ? o->rect.h - row : 2, false);
    for (int i = 0; i < count; i++)
    {
        pic_t *p = ordered[i];
        if (o->rgb)
            td_picture_add_rgb_runs(client.x + o->rect.x + p->x, client.y + o->rect.y + p->y, p->w, p->h, p->serial, render_picture, render_picture_rgb, cached_picture_runs, p);
        else
            td_picture_add(client.x + o->rect.x + p->x, client.y + o->rect.y + p->y, p->w, p->h, p->serial, render_picture, p);
    }
}

/* The cells a request draws on (for cutting pictures away); false when it
 * draws on none of them, or it is not known. */
static bool request_cells(const obj_t *o, const td_pyui_msg_t *m, const char *text, int r[4])
{
    int x0, y0, x1, y1;   /* pixels, ends exclusive */
    switch (m->op)
    {
    case TD_PYUI_PUT:
    {
        int n = td_utf8_len(text), lines = 1;
        for (const char *c = text; *c; c++)
            lines += *c == '\n';
        r[0] = m->a, r[1] = m->b, r[2] = lines > 1 ? o->rect.w : m->a + n, r[3] = m->b + lines;
        return true;
    }
    case TD_PYUI_FILL:
        r[0] = m->a, r[1] = m->b, r[2] = m->a + m->c, r[3] = m->b + m->d;
        return true;
    case TD_PYUI_BIG:
    {
        int scale = m->e < 1 ? 1 : m->e, n = td_utf8_len(text);
        n = m->f > n ? m->f : n;
        r[0] = m->a, r[1] = m->b, r[2] = m->a + n * 4 * scale, r[3] = m->b + (5 * scale + 1) / 2;
        return true;
    }
    case TD_PYUI_RECT:
    case TD_PYUI_GRADIENT:
    case TD_PYUI_BAR:
        x0 = m->a, y0 = m->b, x1 = m->a + m->c, y1 = m->b + m->d;
        break;
    case TD_PYUI_LINE:
        x0 = m->a < m->c ? m->a : m->c, x1 = (m->a > m->c ? m->a : m->c) + 1;
        y0 = m->b < m->d ? m->b : m->d, y1 = (m->b > m->d ? m->b : m->d) + 1;
        break;
    case TD_PYUI_CIRCLE:
        x0 = m->a - m->c, y0 = m->b - m->c, x1 = m->a + m->c + 1, y1 = m->b + m->c + 1;
        break;
    case TD_PYUI_PTEXT:
        x0 = m->a, y0 = m->b, x1 = m->a + 8 * (m->e < 1 ? 1 : m->e) * td_utf8_len(text), y1 = m->b + 16 * (m->e < 1 ? 1 : m->e);
        break;
    case TD_PYUI_DRAW_IMAGE:
    {
        const obj_t *io = find(m->win);
        int w = m->c > 0 ? m->c : io && io->kind == K_IMAGE ? io->img->w
                                                            : 32767;
        int h = m->d > 0 ? m->d : io && io->kind == K_IMAGE ? io->img->h
                                                            : 32767;
        x0 = m->a, y0 = m->b, x1 = m->a + w, y1 = m->b + h;
        break;
    }
    case TD_PYUI_IMAGE_DATA:
    {
        const td_pyui_image_t *img = msg_image(m, text);
        x0 = m->a, y0 = m->b, x1 = m->a + (img ? img->w : 32767), y1 = m->b + (img ? img->h : 32767);
        break;
    }
    default:
        return false;
    }
    r[0] = x0 >> 1, r[1] = y0 >> 1, r[2] = (x1 + 1) >> 1, r[3] = (y1 + 1) >> 1;
    return true;
}

/* A new size for a canvas: what fits of each page's picture stays. */
static void canvas_resize(obj_t *o, int w, int h)
{
    int cells = w * h * o->pages;
    if (w <= 0 || h <= 0 || s_link->cells - canvas_cells(o) + cells > TD_PYUI_MAX_CELLS)
    {
        post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, CELLS_MSG);
        return;
    }
    cell_t *nc = malloc((size_t)cells * sizeof(cell_t));
    if (!nc)
    {
        post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "out of memory");
        return;
    }
    for (int p = 0; p < o->pages; p++)
    {
        const cell_t *old = canvas_page(o, p);
        for (int y = 0; y < h; y++)
        {
            for (int x = 0; x < w; x++)
            {
                cell_t *c = &nc[(size_t)p * (size_t)(w * h) + (size_t)(y * w + x)];
                if (x < o->rect.w && y < o->rect.h)
                    *c = old[y * o->rect.w + x];
                else
                    *c = (cell_t){' ', -1, -1};
            }
        }
    }
    uint8_t *npx = NULL;
    long count = 4L * w * h * o->pages;
    long bytes = count * (o->rgb ? 3 : 1);
    if (o->px)
    {
        if (!td_pyui_pixels_take(bytes))
        {
            free(nc);
            post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, PIXELS_MSG);
            return;
        }
        npx = malloc((size_t)bytes);
        if (!npx)
        {
            td_pyui_pixels_give(bytes);
            free(nc);
            post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "out of memory");
            return;
        }
    }
    if (npx)
    {
        int ow = 2 * o->rect.w, oh = 2 * o->rect.h, nw = 2 * w, nh = 2 * h;
        for (int p = 0; p < o->pages; p++)
        {
            const uint8_t *src = canvas_layer(o, p);
            uint8_t *dst = npx + (size_t)p * (size_t)(nw * nh);
            for (int y = 0; y < nh; y++)
                for (int x = 0; x < nw; x++)
                    dst[y * nw + x] = x < ow && y < oh ? src[y * ow + x] : td_theme()->win_bg;
            if (o->rgb)
            {
                uint16_t *to = (uint16_t *)(npx + count) + (size_t)p * nw * nh;
                const uint16_t *from = canvas_rgb(o, p);
                for (int y = 0; y < nh; y++)
                    for (int x = 0; x < nw; x++)
                        to[y * nw + x] = x < ow && y < oh ? from[y * ow + x] : td_palette_rgb565(td_theme()->win_bg);
            }
        }
        td_pyui_pixels_give(o->px_bytes);
        free(o->px);
        o->px = npx;
        o->rgb = td_pyui_rgb_enabled() ? (uint16_t *)(npx + count) : NULL;
        o->rgb_serial = ++s_link->pic_serial;
        o->px_bytes = bytes;
    }
    s_link->cells += cells - canvas_cells(o);
    free(o->cells);
    o->cells = nc;
    o->rect.w = w;
    o->rect.h = h;
}

/* Back to a normal window, so a move or resize is not undone by maximise. */
static void unmaximize(td_window_t *w)
{
    if (w->flags & TD_WIN_FULLSCREEN)
        td_win_set_fullscreen(w, false);
    if (w->flags & TD_WIN_MAXIMIZED)
        td_win_toggle_maximize(w);
}

static void show_window(td_window_t *w, int how)
{
    switch (how)
    {
    case TD_PYUI_SHOW_RESTORE:
        unmaximize(w);
        td_win_focus(w);   /* also brings it back from the taskbar */
        break;
    case TD_PYUI_SHOW_MAXIMIZE:
        if (!(w->flags & TD_WIN_MAXIMIZED))
        {
            unmaximize(w);
            td_win_toggle_maximize(w);
        }
        break;
    case TD_PYUI_SHOW_MINIMIZE:
        td_win_minimize(w);
        break;
    case TD_PYUI_SHOW_FULLSCREEN:
        td_win_set_fullscreen(w, true);
        break;
    default:
        break;
    }
}

static void open_window(const td_pyui_msg_t *m, const char *text)
{
    obj_t *o = new_obj(K_WINDOW, m->id);
    if (!o)
        return;
    td_window_desc_t d = {
        .title = text,
        .rect = td_rect(m->c < 0 ? -1 : m->c, m->d < 0 ? -1 : m->d, m->a + 2, m->b + 2),
        .flags = TD_WIN_MOVABLE | TD_WIN_CLOSABLE | ((m->flags & 1) ? TD_WIN_RESIZABLE : 0),
        .on_draw = win_draw,
        .on_event = win_event,
        .on_close = win_close,
        .user = o,
    };
    o->w = td_win_create(&d);
    if (!o->w)
    {
        post(TD_PYUI_EV_CLOSE, m->id, 0, 0, 0, 0, NULL);
        post(TD_PYUI_EV_ERROR, m->id, 0, 0, 0, 0, "the desktop has no room for another window");
        free_obj(o);
        return;
    }
    /* The size asked for; a smaller screen gives less, which check_sizes()
     * then reports. */
    o->rect = td_rect(0, 0, m->a, m->b);
}

static void add_widget(const td_pyui_msg_t *m, const char *text)
{
    obj_t *wo = find(m->win);
    if (!wo || wo->kind != K_WINDOW)
        return;   /* closed in the meantime */
    td_window_t *win = wo->w;
    if (m->flags == TD_PYUI_W_CANVAS)
    {
        int pages = m->e < 1 ? 1 : m->e;
        int cells = m->c * m->d * pages;
        if (pages > TD_PYUI_MAX_PAGES)
        {
            post(TD_PYUI_EV_ERROR, m->id, 0, 0, 0, 0, "too many canvas pages");
            return;
        }
        if (m->c <= 0 || m->d <= 0 || s_link->cells + cells > TD_PYUI_MAX_CELLS)
        {
            post(TD_PYUI_EV_ERROR, m->id, 0, 0, 0, 0, CELLS_MSG);
            return;
        }
        obj_t *o = new_obj(K_CANVAS, m->id);
        if (!o)
            return;
        o->win = m->win;
        o->rect = td_rect(m->a, m->b, m->c, m->d);
        o->pages = (uint8_t)pages;
        o->cells = malloc((size_t)cells * sizeof(cell_t));
        if (!o->cells)
        {
            free_obj(o);
            post(TD_PYUI_EV_ERROR, m->id, 0, 0, 0, 0, "out of memory");
            return;
        }
        s_link->cells += cells;
        for (int p = 0; p < pages; p++)
        {
            o->draw = (uint8_t)p;
            canvas_fill(o, 0, 0, m->c, m->d, ' ', -1, -1);
        }
        o->draw = 0;
        td_win_invalidate(win);
        return;
    }
    obj_t *o = new_obj(K_WIDGET, m->id);
    if (!o)
        return;
    o->win = m->win;
    o->wtype = m->flags;
    switch (m->flags)
    {
    case TD_PYUI_W_LABEL:
        o->wd = td_label(win, m->a, m->b, m->c, text);
        break;
    case TD_PYUI_W_BUTTON:
        o->wd = td_button(win, m->a, m->b, text, on_click, o);
        break;
    case TD_PYUI_W_CHECKBOX:
        o->wd = td_checkbox(win, m->a, m->b, text, m->e != 0, on_check, o);
        break;
    case TD_PYUI_W_TEXTBOX:
        o->wd = td_textbox(win, m->a, m->b, m->c, TD_TEXT_MAX - 1, on_click, o);
        if (o->wd)
        {
            td_widget_set_text(o->wd, text);
            o->wd->secret = m->e != 0;
            o->seen = hash(td_widget_text(o->wd));
        }
        break;
    case TD_PYUI_W_PROGRESS:
        o->wd = td_progress(win, m->a, m->b, m->c);
        if (o->wd)
            td_progress_set(o->wd, m->e);
        break;
    case TD_PYUI_W_LIST:
        /* The scrollbar takes the last column. A width <= 0 reaches to that
         * many cells before the right edge, and the widgets keep following
         * the window when it is resized. */
        o->wd = td_list(win, td_rect(m->a, m->b, m->c - 1, m->d), list_item, on_click, o);
        if (o->wd)
        {
            o->bar = td_scrollbar(win, m->c > 0 ? m->a + m->c - 1 : m->c - 1, m->b, m->d, o->wd);
            o->seen = (uint32_t)td_list_selected(o->wd);
        }
        break;
    default:
        break;
    }
    if (!o->wd)
    {
        free_obj(o);
        post(TD_PYUI_EV_ERROR, m->id, 0, 0, 0, 0, "the desktop has no room for more widgets");
    }
}

static void show_dialog(const td_pyui_msg_t *m, char *text)
{
    const char *p[20];
    int n = split(text, m->len, p, m->op == TD_PYUI_MENU ? 20 : 3);
    obj_t *o = new_obj(m->op == TD_PYUI_MENU ? K_MENU : K_DIALOG, m->id);
    if (!o)
    {
        post(TD_PYUI_EV_RESULT, m->id, -1, 0, 0, 0, NULL);
        return;
    }
    if (m->op == TD_PYUI_MENU)
    {
        int x, y;
        if (!td_wm_mouse_pos(&x, &y))
        {
            x = td_stats()->cols / 3;
            y = td_wm_desktop_rows() / 3;
        }
        td_menu_popup(x, y, p, n, on_menu, o);
        if (!td_menu_is_open())
            check_dialogs();
        return;
    }
    while (n < 3)
        p[n++] = "";
    if (m->op == TD_PYUI_MSGBOX)
        o->w = td_msgbox(p[0], p[1], p[2], on_msgbox, o);
    else if (m->flags & 1)
        o->w = td_passwordbox(p[0], p[1], on_input, o);
    else
        o->w = td_inputbox(p[0], p[1], p[2], on_input, o);
    if (!o->w)
    {
        post(TD_PYUI_EV_RESULT, m->id, -1, 0, 0, 0, NULL);
        free_obj(o);
    }
}

static void run_request(const td_pyui_msg_t *m, char *text)
{
    if (m->op == TD_PYUI_RENDER_CACHE)
    {
        s_link->cache_limit = TD_PYUI_MAX_CELLS > 2000 && m->a > 0 ? (size_t)(m->a > 128 ? 128 : m->a) * 1024 : 0;
        s_link->cache_warm = (m->flags & 1) != 0;
        td_pcache_close();
        atomic_store(&s_raster_ms, 0);
        atomic_store(&s_decode_ms, 0);
        td_picture_profile_reset();
        for (int i = 0; i < MAX_PICS; i++)
        {
            s_link->pics[i].cache.generation = 0;
            s_link->pics[i].warm_attempted = false;
        }
        if (atomic_load(&s_pixels) > 0)
            ensure_render_cache();
        return;
    }
    link_t *l = s_link;
    if (m->op == TD_PYUI_BATCH_BEGIN)
    {
        if (!l->batch_depth++)
        {
            l->batch_start = td_millis();
            l->batch_expired = false;
            l->batch_limit = STARTUP_BATCH_MS;
            for (int i = 0; i < MAX_OBJS; i++)
                if (l->objs[i].kind == K_WINDOW)
                {
                    l->batch_limit = BATCH_MS;
                    break;
                }
            td_frame_hold(l->batch_limit);
        }
        return;
    }
    if (m->op == TD_PYUI_BATCH_END)
    {
        if (l->batch_depth && !--l->batch_depth)
            td_frame_hold(0);
        return;
    }
    if (m->op == TD_PYUI_OPEN)
    {
        open_window(m, text);
        return;
    }
    if (m->op == TD_PYUI_MSGBOX || m->op == TD_PYUI_INPUT || m->op == TD_PYUI_MENU)
    {
        show_dialog(m, text);
        return;
    }
    if (m->op == TD_PYUI_ADD)
    {
        add_widget(m, text);
        return;
    }
    if (m->op == TD_PYUI_IMAGE)
    {
        add_image(m, text);
        return;
    }
    if (m->op == TD_PYUI_FONT)
    {
        add_font(m, text);
        return;
    }
    obj_t *o = find(m->id);
    if (!o)
    {
        if (m->op == TD_PYUI_IMAGE_DATA)
            free(msg_image(m, text));
        return;   /* its window was closed in the meantime */
    }
    int cells[4];
    if (o->kind == K_CANVAS && request_cells(o, m, text, cells))
    {
        cut_pictures(o, o->draw, cells[0], cells[1], cells[2], cells[3]);
        o->rgb_serial = ++s_link->pic_serial;
    }
    td_widget_t *w = o->wd;
    switch (m->op)
    {
    case TD_PYUI_CLOSE:
        if (o->kind == K_WINDOW)
        {
            o->quiet = true;
            td_win_close(o->w);   /* win_close() forgets it */
        }
        else if (o->kind == K_IMAGE || o->kind == K_FONT)
        {
            drop_image_pictures(o->id);
            free_obj(o);
        }
        break;
    case TD_PYUI_IMAGE_DATA:
    case TD_PYUI_DRAW_IMAGE:
    case TD_PYUI_RECT:
    case TD_PYUI_LINE:
    case TD_PYUI_GRADIENT:
    case TD_PYUI_CIRCLE:
    case TD_PYUI_PTEXT:
    case TD_PYUI_BAR:
        pixel_request(o, m, text);
        break;
    case TD_PYUI_PICTURE:
    case TD_PYUI_FONT_TEXT:
        picture_request(o, m, text);
        break;
    case TD_PYUI_TEXT:
        if (o->kind == K_WINDOW)
            td_win_set_title(o->w, text);
        else if (w)
        {
            td_widget_set_text(w, text);
            if (o->wtype == TD_PYUI_W_TEXTBOX)
                o->seen = hash(td_widget_text(w));
        }
        break;
    case TD_PYUI_VALUE:
        if (!w)
            break;
        if (o->wtype == TD_PYUI_W_CHECKBOX)
            td_checkbox_set(w, m->a != 0);
        else if (o->wtype == TD_PYUI_W_PROGRESS)
            td_progress_set(w, m->a);
        else if (o->wtype == TD_PYUI_W_LIST)
        {
            td_list_select(w, m->a);
            o->seen = (uint32_t)td_list_selected(w);
        }
        break;
    case TD_PYUI_VISIBLE:
        if (w)
            td_widget_set_visible(w, m->a != 0);
        if (o->bar)
            td_widget_set_visible(o->bar, m->a != 0);
        break;
    case TD_PYUI_COLOR:
        if (w)
            td_widget_set_color(w, m->a, m->b);
        break;
    case TD_PYUI_FOCUS:
        if (o->kind == K_WINDOW)
            td_win_focus(o->w);
        else if (w)
            td_widget_focus(w);
        break;
    case TD_PYUI_LIST_CLEAR:
    case TD_PYUI_LIST_ADD:
        if (o->wtype != TD_PYUI_W_LIST || !w)
            break;
        if (m->op == TD_PYUI_LIST_CLEAR)
        {
            o->nitems = 0;
            o->items_len = 0;
        }
        else if (!list_add(o, text, m->len))
        {
            post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "the list is full");
        }
        td_list_set_count(w, o->nitems);
        o->seen = (uint32_t)td_list_selected(w);
        break;
    case TD_PYUI_PUT:
    case TD_PYUI_FILL:
        if (o->kind != K_CANVAS)
            break;
        if (m->op == TD_PYUI_PUT)
        {
            canvas_put(o, m->a, m->b, text, m->c, m->d);
        }
        else
        {
            const char *s = text;
            uint32_t cp = *s ? td_utf8_next(&s) : ' ';
            canvas_fill(o, m->a, m->b, m->c, m->d, cp, m->e, m->f);
        }
        if (o->draw == o->show)
            td_win_invalidate(NULL);
        break;
    case TD_PYUI_BIG:
        if (o->kind != K_CANVAS)
            break;
        canvas_big(o, m->a, m->b, text, m->c, m->d, m->e, m->f);
        if (o->draw == o->show)
            td_win_invalidate(NULL);
        break;
    case TD_PYUI_PAGE:
        if (o->kind != K_CANVAS)
            break;
        if (m->a >= o->pages || m->b >= o->pages)
        {
            post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "no such canvas page");
            break;
        }
        if (m->a >= 0)
            o->draw = (uint8_t)m->a;
        if (m->b >= 0 && m->b != o->show)
        {
            o->show = (uint8_t)m->b;
            td_win_invalidate(NULL);
        }
        break;
    case TD_PYUI_PAGE_COPY:
        if (o->kind != K_CANVAS)
            break;
        if (m->a < 0 || m->b < 0 || m->a >= o->pages || m->b >= o->pages)
        {
            post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "no such canvas page");
            break;
        }
        if (m->a != m->b)
        {
            memcpy(canvas_page(o, m->b), canvas_page(o, m->a), (size_t)(o->rect.w * o->rect.h) * sizeof(cell_t));
            if (o->px)
                memcpy(canvas_layer(o, m->b), canvas_layer(o, m->a), (size_t)layer_size(o));
            if (o->rgb)
                memcpy(canvas_rgb(o, m->b), canvas_rgb(o, m->a), (size_t)layer_size(o) * sizeof(uint16_t));
            o->rgb_serial = ++s_link->pic_serial;
            copy_pictures(o, m->a, m->b);
        }
        if (m->b == o->show)
            td_win_invalidate(NULL);
        break;
    case TD_PYUI_RESIZE:
        if (o->kind == K_WINDOW)
        {
            unmaximize(o->w);
            td_win_resize(o->w, m->a + 2, m->b + 1 + td_wm_title_rows());
        }
        else if (o->kind == K_CANVAS)
        {
            forget_pictures(o);
            canvas_resize(o, m->a, m->b);
            td_win_invalidate(NULL);
        }
        break;
    case TD_PYUI_MOVE:
        if (o->kind == K_WINDOW)
        {
            unmaximize(o->w);
            td_win_move(o->w, m->a, m->b);
        }
        else if (o->kind == K_CANVAS)
        {
            o->rect.x = m->a;
            o->rect.y = m->b;
            td_win_invalidate(NULL);
        }
        break;
    case TD_PYUI_SHOW:
        if (o->kind == K_WINDOW)
            show_window(o->w, m->a);
        break;
    default:
        break;
    }
}

/* The script ended: close everything it opened. */
static void close_all(void)
{
    td_pcache_close();
    td_frame_hold(0);
    s_link->batch_depth = 0;
    link_t *l = s_link;
    l->closing = true;
    td_pyui_msg_t m;
    while (ring_get(&l->req, &m, l->text, sizeof(l->text)))
    {
        if (m.op == TD_PYUI_FONT)
        {
            td_pyui_font_t *font = NULL;
            if (m.len == sizeof(font))
                memcpy(&font, l->text, sizeof(font));
            if (font)
            {
                td_pyui_pixels_give((long)font->bytes);
                free(font);
            }
            continue;
        }
        if (m.op != TD_PYUI_IMAGE && m.op != TD_PYUI_IMAGE_DATA)
            continue;
        td_pyui_image_t *img = msg_image(&m, l->text);
        if (img && m.op == TD_PYUI_IMAGE)
            td_pyui_pixels_give(td_pyui_image_bytes(img));
        free(img);
    }
    for (int i = 0; i < MAX_OBJS; i++)
    {
        obj_t *o = &l->objs[i];
        if (o->kind == K_WINDOW)
            td_win_close(o->w);
        else if (o->kind == K_DIALOG)
            td_win_close(o->w);
        else if (o->kind == K_MENU)
            td_menu_close();
        if (o->kind != K_FREE)
            free_obj(o);
    }
}

static void tick(void *user)
{
    (void)user;
    int state = atomic_load_explicit(&s_state, memory_order_acquire);
    if (state == LINK_CLOSING)
    {
        close_all();
        atomic_store_explicit(&s_state, LINK_OFF, memory_order_release);
        return;
    }
    if (state != LINK_ON)
        return;
    link_t *l = s_link;
    check_dialogs();
    td_pyui_msg_t m;
    for (int n = 0; n < TICK_BUDGET && ring_get(&l->req, &m, l->text, sizeof(l->text)); n++)
        run_request(&m, l->text);
    if (l->batch_depth && !l->batch_expired && td_millis() - l->batch_start >= l->batch_limit)
    {
        l->batch_expired = true;
        td_frame_hold(0);
        char message[80];
        snprintf(message, sizeof(message), "drawing batch exceeded %lu ms; partial drawing released", (unsigned long)l->batch_limit);
        post(TD_PYUI_EV_ERROR, 0, 0, 0, 0, 0, message);
    }
    poll_values();
    check_sizes();
    warm_render_cache();
}

bool td_pyui_running(void)
{
    return atomic_load(&s_state) != LINK_OFF;
}

void td_pyui_start(void)
{
    if (atomic_load(&s_started))
        return;
    if (td_timer_start(TICK_MS, true, tick, NULL, td_millis()) >= 0)
        atomic_store(&s_started, true);
}
