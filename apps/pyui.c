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
#include <stdlib.h>
#include <string.h>

#include "td_apps.h"
#include "tinydesk/td_canvas.h"

#define REQ_SIZE    4096   /* powers of two */
#define EVT_SIZE    2048
#define MAX_OBJS    104    /* 4 windows, 48 widgets, images, dialogs and spare */
#define TICK_MS     10
#define TICK_BUDGET 200    /* requests run per tick, so the desktop stays smooth */
#define LIST_ITEMS  1000
#define LIST_BYTES  16384

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
    K_IMAGE
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
    long px_bytes;
    /* image: its pixels; a mask added later is a block of its own */
    td_pyui_image_t *img;
    uint8_t *own_mask;
} obj_t;

typedef struct
{
    ring_t req, evt;
    obj_t objs[MAX_OBJS];
    int cells;          /* canvas cells in use */
    bool closing;       /* close_all() is running: post nothing */
    char text[TD_PYUI_TEXT_MAX + 1];
} link_t;

static atomic_int s_state = LINK_OFF;
static atomic_long s_pixels;   /* bytes of TD_PYUI_MAX_PIXELS in use */
static link_t *s_link;    /* set by the script before LINK_ON, freed after LINK_OFF */
static atomic_bool s_started;

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
    if (w < 1 || h < 1 || w > 4096 || h > 4096)
        return NULL;
    size_t n = (size_t)w * (size_t)h;
    td_pyui_image_t *img = calloc(1, sizeof(*img) + n * (mask ? 2 : 1));
    if (!img)
        return NULL;
    img->w = (int16_t)w;
    img->h = (int16_t)h;
    img->px = (uint8_t *)(img + 1);
    img->mask = mask ? img->px + n : NULL;
    return img;
}

long td_pyui_image_bytes(const td_pyui_image_t *img)
{
    return (long)img->w * img->h * (img->mask ? 2 : 1);
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

static void free_obj(obj_t *o)
{
    if (o->kind == K_CANVAS)
        s_link->cells -= canvas_cells(o);
    if (o->px)
        td_pyui_pixels_give(o->px_bytes);
    free(o->px);
    if (o->img)
        td_pyui_pixels_give(td_pyui_image_bytes(o->img));
    free(o->img);
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
static void draw_canvas(const obj_t *o, const td_theme_t *t)
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
            draw_canvas(o, t);
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
    for (int i = 0; i < MAX_OBJS; i++)
    {
        obj_t *o = &s_link->objs[i];
        if (o->kind == K_CANVAS && o->win == wo->id && td_rect_contains(o->rect, ev->x, ev->y))
        {
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
        }
    }
}

/* The canvas's pixel layers, allocated by the first pixel drawn. */
static bool canvas_pixels(obj_t *o)
{
    if (o->px)
        return true;
    long bytes = layer_size(o) * o->pages;
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
        return true;
    }
    if (o->kind != K_CANVAS || !canvas_pixels(o))
        return false;
    s->px = canvas_layer(o, o->draw);
    s->w = 2 * o->rect.w;
    s->h = 2 * o->rect.h;
    s->cells = canvas_page(o, o->draw);
    s->cols = o->rect.w;
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
    if (s->cells)
    {
        cell_t *c = &s->cells[(y >> 1) * s->cols + (x >> 1)];
        if (c->ch != PX_CELL)
        {
            uint8_t b = cell_bg(c);
            uint8_t *p = s->px + (y & ~1) * s->w + (x & ~1);
            p[0] = p[1] = p[s->w] = p[s->w + 1] = b;
            c->ch = PX_CELL;
            c->fg = -1;
            c->bg = TD_PYUI_CLEAR;
        }
        s->px[y * s->w + x] = color < 0 ? td_theme()->win_bg : (uint8_t)color;
        return;
    }
    size_t i = (size_t)y * (size_t)s->w + (size_t)x;
    if (s->mask)
        s->mask[i] = color == TD_PYUI_CLEAR;
    if (color >= 0)
        s->px[i] = (uint8_t)color;
}

static void fill_rect(surf_t *s, int x0, int y0, int w, int h, int color)
{
    int x1 = x0 + w > s->w ? s->w : x0 + w, y1 = y0 + h > s->h ? s->h : y0 + h;
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
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            if (!in_round_rect(x, y, w, h, r))
                continue;
            if (outline && in_round_rect(x - 1, y, w, h, r) && in_round_rect(x + 1, y, w, h, r) &&
                in_round_rect(x, y - 1, w, h, r) && in_round_rect(x, y + 1, w, h, r))
                continue;
            plot(s, x0 + x, y0 + y, color);
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
    for (int y = 0; y < dh; y++)
    {
        int ty = dy + y;
        if (ty < 0 || ty >= d->h)
            continue;
        int iy = sy + (int)((long)y * sh / dh);
        const uint8_t *row = img->px + (size_t)iy * (size_t)img->w;
        const uint8_t *mrow = img->mask ? img->mask + (size_t)iy * (size_t)img->w : NULL;
        for (int x = 0; x < dw; x++)
        {
            int ix = sx + (int)((long)x * sw / dw);
            if (!mrow || !mrow[ix])
                plot(d, dx + x, ty, row[ix]);
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
        if (m->op != TD_PYUI_GRADIENT && m->e == TD_PYUI_CLEAR && !image_mask(o, &s))
            return;
        if (m->op == TD_PYUI_RECT && m->f > 0)
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
        break;
    }
    if (o->kind == K_CANVAS && o->draw == o->show)
        td_win_invalidate(NULL);
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
    long bytes = 4L * w * h * o->pages;
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
        }
        td_pyui_pixels_give(o->px_bytes);
        free(o->px);
        o->px = npx;
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
    obj_t *o = find(m->id);
    if (!o)
    {
        if (m->op == TD_PYUI_IMAGE_DATA)
            free(msg_image(m, text));
        return;   /* its window was closed in the meantime */
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
        else if (o->kind == K_IMAGE)
        {
            free_obj(o);
        }
        break;
    case TD_PYUI_IMAGE_DATA:
    case TD_PYUI_DRAW_IMAGE:
    case TD_PYUI_RECT:
    case TD_PYUI_LINE:
    case TD_PYUI_GRADIENT:
    case TD_PYUI_CIRCLE:
        pixel_request(o, m, text);
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
    link_t *l = s_link;
    l->closing = true;
    td_pyui_msg_t m;
    while (ring_get(&l->req, &m, l->text, sizeof(l->text)))
    {
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
    poll_values();
    check_sizes();
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
