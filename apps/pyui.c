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
 */
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#include "td_apps.h"

#define REQ_SIZE    4096   /* powers of two */
#define EVT_SIZE    2048
#define MAX_OBJS    72     /* 4 windows, 48 widgets, dialogs and spare */
#define TICK_MS     10
#define TICK_BUDGET 200    /* requests run per tick, so the desktop stays smooth */
#define LIST_ITEMS  1000
#define LIST_BYTES  16384

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
    K_MENU
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
    cell_t *cells;
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

static void free_obj(obj_t *o)
{
    if (o->kind == K_CANVAS)
        s_link->cells -= o->rect.w * o->rect.h;
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

static void win_draw(td_window_t *win, int cw, int ch)
{
    (void)cw;
    (void)ch;
    obj_t *wo = win->user;
    const td_theme_t *t = td_theme();
    for (int i = 0; i < MAX_OBJS; i++)
    {
        obj_t *o = &s_link->objs[i];
        if (o->kind != K_CANVAS || o->win != wo->id)
            continue;
        for (int y = 0; y < o->rect.h; y++)
        {
            for (int x = 0; x < o->rect.w; x++)
            {
                const cell_t *c = &o->cells[y * o->rect.w + x];
                td_putc(o->rect.x + x, o->rect.y + y, c->ch ? c->ch : ' ', c->fg < 0 ? t->win_fg : (uint8_t)c->fg,
                        c->bg < 0 ? t->win_bg : (uint8_t)c->bg, 0);
            }
        }
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
        {
            cell_t *c = &o->cells[y * o->rect.w + x];
            c->ch = (uint16_t)(cp > 0xFFFF ? '?' : cp);
            c->fg = (int16_t)fg;
            c->bg = (int16_t)bg;
        }
        x++;
    }
}

static void canvas_fill(obj_t *o, int x0, int y0, int w, int h, uint32_t cp, int fg, int bg)
{
    for (int y = y0 < 0 ? 0 : y0; y < y0 + h && y < o->rect.h; y++)
    {
        for (int x = x0 < 0 ? 0 : x0; x < x0 + w && x < o->rect.w; x++)
        {
            cell_t *c = &o->cells[y * o->rect.w + x];
            c->ch = (uint16_t)(cp > 0xFFFF ? '?' : cp);
            c->fg = (int16_t)fg;
            c->bg = (int16_t)bg;
        }
    }
}

/* A new size for a canvas: what fits of the old picture stays. */
static void canvas_resize(obj_t *o, int w, int h)
{
    int cells = w * h;
    if (w <= 0 || h <= 0 || s_link->cells - o->rect.w * o->rect.h + cells > TD_PYUI_MAX_CELLS)
    {
        post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "canvases are limited to 2000 cells in all");
        return;
    }
    cell_t *nc = malloc((size_t)cells * sizeof(cell_t));
    if (!nc)
    {
        post(TD_PYUI_EV_ERROR, o->id, 0, 0, 0, 0, "out of memory");
        return;
    }
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            cell_t *c = &nc[y * w + x];
            if (x < o->rect.w && y < o->rect.h)
                *c = o->cells[y * o->rect.w + x];
            else
                *c = (cell_t){' ', -1, -1};
        }
    }
    free(o->cells);
    o->cells = nc;
    s_link->cells += cells - o->rect.w * o->rect.h;
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
        int cells = m->c * m->d;
        if (m->c <= 0 || m->d <= 0 || s_link->cells + cells > TD_PYUI_MAX_CELLS)
        {
            post(TD_PYUI_EV_ERROR, m->id, 0, 0, 0, 0, "canvases are limited to 2000 cells in all");
            return;
        }
        obj_t *o = new_obj(K_CANVAS, m->id);
        if (!o)
            return;
        o->win = m->win;
        o->rect = td_rect(m->a, m->b, m->c, m->d);
        o->cells = malloc((size_t)cells * sizeof(cell_t));
        if (!o->cells)
        {
            free_obj(o);
            post(TD_PYUI_EV_ERROR, m->id, 0, 0, 0, 0, "out of memory");
            return;
        }
        s_link->cells += cells;
        canvas_fill(o, 0, 0, m->c, m->d, ' ', -1, -1);
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
    obj_t *o = find(m->id);
    if (!o)
        return;   /* its window was closed in the meantime */
    td_widget_t *w = o->wd;
    switch (m->op)
    {
    case TD_PYUI_CLOSE:
        if (o->kind == K_WINDOW)
        {
            o->quiet = true;
            td_win_close(o->w);   /* win_close() forgets it */
        }
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
        td_win_invalidate(NULL);
        break;
    case TD_PYUI_RESIZE:
        if (o->kind == K_WINDOW)
        {
            unmaximize(o->w);
            td_win_resize(o->w, m->a + 2, m->b + 2);
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

void td_pyui_start(void)
{
    if (atomic_load(&s_started))
        return;
    if (td_timer_start(TICK_MS, true, tick, NULL, td_millis()) >= 0)
        atomic_store(&s_started, true);
}
