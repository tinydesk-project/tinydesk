/*
 * wm.c - window manager: window pool, z-order, focus, moving and resizing,
 * the desktop, the taskbar and the start menu.
 *
 * Every frame is composed from scratch (desktop, then windows bottom to
 * top, then taskbar and menu); the renderer's diff keeps traffic small.
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "tinydesk/td.h"

/* ------------------------------------------------------------- state */

static td_window_t s_wins[TD_MAX_WINDOWS];
static int s_order[TD_MAX_WINDOWS];   /* pool indexes, bottom to top */
static int s_order_count;
static int s_cols = TD_DEFAULT_COLS, s_rows = TD_DEFAULT_ROWS;
static bool s_dirty = true;

typedef enum
{
    DRAG_NONE,
    DRAG_MOVE,
    DRAG_RESIZE,
    DRAG_CAPTURE,
    DRAG_DESKTOP
} drag_mode_t;
static struct
{
    drag_mode_t mode;
    td_window_t *win;
    int dx, dy;           /* grab offset inside the frame */
} s_drag;

static struct
{
    td_window_t *win;
    uint32_t time;
} s_title_click;          /* double-click on a title bar maximises */

static const td_app_t *s_apps[TD_MAX_APPS];
static int s_app_count;

/* Drag and drop of files: a press on a draggable thing arms it, moving
 * the mouse a little starts it, releasing drops it. */
static struct
{
    bool armed;             /* a press that may turn into a drag */
    bool active;
    int x0, y0;             /* where the press happened */
    int icon;               /* desktop icon pressed, or -1 */
    td_window_t *win;       /* window pressed in, or NULL */
    td_drag_item_t item;
} s_dnd;

/* Extra start-menu entries (above "Redraw screen" / "Exit"). */
#define START_EXTRA_MAX 4
static struct
{
    const char *label;
    void (*fn)(void);
} s_start_extra[START_EXTRA_MAX];
static int s_start_extra_count;

/* The logged-in user, shown in the taskbar tray. */
static char s_user_label[24];

/* Taskbar tray hit area: the network indicator. */
static int s_tray_net_x0 = -1, s_tray_net_x1 = -1;

/* Popup menu (start menu or context menu), see "popup menus" below. */
static bool s_menu_open;
static int s_menu_sel;

/* Extra desktop icons (the Desktop folder), after the app icons. */
static const td_desktop_provider_t *s_provider;

/* Taskbar hit areas, filled in while composing. */
#define TASKBAR_START_W 7
static td_ui_size_t s_icon_size = TD_UI_MEDIUM, s_menu_size = TD_UI_MEDIUM, s_bar_size = TD_UI_MEDIUM;
static const char *s_launch_icon;   /* glyph of the app being launched */
static struct
{
    int x0, x1;
    td_window_t *win;
} s_task_btn[TD_MAX_WINDOWS];
static int s_task_btn_count;

/* Mouse position for hover highlights, and an id for what is under it: a
 * frame is only recomposed when the pointer moves onto something else. */
static int s_mouse_x = -1, s_mouse_y = -1;
static uintptr_t s_hover_id;
static volatile bool s_pointer;   /* td_wm_set_pointer */

/* Desktop icons: one per app, in columns from the top-left corner. */
/* Icon slots (cells), by td_ui_size_t: small = one line; medium = 3 rows
 * of glyph box + 2 label rows + gap; large = 5 rows of box + 1 label row +
 * gap. */
static const int s_icon_slot_w[3] = {21, 12, 16};
static const int s_icon_slot_h[3] = {1, 6, 7};
#define ICON_LABEL_MAX 64 /* a label line: up to 15 columns, up to 4 bytes each */
static int icon_count(void);
static td_ui_size_t icon_eff(void);
static int icon_w(void)
{
    return s_icon_slot_w[icon_eff()];
}
static int icon_h(void)
{
    return s_icon_slot_h[icon_eff()];
}
static int s_icon_sel = -1;
static int s_icon_click = -1;
static uint32_t s_icon_click_ms;

/* ------------------------------------------------------------ helpers */

static int clamp(int v, int lo, int hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

static td_window_t *top_window(bool include_hidden)
{
    for (int i = s_order_count - 1; i >= 0; i--)
    {
        td_window_t *w = &s_wins[s_order[i]];
        if (include_hidden || !(w->flags & TD_WIN_HIDDEN))
            return w;
    }
    return NULL;
}

/* Top-most open modal window, or NULL. */
static td_window_t *modal_window(void)
{
    for (int i = s_order_count - 1; i >= 0; i--)
    {
        td_window_t *w = &s_wins[s_order[i]];
        if ((w->flags & TD_WIN_MODAL) && !(w->flags & TD_WIN_HIDDEN))
            return w;
    }
    return NULL;
}

/* Kiosk mode: one application on the whole screen (td_wm_set_kiosk). */
static bool s_kiosk;
static bool (*s_kiosk_hook)(const td_event_t *ev);

static bool taskbar_visible(void)
{
    td_window_t *top = top_window(false);
    return !s_kiosk && !(top && (top->flags & TD_WIN_FULLSCREEN));
}

static int taskbar_h(void)
{
    if (s_kiosk)
        return 0;
    return s_bar_size == TD_UI_LARGE ? 2 : 1;
}
static bool in_taskbar(int y)
{
    return taskbar_visible() && y >= s_rows - taskbar_h() && y < s_rows;
}

int td_wm_desktop_rows(void)
{
    return s_rows - taskbar_h();
}

void td_wm_set_pointer(bool on)
{
    s_pointer = on;
    s_dirty = true;
}

bool td_wm_mouse_pos(int *x, int *y)
{
    if (s_mouse_x < 0)
        return false;
    *x = s_mouse_x;
    *y = s_mouse_y;
    return true;
}

/* Roughly how light a 256-colour palette entry is (xterm's colours). */
static bool colour_is_light(uint8_t c)
{
    if (c < 16)
        return c == 7 || c == 8 || c == 10 || c == 11 || c == 14 || c == 15;
    if (c >= 232)
        return c >= 244;   /* the grey ramp: 8 + 10 * (c - 232) */
    static const uint8_t level[6] = {0, 95, 135, 175, 215, 255};
    int i = c - 16;
    int r = level[i / 36], g = level[(i / 6) % 6], b = level[i % 6];
    return r * 299 + g * 587 + b * 114 >= 128 * 1000;
}

/* Swap colours to show that the mouse is over something. */
static void hover_colours(uint8_t *fg, uint8_t *bg)
{
    uint8_t t = *fg;
    *fg = *bg;
    *bg = t;
}

static bool mouse_over(int x, int y, int w)
{
    return s_mouse_y == y && s_mouse_x >= x && s_mouse_x < x + w;
}
/* Over a taskbar item: every row of the taskbar counts. */
static bool mouse_over_bar(int x, int w)
{
    return in_taskbar(s_mouse_y) && s_mouse_x >= x && s_mouse_x < x + w;
}

static void order_remove(int idx)
{
    for (int i = 0; i < s_order_count; i++)
    {
        if (s_order[i] != idx)
            continue;
        memmove(&s_order[i], &s_order[i + 1], (size_t)(s_order_count - i - 1) * sizeof(int));
        s_order_count--;
        return;
    }
}

static int win_index(const td_window_t *w)
{
    return (int)(w - s_wins);
}

/* Put a new window on top, except that an open modal dialog stays above
 * ordinary windows (otherwise it would be hidden yet still hold the input). */
static void order_push(int idx)
{
    int at = s_order_count;
    if (!(s_wins[idx].flags & TD_WIN_MODAL))
    {
        for (int i = 0; i < s_order_count; i++)
        {
            const td_window_t *o = &s_wins[s_order[i]];
            if ((o->flags & TD_WIN_MODAL) && !(o->flags & TD_WIN_HIDDEN))
            {
                at = i;
                break;
            }
        }
    }
    memmove(&s_order[at + 1], &s_order[at], (size_t)(s_order_count - at) * sizeof(int));
    s_order[at] = idx;
    s_order_count++;
}

/* Window frames (td_wm_set_window_size): small = one title row and
 * buttons "[x]"; medium = buttons "[ x ]" and a wider resize corner; large
 * = the same with two title rows. */
static td_ui_size_t s_win_size = TD_UI_SMALL;

static int title_h(void)
{
    return s_win_size == TD_UI_LARGE ? 2 : 1;
}
static int button_w(void)
{
    return s_win_size == TD_UI_SMALL ? 3 : 5;
}
static int grip_w(void)
{
    return s_win_size == TD_UI_SMALL ? 2 : 4;
}

/* Title-bar button positions (absolute x). */
static int close_x(const td_window_t *w)
{
    return w->rect.x + w->rect.w - 1 - button_w();
}
static int min_x(const td_window_t *w)
{
    return close_x(w) - button_w();
}
static bool in_title(const td_window_t *w, int y)
{
    return y >= w->rect.y && y < w->rect.y + title_h();
}

static bool has_frame(const td_window_t *w)
{
    return !(w->flags & TD_WIN_FULLSCREEN);
}

/* ------------------------------------------------------------ windows */

bool td_win_is_open(const td_window_t *win)
{
    return win && win >= s_wins && win < s_wins + TD_MAX_WINDOWS && win->used;
}

td_rect_t td_win_client(const td_window_t *win)
{
    td_rect_t r = win->rect;
    if (!has_frame(win))
        return r;
    return td_rect(r.x + 1, r.y + title_h(), r.w - 2, r.h - title_h() - 1);
}

int td_wm_title_rows(void)
{
    return title_h();
}

td_ui_size_t td_wm_window_size(void)
{
    return s_win_size;
}

static void clamp_rect(td_window_t *w);

void td_wm_set_window_size(td_ui_size_t size)
{
    int old = title_h();
    s_win_size = size;
    int delta = title_h() - old;
    /* Open windows keep their contents' size: they grow or shrink by the
     * title bar's change. */
    for (int i = 0; delta && i < TD_MAX_WINDOWS; i++)
    {
        td_window_t *w = &s_wins[i];
        if (!w->used || !has_frame(w))
            continue;
        w->min_h += delta;
        w->restore_rect.h += delta;
        if (!(w->flags & TD_WIN_MAXIMIZED))
        {
            w->rect.h += delta;
            if (w->rect.y + w->rect.h > td_wm_desktop_rows())
                w->rect.y = td_wm_desktop_rows() - w->rect.h;
            clamp_rect(w);
        }
    }
    s_dirty = true;
}

void td_win_invalidate(td_window_t *win)
{
    (void)win;
    s_dirty = true;
}

void td_wm_invalidate(void)
{
    s_dirty = true;
}
bool td_wm_needs_redraw(void)
{
    return s_dirty;
}

static void tick_cb(void *user)
{
    td_window_t *w = user;
    if (td_win_is_open(w) && w->on_tick)
        w->on_tick(w);
}

/* Keep a window's rectangle inside the screen. */
static void clamp_rect(td_window_t *w)
{
    int desk = td_wm_desktop_rows();
    if (w->flags & TD_WIN_FULLSCREEN)
    {
        w->rect = td_rect(0, 0, s_cols, s_rows);
        return;
    }
    if (w->flags & TD_WIN_MAXIMIZED)
    {
        w->rect = td_rect(0, 0, s_cols, desk);
        return;
    }
    td_rect_t *r = &w->rect;
    r->w = clamp(r->w, w->min_w, s_cols);
    r->h = clamp(r->h, w->min_h, desk);
    /* The title bar always stays on screen. */
    r->x = clamp(r->x, 0, s_cols - r->w);
    r->y = clamp(r->y, 0, desk - 1);
}

/* Set when an ordinary window was refused because the pool is (nearly) full. */
static bool s_win_refused;

td_window_t *td_win_create(const td_window_desc_t *desc)
{
    int idx = -1, free_slots = 0;
    for (int i = 0; i < TD_MAX_WINDOWS; i++)
    {
        if (s_wins[i].used)
            continue;
        if (idx < 0)
            idx = i;
        free_slots++;
    }
    /* The last free slot is kept for a modal dialog, so "too many windows"
     * can still be said when the pool is full. */
    if (idx < 0 || (free_slots <= 1 && !(desc->flags & TD_WIN_MODAL)))
    {
        s_win_refused = true;
        return NULL;
    }

    td_window_t *w = &s_wins[idx];
    memset(w, 0, sizeof(*w));
    w->used = true;
    w->id = (uint8_t)idx;
    td_utf8_copy(w->title, sizeof(w->title), desc->title ? desc->title : "", INT_MAX); /* whole characters */
    w->flags = desc->flags;
    w->min_w = desc->min_w > 0 ? desc->min_w : 16;
    w->min_h = desc->min_h > 0 ? desc->min_h : 4;
    w->on_draw = desc->on_draw;
    w->on_event = desc->on_event;
    w->on_close = desc->on_close;
    w->on_close_request = desc->on_close_request;
    w->on_tick = desc->on_tick;
    w->on_drag_start = desc->on_drag_start;
    w->on_drop = desc->on_drop;
    w->user = desc->user;
    w->timer_id = -1;
    w->icon = desc->icon ? desc->icon : s_launch_icon;

    w->rect = desc->rect;
    if (w->rect.w <= 0)
        w->rect.w = 40;
    if (w->rect.h <= 0)
        w->rect.h = 12;
    if (has_frame(w))
    {
        w->rect.h += title_h() - 1;   /* a taller title bar adds rows */
        w->min_h += title_h() - 1;
    }
    if (w->rect.w > s_cols)
        w->rect.w = s_cols;
    if (w->rect.h > td_wm_desktop_rows())
        w->rect.h = td_wm_desktop_rows();
    if (desc->rect.x < 0)
        w->rect.x = (s_cols - w->rect.w) / 2;
    if (desc->rect.y < 0)
        w->rect.y = (td_wm_desktop_rows() - w->rect.h) / 2;
    w->restore_rect = w->rect;
    /* In kiosk mode every window but a dialog covers the screen, with no
     * frame to move, resize, minimise or close it by. */
    if (s_kiosk && !(w->flags & TD_WIN_MODAL))
        w->flags |= TD_WIN_FULLSCREEN;
    clamp_rect(w);

    if (w->on_tick && desc->tick_ms > 0)
    {
        w->timer_id = td_timer_start(desc->tick_ms, true, tick_cb, w, td_millis());
        if (w->timer_id < 0)
            w->incomplete = true;   /* it would never update */
    }

    order_push(idx);
    s_menu_open = false;
    s_dirty = true;
    return w;
}

void td_win_close(td_window_t *win)
{
    if (!td_win_is_open(win))
        return;
    void (*on_close)(td_window_t *) = win->on_close;
    win->on_close = NULL;       /* guard against re-entry */
    if (on_close)
        on_close(win);

    if (win->timer_id >= 0)
        td_timer_stop(win->timer_id);
    td_widgets_free(win);
    order_remove(win_index(win));
    if (s_drag.win == win)
        s_drag.mode = DRAG_NONE, s_drag.win = NULL;
    if (s_title_click.win == win)
        s_title_click.win = NULL;
    if (s_dnd.win == win)
        s_dnd.win = NULL, s_dnd.armed = false;
    win->used = false;
    s_dirty = true;
}

void td_wm_close_all(void)
{
    /* Close from the top; on_close handlers may open nothing new here. */
    for (int pass = 0; pass < TD_MAX_WINDOWS && s_order_count > 0; pass++)
    {
        td_window_t *w = &s_wins[s_order[s_order_count - 1]];
        td_win_close(w);
    }
    memset(&s_dnd, 0, sizeof(s_dnd));
    s_drag.mode = DRAG_NONE;
    s_drag.win = NULL;
    s_menu_open = false;
    s_dirty = true;
}

void td_win_request_close(td_window_t *win)
{
    if (!td_win_is_open(win))
        return;
    if (win->on_close_request && !win->on_close_request(win))
        return;
    td_win_close(win);
}

void td_win_focus(td_window_t *win)
{
    if (!td_win_is_open(win))
        return;
    td_window_t *modal = modal_window();
    if (modal && modal != win)
        return;   /* a modal dialog keeps the focus */
    win->flags &= (uint16_t)~TD_WIN_HIDDEN;
    int idx = win_index(win);
    order_remove(idx);
    s_order[s_order_count++] = idx;
    s_dirty = true;
}

td_window_t *td_win_focused(void)
{
    return top_window(false);
}

void td_win_set_title(td_window_t *win, const char *title)
{
    if (!td_win_is_open(win))
        return;
    td_utf8_copy(win->title, sizeof(win->title), title ? title : "", INT_MAX); /* whole characters */
    s_dirty = true;
}

void td_win_move(td_window_t *win, int x, int y)
{
    if (!td_win_is_open(win))
        return;
    win->rect.x = x;
    win->rect.y = y;
    clamp_rect(win);
    s_dirty = true;
}

void td_win_resize(td_window_t *win, int w, int h)
{
    if (!td_win_is_open(win))
        return;
    win->rect.w = clamp(w, win->min_w, s_cols - win->rect.x);
    win->rect.h = clamp(h, win->min_h, td_wm_desktop_rows() - win->rect.y);
    clamp_rect(win);
    s_dirty = true;
}

void td_win_minimize(td_window_t *win)
{
    if (!td_win_is_open(win) || (win->flags & TD_WIN_MODAL))
        return;
    win->flags |= TD_WIN_HIDDEN;
    s_dirty = true;
}

void td_win_toggle_maximize(td_window_t *win)
{
    if (!td_win_is_open(win) || !(win->flags & TD_WIN_RESIZABLE))
        return;
    if (win->flags & (TD_WIN_MAXIMIZED | TD_WIN_FULLSCREEN))
    {
        win->flags &= (uint16_t) ~(TD_WIN_MAXIMIZED | TD_WIN_FULLSCREEN);
        win->rect = win->restore_rect;
    }
    else
    {
        win->restore_rect = win->rect;
        win->flags |= TD_WIN_MAXIMIZED;
    }
    clamp_rect(win);
    s_dirty = true;
}

void td_win_set_fullscreen(td_window_t *win, bool on)
{
    if (!td_win_is_open(win))
        return;
    bool is_on = (win->flags & TD_WIN_FULLSCREEN) != 0;
    if (on == is_on)
        return;
    if (on)
    {
        if (!(win->flags & TD_WIN_MAXIMIZED))
            win->restore_rect = win->rect;
        win->flags &= (uint16_t)~TD_WIN_MAXIMIZED;
        win->flags |= TD_WIN_FULLSCREEN;
    }
    else
    {
        win->flags &= (uint16_t)~TD_WIN_FULLSCREEN;
        win->rect = win->restore_rect;
    }
    clamp_rect(win);
    td_win_focus(win);
    s_dirty = true;
}

/* Alt+Shift+Tab: the bottom visible window to the top. */
static void cycle_windows_back(void)
{
    for (int i = 0; i < s_order_count; i++)
    {
        td_window_t *w = &s_wins[s_order[i]];
        if (!(w->flags & TD_WIN_HIDDEN))
        {
            td_win_focus(w);
            return;
        }
    }
}

/* Move the top window to the bottom so the next one comes forward. */
static void cycle_windows(void)
{
    td_window_t *top = top_window(false);
    if (!top)
        return;
    int idx = win_index(top);
    order_remove(idx);
    memmove(&s_order[1], &s_order[0], (size_t)s_order_count * sizeof(int));
    s_order[0] = idx;
    s_order_count++;
    s_dirty = true;
}

/* ------------------------------------------------------------ manager */

void td_wm_init(int cols, int rows)
{
    for (int i = 0; i < TD_MAX_WINDOWS; i++)
    {
        if (s_wins[i].used)
            td_win_close(&s_wins[i]);
    }
    memset(s_wins, 0, sizeof(s_wins));
    s_order_count = 0;
    s_drag.mode = DRAG_NONE;
    s_drag.win = NULL;
    s_menu_open = false;
    td_widgets_reset();
    s_cols = cols;
    s_rows = rows;
    s_dirty = true;
}

void td_wm_screen_size(int *cols, int *rows)
{
    *cols = s_cols;
    *rows = s_rows;
}

void td_wm_set_screen_size(int cols, int rows)
{
    s_cols = cols;
    s_rows = rows;
    for (int i = 0; i < s_order_count; i++)
        clamp_rect(&s_wins[s_order[i]]);
    s_dirty = true;
}

/* -------------------------------------------------------------- apps */

int td_app_register(const td_app_t *app)
{
    if (!app || s_app_count >= TD_MAX_APPS)
        return -1;
    s_apps[s_app_count++] = app;
    return 0;
}

int td_app_count(void)
{
    return s_app_count;
}

int td_wm_windows(td_window_t **out, int max)
{
    int n = 0;
    for (int i = 0; i < TD_MAX_WINDOWS && n < max; i++)
        if (s_wins[i].used && !(s_wins[i].flags & TD_WIN_MODAL))
            out[n++] = &s_wins[i];
    return n;
}

const td_app_t *td_app_get(int index)
{
    return (index >= 0 && index < s_app_count) ? s_apps[index] : NULL;
}

/* Windows an app opens while launching get its glyph on the taskbar. A
 * window whose widgets did not all fit in the shared pool (TD_MAX_WIDGETS),
 * or that got no tick timer (TD_MAX_TIMERS), would be missing parts or never
 * update: it is closed again and the user is told why, as when no window
 * slot was left for it (TD_MAX_WINDOWS). */
static void launch_app(const td_app_t *app)
{
    bool was_open[TD_MAX_WINDOWS];
    for (int i = 0; i < TD_MAX_WINDOWS; i++)
        was_open[i] = s_wins[i].used;
    s_win_refused = false;
    const char *prev = s_launch_icon;
    s_launch_icon = app->icon;
    app->launch();
    s_launch_icon = prev;

    bool closed = s_win_refused;     /* no window slot left for it */
    for (int i = 0; i < TD_MAX_WINDOWS; i++)
    {
        if (s_wins[i].used && !was_open[i] && s_wins[i].incomplete)
        {
            td_win_close(&s_wins[i]);
            closed = true;
        }
    }
    if (closed)
        td_msgbox(app->name, "Too many windows are open. Close one, then try again.", "OK", NULL, NULL);
}

bool td_app_launch(const char *name)
{
    for (int i = 0; i < s_app_count; i++)
    {
        if (strcmp(s_apps[i]->name, name) == 0)
        {
            launch_app(s_apps[i]);
            return true;
        }
    }
    return false;
}

/* ------------------------------------------------------ popup menus */

/* One popup at a time: the start menu or a right-click (context) menu.
 * Labels are copied; "-" is a separator line. */
#define MENU_MAX   20
#define MENU_LABEL 28

static struct
{
    int count;
    char labels[MENU_MAX][MENU_LABEL];
    const char *glyphs[MENU_MAX];   /* start menu (medium/large): app glyphs, NULL for others */
    bool has_glyphs;
    int row_h;                      /* rows per item: 1, or 2 for the large start menu */
    td_rect_t rect;
    bool is_start;          /* the start menu (highlights [Start]) */
    td_menu_fn fn;
    void *user;
} s_menu;

static bool menu_is_sep(int i)
{
    return strcmp(s_menu.labels[i], "-") == 0;
}

static td_rect_t menu_rect(void)
{
    return s_menu.rect;
}

static int menu_item_rows(int i)
{
    return menu_is_sep(i) ? 1 : s_menu.row_h;
}

/* Screen row of item i's first line. */
static int menu_item_y(int i)
{
    int y = s_menu.rect.y + 1;
    for (int j = 0; j < i; j++)
        y += menu_item_rows(j);
    return y;
}

/* Item on screen row y, or -1. */
static int menu_index_at(int y)
{
    int top = s_menu.rect.y + 1;
    for (int i = 0; i < s_menu.count; i++)
    {
        int h = menu_item_rows(i);
        if (y >= top && y < top + h)
            return i;
        top += h;
    }
    return -1;
}

/* Next selectable item from i in direction dir (wrapping). */
static int menu_step(int i, int dir)
{
    for (int n = 0; n < s_menu.count; n++)
    {
        i = (i + dir + s_menu.count) % s_menu.count;
        if (!menu_is_sep(i))
            return i;
    }
    return 0;
}

static void menu_open_at(int x, int y, const char *const *items, const char *const *glyphs, int count,
                         int row_h, td_menu_fn fn, void *user, bool is_start)
{
    if (count > MENU_MAX)
        count = MENU_MAX;
    int w = 12;
    for (int i = 0; i < count; i++)
    {
        snprintf(s_menu.labels[i], MENU_LABEL, "%s", items[i]);
        s_menu.glyphs[i] = glyphs ? glyphs[i] : NULL;
        int len = td_utf8_len(s_menu.labels[i]) + 4 + (glyphs ? 3 : 0);
        if (len > w)
            w = len;
    }
    s_menu.count = count;
    s_menu.has_glyphs = glyphs != NULL;
    s_menu.row_h = row_h;
    int desk = td_wm_desktop_rows();
    int h = 2;
    for (int i = 0; i < count; i++)
        h += menu_item_rows(i);
    if (h > desk && row_h > 1)
    {      /* too tall for the screen: one row per item */
        s_menu.row_h = 1;
        h = count + 2;
    }
    if (w > s_cols)
        w = s_cols;
    if (h > desk)
        h = desk;
    x = clamp(x, 0, s_cols - w);
    y = clamp(y, 0, desk - h);        /* keep it above the taskbar */

    s_menu.rect = td_rect(x, y, w, h);
    s_menu.is_start = is_start;
    s_menu.fn = fn;
    s_menu.user = user;
    s_menu_open = true;
    s_menu_sel = menu_step(-1, 1);
    s_dirty = true;
}

void td_menu_popup(int x, int y, const char *const *items, int count,
                   td_menu_fn fn, void *user)
{
    menu_open_at(x, y, items, NULL, count, 1, fn, user, false);
}

void td_menu_close(void)
{
    s_menu_open = false;
    s_dirty = true;
}

bool td_menu_is_open(void)
{
    return s_menu_open;
}

static void menu_activate(int i)
{
    if (i < 0 || i >= s_menu.count || menu_is_sep(i))
        return;
    td_menu_fn fn = s_menu.fn;
    void *user = s_menu.user;
    s_menu_open = false;          /* close first: fn may open another menu */
    s_dirty = true;
    if (fn)
        fn(i, user);
}

/* The start menu: every app, then the extra entries. */
static void start_menu_chosen(int i, void *user)
{
    (void)user;
    int extra = i - s_app_count - 1;       /* after the separator */
    if (i < s_app_count)
        launch_app(s_apps[i]);
    else if (extra >= 0 && extra < s_start_extra_count)
        s_start_extra[extra].fn();
    else if (extra == s_start_extra_count)
        td_full_redraw();
    else if (extra == s_start_extra_count + 1)
        td_quit();
}

void td_wm_add_start_item(const char *label, void (*fn)(void))
{
    if (!label || !fn || s_start_extra_count >= START_EXTRA_MAX)
        return;
    s_start_extra[s_start_extra_count].label = label;
    s_start_extra[s_start_extra_count].fn = fn;
    s_start_extra_count++;
}

void td_wm_set_kiosk(bool on, bool (*hook)(const td_event_t *ev))
{
    s_kiosk = on;
    s_kiosk_hook = hook;
    s_menu_open = false;
    for (int i = 0; i < TD_MAX_WINDOWS; i++)
    {
        td_window_t *w = &s_wins[i];
        if (!w->used || (w->flags & TD_WIN_MODAL))
            continue;
        /* On: everything covers the screen. Off: back to windows. */
        td_win_set_fullscreen(w, on);
        if (!on)
            w->flags &= (uint16_t)~TD_WIN_HIDDEN;
    }
    s_dirty = true;
}

bool td_wm_kiosk(void)
{
    return s_kiosk;
}

void td_wm_set_user_label(const char *label)
{
    snprintf(s_user_label, sizeof(s_user_label), "%s", label ? label : "");
    s_dirty = true;
}

static td_ui_size_t valid_size(td_ui_size_t s)
{
    return s == TD_UI_SMALL || s == TD_UI_LARGE ? s : TD_UI_MEDIUM;
}

void td_wm_set_icon_size(td_ui_size_t size)
{
    s_icon_size = valid_size(size);
    s_icon_sel = -1;
    s_dirty = true;
}
td_ui_size_t td_wm_icon_size(void)
{
    return s_icon_size;
}

void td_wm_set_start_menu_size(td_ui_size_t size)
{
    s_menu_size = valid_size(size);
    s_dirty = true;
}
td_ui_size_t td_wm_start_menu_size(void)
{
    return s_menu_size;
}

void td_wm_set_taskbar_size(td_ui_size_t size)
{
    size = valid_size(size);
    if (size == s_bar_size)
        return;
    s_bar_size = size;
    s_menu_open = false;
    td_wm_set_screen_size(s_cols, s_rows);    /* the desktop got taller or shorter */
}
td_ui_size_t td_wm_taskbar_size(void)
{
    return s_bar_size;
}

const char *td_ui_size_name(td_ui_size_t size)
{
    return size == TD_UI_SMALL ? "Small" : size == TD_UI_LARGE ? "Large"
                                                               : "Medium";
}

void td_wm_toggle_start_menu(void)
{
    if (s_kiosk)
        return;
    if (s_menu_open && s_menu.is_start)
    {
        td_menu_close();
        return;
    }
    const char *items[MENU_MAX], *glyphs[MENU_MAX];
    int n = 0;
    for (int i = 0; i < s_app_count && n < MENU_MAX - 3 - s_start_extra_count; i++)
    {
        glyphs[n] = s_apps[i]->icon;
        items[n++] = s_apps[i]->name;
    }
    glyphs[n] = NULL;
    items[n++] = "-";
    for (int i = 0; i < s_start_extra_count; i++)
    {
        glyphs[n] = NULL;
        items[n++] = s_start_extra[i].label;
    }
    glyphs[n] = NULL;
    items[n++] = "Redraw screen";
    glyphs[n] = NULL;
    items[n++] = "Exit";
    bool with_glyphs = s_menu_size != TD_UI_SMALL;
    menu_open_at(0, s_rows, items, with_glyphs ? glyphs : NULL, n, s_menu_size == TD_UI_LARGE ? 2 : 1,
                 start_menu_chosen, NULL, true);
}

static void draw_menu(void)
{
    const td_theme_t *t = td_theme();
    td_rect_t r = menu_rect();
    td_fill(r, ' ', t->menu_fg, t->menu_bg);
    td_box(r, TD_BOX_SINGLE, t->menu_fg, t->menu_bg);
    td_shadow(r);
    for (int i = 0; i < s_menu.count; i++)
    {
        int y = menu_item_y(i);
        int ih = menu_item_rows(i);
        if (y + ih > r.y + r.h - 1)
            break;
        if (menu_is_sep(i))
        {
            td_putc(r.x, y, 0x251C, t->menu_fg, t->menu_bg, 0);
            for (int x = r.x + 1; x < r.x + r.w - 1; x++)
                td_putc(x, y, 0x2500, t->menu_fg, t->menu_bg, 0);
            td_putc(r.x + r.w - 1, y, 0x2524, t->menu_fg, t->menu_bg, 0);
            continue;
        }
        bool sel = (i == s_menu_sel);
        uint8_t fg = sel ? t->menu_select_fg : t->menu_fg;
        uint8_t bg = sel ? t->menu_select_bg : t->menu_bg;
        td_fill(td_rect(r.x + 1, y, r.w - 2, ih), ' ', fg, bg);
        int tx = r.x + 2;
        if (s_menu.has_glyphs)
        {
            if (s_menu.glyphs[i])
                td_textn(tx, y, s_menu.glyphs[i], 2, fg, bg, TD_BOLD);
            tx += 3;
        }
        td_textn(tx, y, s_menu.labels[i], r.x + r.w - 2 - tx, fg, bg, 0);
    }
}

static void menu_key(const td_event_t *ev)
{
    switch (ev->key)
    {
    case TD_KEY_UP:
        s_menu_sel = menu_step(s_menu_sel, -1);
        break;
    case TD_KEY_DOWN:
        s_menu_sel = menu_step(s_menu_sel, 1);
        break;
    case TD_KEY_HOME:
        s_menu_sel = menu_step(-1, 1);
        break;
    case TD_KEY_END:
        s_menu_sel = menu_step(s_menu.count, -1);
        break;
    case TD_KEY_ENTER:
        menu_activate(s_menu_sel);
        break;
    case TD_KEY_ESC:
    case TD_KEY_F10:
        s_menu_open = false;
        break;
    default:
        break;
    }
    s_dirty = true;
}

/* Returns true if the menu consumed the mouse event. A click outside
 * closes the menu and then goes on to whatever is under it. */
static bool menu_mouse(const td_event_t *ev)
{
    td_rect_t r = menu_rect();
    if (!td_rect_contains(r, ev->x, ev->y))
    {
        if (ev->action == TD_MOUSE_PRESS)
        {
            bool was_start = s_menu.is_start;
            s_menu_open = false;
            s_dirty = true;
            /* A click on [Start] while the start menu is open just closes it. */
            return was_start && in_taskbar(ev->y) && ev->x < TASKBAR_START_W;
        }
        return false;
    }
    int i = menu_index_at(ev->y);
    if (i < 0 || i >= s_menu.count || menu_is_sep(i))
        return true;
    if (s_menu_sel != i)
    {            /* the highlight follows the mouse */
        s_menu_sel = i;
        s_dirty = true;
    }
    if (ev->action == TD_MOUSE_PRESS && (ev->button == TD_BUTTON_LEFT || ev->button == TD_BUTTON_RIGHT))
        menu_activate(i);
    return true;
}

/* ------------------------------------------------- window menus */

enum
{
    WA_MAXIMIZE,
    WA_MINIMIZE,
    WA_FULLSCREEN,
    WA_CLOSE
};
static td_window_t *s_menu_win;
static int s_win_actions[MENU_MAX];

static void window_menu_chosen(int i, void *user)
{
    (void)user;
    td_window_t *w = s_menu_win;
    if (!td_win_is_open(w))
        return;
    switch (s_win_actions[i])
    {
    case WA_MAXIMIZE:
        if (w->flags & TD_WIN_FULLSCREEN)
            td_win_set_fullscreen(w, false);
        else
            td_win_toggle_maximize(w);
        break;
    case WA_MINIMIZE:
        td_win_minimize(w);
        break;
    case WA_FULLSCREEN:
        td_win_set_fullscreen(w, true);
        break;
    case WA_CLOSE:
        td_win_request_close(w);
        break;
    default:
        break;
    }
}

/* Right-click on a title bar or taskbar button. */
static void window_menu(td_window_t *w, int x, int y)
{
    if (s_kiosk)
        return;
    const char *items[6];
    int n = 0;
    if (w->flags & TD_WIN_RESIZABLE)
    {
        bool big = (w->flags & (TD_WIN_MAXIMIZED | TD_WIN_FULLSCREEN)) != 0;
        s_win_actions[n] = WA_MAXIMIZE;
        items[n++] = big ? "Restore" : "Maximize";
    }
    if (!(w->flags & TD_WIN_MODAL))
    {
        s_win_actions[n] = WA_MINIMIZE;
        items[n++] = "Minimize";
    }
    if ((w->flags & TD_WIN_RESIZABLE) && !(w->flags & TD_WIN_FULLSCREEN))
    {
        s_win_actions[n] = WA_FULLSCREEN;
        items[n++] = "Full screen (F11)";
    }
    if (w->flags & TD_WIN_CLOSABLE)
    {
        if (n > 0)
        {
            s_win_actions[n] = -1;
            items[n++] = "-";
        }
        s_win_actions[n] = WA_CLOSE;
        items[n++] = "Close";
    }
    if (n == 0)
        return;
    s_menu_win = w;
    td_menu_popup(x, y, items, n, window_menu_chosen, NULL);
}

/* ----------------------------------------------------------- taskbar */

/* Date and time (from the clock provider); the uptime is shown by System
 * Monitor and About. Returns false when there is no clock to show. */
static const td_clock_provider_t *s_clock;
static int s_clock_x0 = -1, s_clock_x1 = -1;

void td_wm_set_clock(const td_clock_provider_t *clock)
{
    s_clock = clock;
    s_dirty = true;
}

static bool format_clock(char *buf, int cap)
{
    buf[0] = '\0';
    return s_clock && s_clock->text && s_clock->text(buf, cap) && buf[0];
}

/* Network indicator: "LAN", "Wi-Fi" with 1..3 signal bars, or "Offline".
 * Draws right-aligned so that it ends at x_end; returns its start column. */
static int draw_tray_net(int x_end, int y)
{
    const td_theme_t *t = td_theme();
    const td_net_ops_t *net = td_sysinfo()->net;
    s_tray_net_x0 = s_tray_net_x1 = -1;
    if (!net || !net->status)
        return x_end;

    td_net_status_t st;
    memset(&st, 0, sizeof(st));
    net->status(&st);
    char text[24];
    int bars = 0;
    if (st.wifi_up)
    {
        bars = st.rssi >= -60 ? 3 : st.rssi >= -72 ? 2
                                                   : 1;
        snprintf(text, sizeof(text), "%sWi-Fi ", st.eth_up ? "LAN " : "");
    }
    else
    {
        snprintf(text, sizeof(text), "%s", st.eth_up ? "LAN" : "Offline");
    }
    int w = td_utf8_len(text) + (st.wifi_up ? 3 : 0) + 2;
    int x = x_end - w;
    uint8_t fg = st.wifi_up || st.eth_up ? t->taskbar_fg : t->dim, bg = t->taskbar_bg;
    bool hot = mouse_over(x, y, w);
    if (hot)
        hover_colours(&fg, &bg);
    td_fill(td_rect(x, y, w, 1), ' ', fg, bg);
    int cx = x + 1 + td_text(x + 1, y, text, fg, bg, 0);
    if (st.wifi_up)
    {
        uint8_t on = hot ? fg : (uint8_t)(t->taskbar_bg == 7 ? 2 : 10);   /* green bars */
        for (int i = 0; i < 3; i++)
            td_putc(cx + i, y, i < bars ? 0x2588 : 0x2591, i < bars ? on : fg, bg, 0);
    }
    s_tray_net_x0 = x;
    s_tray_net_x1 = x + w;
    return x;
}

static void draw_taskbar(void)
{
    const td_theme_t *t = td_theme();
    int h = taskbar_h();
    int y = s_rows - h;                       /* top row of the taskbar */
    td_fill(td_rect(0, y, s_cols, h), ' ', t->taskbar_fg, t->taskbar_bg);

    bool start_open = s_menu_open && s_menu.is_start;
    uint8_t sfg = start_open ? t->taskbar_active_fg : t->taskbar_fg;
    uint8_t sbg = start_open ? t->taskbar_active_bg : t->taskbar_bg;
    if (!start_open && mouse_over_bar(0, TASKBAR_START_W))
        hover_colours(&sfg, &sbg);
    td_fill(td_rect(0, y, TASKBAR_START_W, h), ' ', sfg, sbg);
    td_text(0, y, "[Start]", sfg, sbg, TD_BOLD);

    /* Clock: the time only (small), time and date (medium), or the time
     * over the date (large). */
    char clock[32], tm[16], dt[24];
    bool parts = s_clock && s_clock->parts && s_clock->parts(tm, sizeof(tm), dt, sizeof(dt));
    int clock_x = s_cols - 1;
    s_clock_x0 = s_clock_x1 = -1;
    bool have_clock = format_clock(clock, sizeof(clock));
    if (have_clock)
    {
        const char *l1 = clock, *l2 = NULL;
        if (parts && s_bar_size == TD_UI_SMALL)
            l1 = tm;
        if (parts && s_bar_size == TD_UI_LARGE)
        {
            l1 = tm;
            l2 = dt;
        }
        int len = td_utf8_len(l1);
        if (l2 && td_utf8_len(l2) > len)
            len = td_utf8_len(l2);
        int w = len + 2;
        clock_x = s_cols - w;
        uint8_t fg = t->taskbar_fg, bg = t->taskbar_bg;
        if (s_clock->click && mouse_over_bar(clock_x, w))
            hover_colours(&fg, &bg);
        td_fill(td_rect(clock_x, y, w, h), ' ', fg, bg);
        td_text(clock_x + 1 + (len - td_utf8_len(l1)), y, l1, fg, bg, 0);
        if (l2)
            td_text(clock_x + 1 + (len - td_utf8_len(l2)), y + 1, l2, fg, bg, 0);
        s_clock_x0 = clock_x;
        s_clock_x1 = clock_x + w;
    }
    clock_x = draw_tray_net(clock_x, y);
    if (s_user_label[0] && s_bar_size != TD_UI_SMALL)
    {
        int len = td_utf8_len(s_user_label);
        clock_x -= len + 2;
        td_text(clock_x + 1, y, s_user_label, t->taskbar_fg, t->taskbar_bg, TD_BOLD);
    }

    /* One button per window, in creation (pool) order so they don't jump
     * around when the focus changes. */
    int max_title = s_bar_size == TD_UI_SMALL ? 8 : s_bar_size == TD_UI_LARGE ? 18
                                                                              : 14;
    td_window_t *focused = td_win_focused();
    int x = TASKBAR_START_W + 1;
    s_task_btn_count = 0;
    for (int i = 0; i < TD_MAX_WINDOWS; i++)
    {
        td_window_t *w = &s_wins[i];
        if (!w->used || (w->flags & TD_WIN_MODAL))
            continue;
        bool glyph = s_bar_size == TD_UI_LARGE && w->icon;
        int len = td_utf8_len(w->title);
        if (len > max_title)
            len = max_title;
        int bw = len + 2 + (glyph ? 3 : 0);
        if (x + bw > clock_x - 1)
            break;

        bool active = (w == focused);
        bool hidden = (w->flags & TD_WIN_HIDDEN) != 0;
        uint8_t fg = active ? t->taskbar_active_fg : (hidden ? t->dim : t->taskbar_fg);
        uint8_t bg = active ? t->taskbar_active_bg : t->taskbar_bg;
        if (!active && mouse_over_bar(x, bw))
            hover_colours(&fg, &bg);
        td_fill(td_rect(x, y, bw, h), ' ', fg, bg);
        int tx = x + 1;
        if (glyph)
        {
            td_textn(tx, y, w->icon, 2, fg, bg, TD_BOLD);
            tx += 3;
        }
        td_textn(tx, y, w->title, len, fg, bg, 0);
        if (h > 1)
        {
            /* A line under the button: bright for the active window, dim
             * for the others, none for minimised ones. */
            if (!hidden)
                for (int k = x + 1; k < x + bw - 1; k++)
                    td_putc(k, y + 1, 0x2500, active ? t->accent : t->dim, bg, active ? TD_BOLD : 0);
        }

        s_task_btn[s_task_btn_count].x0 = x;
        s_task_btn[s_task_btn_count].x1 = x + bw;
        s_task_btn[s_task_btn_count].win = w;
        s_task_btn_count++;
        x += bw + 1;
    }
}

static void taskbar_click(int x)
{
    if (x >= s_clock_x0 && x < s_clock_x1 && s_clock && s_clock->click)
    {
        s_clock->click(TD_BUTTON_LEFT, x, s_rows - 1);
        return;
    }
    if (x >= s_tray_net_x0 && x < s_tray_net_x1)
    {
        td_app_launch("Network");
        return;
    }
    if (x < TASKBAR_START_W)
    {
        td_wm_toggle_start_menu();
        return;
    }
    for (int i = 0; i < s_task_btn_count; i++)
    {
        if (x < s_task_btn[i].x0 || x >= s_task_btn[i].x1)
            continue;
        td_window_t *w = s_task_btn[i].win;
        /* Clicking the focused window's button minimises it. */
        if (w == td_win_focused())
            td_win_minimize(w);
        else
            td_win_focus(w);
        return;
    }
}

/* Top-most visible window containing the cell, or NULL for the desktop. */
td_window_t *td_wm_window_at(int x, int y)
{
    for (int i = s_order_count - 1; i >= 0; i--)
    {
        td_window_t *w = &s_wins[s_order[i]];
        if (w->flags & TD_WIN_HIDDEN)
            continue;
        if (td_rect_contains(w->rect, x, y))
            return w;
    }
    return NULL;
}

/* ------------------------------------------------------ desktop icons */

void td_desktop_set_provider(const td_desktop_provider_t *provider)
{
    s_provider = provider;
    s_dirty = true;
}

static int provider_count(void)
{
    return (s_provider && s_provider->count) ? s_provider->count(s_provider->user) : 0;
}

static int icon_count(void)
{
    return s_app_count + provider_count();
}

static const char *icon_name(int i)
{
    if (i < s_app_count)
        return s_apps[i]->name;
    const char *l = s_provider->label ? s_provider->label(i - s_app_count, s_provider->user) : NULL;
    return l ? l : "?";
}

static const char *icon_glyph(int i)
{
    if (i < s_app_count)
        return s_apps[i]->icon;
    return s_provider->icon ? s_provider->icon(i - s_app_count, s_provider->user) : NULL;
}

/* The glyph colour a provider item asks for, or -1. */
static int icon_glyph_fg(int i)
{
    if (i < s_app_count || !s_provider->icon_fg)
        return -1;
    return s_provider->icon_fg(i - s_app_count, s_provider->user);
}

/* How many icons of a size fit on the desktop. */
static int icon_capacity(td_ui_size_t sz)
{
    int per = (td_wm_desktop_rows() - 1) / s_icon_slot_h[sz];
    int cols = (s_cols - 1) / s_icon_slot_w[sz];
    return (per < 1 ? 1 : per) * (cols < 1 ? 1 : cols);
}

/* The size in use: the chosen one, or smaller while the icons would not
 * fit on the screen. */
static td_ui_size_t icon_eff(void)
{
    td_ui_size_t sz = s_icon_size;
    int n = icon_count();
    while (sz != TD_UI_SMALL && icon_capacity(sz) < n)
        sz = sz == TD_UI_LARGE ? TD_UI_MEDIUM : TD_UI_SMALL;
    return sz;
}

static int icons_per_column(void)
{
    int n = (td_wm_desktop_rows() - 1) / icon_h();
    return n < 1 ? 1 : n;
}

static td_rect_t icon_rect(int i)
{
    int per = icons_per_column();
    bool small = icon_eff() == TD_UI_SMALL;
    int h = small ? 1 : icon_h() - 1;
    return td_rect(1 + (i / per) * icon_w(), 1 + (i % per) * icon_h(), icon_w() - (small ? 1 : 0), h);
}

/* Icon under the cell, or -1. Windows cover icons. */
static int icon_at(int x, int y)
{
    if (!td_desktop_icons() || td_wm_window_at(x, y))
        return -1;
    for (int i = 0; i < icon_count(); i++)
        if (td_rect_contains(icon_rect(i), x, y))
            return i;
    return -1;
}

/* Split an app or file name into at most two label lines of icon_w() - 1
 * columns, breaking at a space ("System Monitor" -> "System" / "Monitor").
 * Columns are code points, as the screen draws them; a character is never
 * cut in half. */
static void icon_label(const char *name, char *l1, char *l2, size_t cap)
{
    const int max = icon_w() - 1;
    l2[0] = '\0';
    if (td_utf8_len(name) <= max)
    {
        td_utf8_copy(l1, cap, name, INT_MAX);
        return;
    }
    const char *space = NULL, *p = name;
    int space_at = 0;
    for (int i = 0; i <= max; i++)
    {
        const char *at = p;
        uint32_t cp = td_utf8_next(&p);
        if (cp == 0)
            break;
        if (cp == ' ')
        {
            space = at;
            space_at = i;
        }
    }
    if (space)
    {
        td_utf8_copy(l1, cap, name, space_at);
        td_utf8_copy(l2, cap, space + 1, max);
    }
    else
    {
        td_utf8_copy(l1, cap, name, max);
        td_utf8_copy(l2, cap, td_utf8_skip(name, max), max);
    }
}

static void draw_label_line(int x, int y, const char *text, uint8_t fg, uint8_t bg, uint8_t attr)
{
    if (!text[0])
        return;
    int len = td_utf8_len(text);
    int lx = x + (icon_w() - len) / 2;
    td_fill(td_rect(lx - 1, y, len + 2, 1), ' ', fg, bg);
    td_text(lx, y, text, fg, bg, attr);
}

static void draw_icon(int i)
{
    const td_theme_t *t = td_theme();
    td_rect_t r = icon_rect(i);
    const char *name = icon_name(i);
    const char *icon = icon_glyph(i);
    bool selected = (i == s_icon_sel);
    bool hover = icon_at(s_mouse_x, s_mouse_y) == i;
    char glyph[16];
    if (icon)
        snprintf(glyph, sizeof(glyph), "%s", icon);
    else
    {
        /* The name's first character (whole, even if it is not ASCII) and a space. */
        int n = td_utf8_copy(glyph, sizeof(glyph) - 1, name, 1);
        glyph[n] = ' ';
        glyph[n + 1] = '\0';
    }

    uint8_t fg = t->icon_fg, bg = t->desktop_bg;
    if (hover || selected)
        hover_colours(&fg, &bg);
    uint8_t lfg = selected ? t->select_fg : t->icon_fg;
    uint8_t lbg = selected ? t->select_bg : t->desktop_bg;
    uint8_t attr = hover ? TD_UNDERLINE : 0;
    int own = hover || selected ? -1 : icon_glyph_fg(i);   /* a provider's glyph colour */

    td_ui_size_t sz = icon_eff();
    if (sz == TD_UI_SMALL)
    {
        /* One line: glyph and name, like a list. */
        if (!selected && hover)
        {
            lfg = fg;
            lbg = bg;
        }
        td_fill(r, ' ', lfg, lbg);
        td_textn(r.x + 1, r.y, glyph, 2, own >= 0 ? (uint8_t)own : lfg, lbg, TD_BOLD);
        td_textn(r.x + 4, r.y, name, r.w - 5, lfg, lbg, attr);
        return;
    }

    /* The glyph in a box centred in the slot: 4 x 3 cells, or 8 x 5 with a
     * double frame for large icons. */
    bool large = sz == TD_UI_LARGE;
    int bw = large ? 8 : 4, bh = large ? 5 : 3;
    td_rect_t box = td_rect(r.x + (icon_w() - bw) / 2, r.y, bw, bh);
    td_fill(box, ' ', fg, bg);
    td_box(box, large ? TD_BOX_DOUBLE : TD_BOX_SINGLE, fg, bg);
    td_textn(box.x + (bw - 2) / 2, box.y + bh / 2, glyph, 2, own >= 0 ? (uint8_t)own : fg, bg, TD_BOLD);

    char l1[ICON_LABEL_MAX], l2[ICON_LABEL_MAX];
    if (large)
    {
        /* One wider label line (15 columns fit every app name). */
        td_utf8_copy(l1, sizeof(l1), name, icon_w() - 1);
        draw_label_line(r.x, r.y + bh, l1, lfg, lbg, attr);
        return;
    }
    icon_label(name, l1, l2, sizeof(l1));
    draw_label_line(r.x, r.y + bh, l1, lfg, lbg, attr);
    draw_label_line(r.x, r.y + bh + 1, l2, lfg, lbg, attr);
}

static void launch_icon(int i)
{
    if (i < 0 || i >= icon_count())
        return;
    s_icon_click = -1;
    s_dirty = true;
    if (i < s_app_count)
        launch_app(s_apps[i]);
    else if (s_provider->open)
        s_provider->open(i - s_app_count, s_provider->user);
}

static int s_menu_icon;

static void app_icon_menu_chosen(int item, void *user)
{
    (void)item;
    (void)user;
    launch_icon(s_menu_icon);
}

/* Right-click on the desktop: the icon's menu, or the desktop menu. */
static void desktop_context(const td_event_t *ev)
{
    int i = icon_at(ev->x, ev->y);
    s_icon_sel = i;
    s_dirty = true;
    if (i >= 0 && i < s_app_count)
    {
        static const char *const items[] = {"Open"};
        s_menu_icon = i;
        td_menu_popup(ev->x, ev->y, items, 1, app_icon_menu_chosen, NULL);
    }
    else if (s_provider && s_provider->context)
    {
        s_provider->context(i >= 0 ? i - s_app_count : -1, ev->x, ev->y, s_provider->user);
    }
    else
    {
        td_wm_toggle_start_menu();
        s_menu.rect.x = clamp(ev->x, 0, s_cols - s_menu.rect.w);
        s_menu.rect.y = clamp(ev->y, 0, td_wm_desktop_rows() - s_menu.rect.h);
    }
}

/* A click selects an icon, a double-click opens its app. */
static void desktop_press(const td_event_t *ev)
{
    int i = icon_at(ev->x, ev->y);
    bool dbl = i >= 0 && i == s_icon_click && ev->time_ms - s_icon_click_ms < TD_DOUBLE_CLICK_MS;
    s_icon_sel = i;
    s_icon_click = i;
    s_icon_click_ms = ev->time_ms;
    s_menu_open = false;
    s_dirty = true;
    if (dbl)
    {
        launch_icon(i);
        return;
    }
    /* Files and folders on the desktop can be dragged. */
    if (i >= s_app_count && s_provider && s_provider->drag)
    {
        s_dnd.armed = true;
        s_dnd.icon = i;
        s_dnd.win = NULL;
        s_dnd.x0 = ev->x;
        s_dnd.y0 = ev->y;
        s_drag.mode = DRAG_DESKTOP;
    }
}

/* While no window has the focus, arrow keys move between icons and Enter
 * opens one. Returns true if the key was used. */
static bool desktop_key(const td_event_t *ev)
{
    if (!td_desktop_icons() || icon_count() == 0 || ev->mods)
        return false;
    int per = icons_per_column();
    int sel = s_icon_sel;
    switch (ev->key)
    {
    case TD_KEY_UP:
        sel = sel < 0 ? 0 : sel - 1;
        break;
    case TD_KEY_DOWN:
        sel = sel < 0 ? 0 : sel + 1;
        break;
    case TD_KEY_LEFT:
        sel = sel < 0 ? 0 : sel - per;
        break;
    case TD_KEY_RIGHT:
        sel = sel < 0 ? 0 : sel + per;
        break;
    case TD_KEY_ENTER:
        if (sel < 0)
            return false;
        launch_icon(sel);
        return true;
    default:
        return false;
    }
    s_icon_sel = clamp(sel, 0, icon_count() - 1);
    s_dirty = true;
    return true;
}

/* Identify what the mouse is over, so hover highlights only cost a frame
 * when the pointer moves onto something different. */
static uintptr_t hover_id(int x, int y)
{
    if (s_menu_open && td_rect_contains(menu_rect(), x, y))
        return 0x10000u + (uintptr_t)y;
    if (in_taskbar(y))
    {
        if (x < TASKBAR_START_W)
            return 0x20000u;
        if (x >= s_tray_net_x0 && x < s_tray_net_x1)
            return 0x2FFFFu;
        for (int i = 0; i < s_task_btn_count; i++)
            if (x >= s_task_btn[i].x0 && x < s_task_btn[i].x1)
                return 0x20001u + (uintptr_t)i;
        return 1;
    }
    td_window_t *w = td_wm_window_at(x, y);
    if (!w)
    {
        int i = icon_at(x, y);
        return i >= 0 ? 0x30000u + (uintptr_t)i : 2;
    }
    if (has_frame(w) && in_title(w, y))
    {
        if (x >= close_x(w) && x < close_x(w) + button_w())
            return (uintptr_t)w + 1;
        if (x >= min_x(w) && x < min_x(w) + button_w())
            return (uintptr_t)w + 2;
    }
    for (td_widget_t *wd = w->widgets; wd; wd = wd->next)
        if (wd->visible && wd->type == TD_WT_BUTTON && td_rect_contains(td_widget_rect(wd), x, y))
            return (uintptr_t)wd;
    return (uintptr_t)w;
}

static void update_hover(void)
{
    uintptr_t id = hover_id(s_mouse_x, s_mouse_y);
    if (id != s_hover_id)
    {
        s_hover_id = id;
        s_dirty = true;
    }
}

/* ---------------------------------------------------------- composing */

static void draw_window(td_window_t *w, bool focused)
{
    const td_theme_t *t = td_theme();
    td_rect_t r = w->rect;
    td_rect_t screen = td_rect(0, 0, s_cols, s_rows);

    if (has_frame(w))
    {
        td_shadow(r);
        td_fill(r, ' ', t->win_fg, t->win_bg);
        uint8_t ffg = focused ? t->frame_active_fg : t->frame_fg;
        td_box(r, focused ? TD_BOX_DOUBLE : TD_BOX_SINGLE, ffg, t->win_bg);

        /* Title bar: the top row(s) between the corners; the title and the
         * buttons' captions on its last row. */
        uint8_t tfg = focused ? t->title_fg : t->title_inactive_fg;
        uint8_t tbg = focused ? t->title_bg : t->title_inactive_bg;
        int th = title_h(), ty = r.y + th - 1, bw = button_w();
        td_fill(td_rect(r.x + 1, r.y, r.w - 2, th), ' ', tfg, tbg);
        int buttons_x = (w->flags & TD_WIN_MODAL) ? close_x(w) : min_x(w);
        td_textn(r.x + 2, ty, w->title, buttons_x - r.x - 3, tfg, tbg, focused ? TD_BOLD : 0);
        bool top = td_wm_window_at(s_mouse_x, s_mouse_y) == w;
        bool wide = bw > 3;
        if (!(w->flags & TD_WIN_MODAL))
        {
            uint8_t fg = tfg, bg = tbg;
            if (top && in_title(w, s_mouse_y) && s_mouse_x >= min_x(w) && s_mouse_x < min_x(w) + bw)
                hover_colours(&fg, &bg);
            td_fill(td_rect(min_x(w), r.y, bw, th), ' ', fg, bg);
            td_text(min_x(w), ty, wide ? "[ - ]" : "[-]", fg, bg, 0);
        }
        if (w->flags & TD_WIN_CLOSABLE)
        {
            bool hot = top && in_title(w, s_mouse_y) && s_mouse_x >= close_x(w) && s_mouse_x < close_x(w) + bw;
            uint8_t fg = hot ? t->close_hover_fg : tfg, bg = hot ? t->close_hover_bg : tbg;
            td_fill(td_rect(close_x(w), r.y, bw, th), ' ', fg, bg);
            td_text(close_x(w), ty, wide ? "[ x ]" : "[x]", fg, bg, hot ? TD_BOLD : 0);
        }

        /* Resize grip: the bottom-right corner (and more of the bottom edge
         * with larger frames), highlighted. */
        if ((w->flags & TD_WIN_RESIZABLE) && !(w->flags & TD_WIN_MAXIMIZED))
        {
            uint32_t edge = focused ? 0x2550 : 0x2500;
            for (int x = r.x + r.w - grip_w(); x < r.x + r.w - 1; x++)
                td_putc(x, r.y + r.h - 1, edge, tbg, t->win_bg, TD_BOLD);
            td_putc(r.x + r.w - 1, r.y + r.h - 1, focused ? 0x255D : 0x2518, tbg, t->win_bg, TD_BOLD);
        }
    }

    td_rect_t c = td_win_client(w);
    if (!has_frame(w))
        td_fill(c, ' ', t->win_fg, t->win_bg);
    td_draw_origin(c.x, c.y);
    td_draw_clip(c);
    if (w->on_draw)
        w->on_draw(w, c.w, c.h);
    td_widgets_draw(w);
    td_draw_origin(0, 0);
    td_draw_clip(screen);
}

void td_wm_compose(td_buffer_t *back)
{
    const td_theme_t *t = td_theme();
    if (back->cols != s_cols || back->rows != s_rows)
        td_buffer_init(back, s_cols, s_rows);
    td_draw_target(back);

    td_fill(td_rect(0, 0, s_cols, s_rows), td_desktop_pattern(), t->desktop_fg, t->desktop_bg);
    if (td_desktop_icons() && !s_kiosk)
        for (int i = 0; i < icon_count(); i++)
            draw_icon(i);

    td_window_t *focused = td_win_focused();
    for (int i = 0; i < s_order_count; i++)
    {
        td_window_t *w = &s_wins[s_order[i]];
        if (w->flags & TD_WIN_HIDDEN)
            continue;
        draw_window(w, w == focused);
    }
    if (taskbar_visible())
    {
        draw_taskbar();
        if (s_menu_open)
            draw_menu();
    }
    if (s_dnd.active)
    {
        /* The dragged item follows the pointer. */
        char label[64];
        snprintf(label, sizeof(label), " %s %s ", s_dnd.item.is_dir ? "[/" : "\xC2\xB6", s_dnd.item.name);
        int len = td_utf8_len(label);
        int x = clamp(s_mouse_x + 1, 0, s_cols - len);
        int y = clamp(s_mouse_y, 0, s_rows - 1);
        td_text(x, y, label, t->select_fg, t->select_bg, TD_BOLD);
    }
    if (s_pointer && s_mouse_x >= 0 && s_mouse_x < s_cols && s_mouse_y < s_rows)
    {
        /* The pointer: a triangle pointing up ('^' in ASCII mode), black on
         * light cells and white on dark ones, on the cell's own background. */
        td_cell_t *c = td_buffer_cell(back, s_mouse_x, s_mouse_y);
        if (c)
        {
            if (c->attr & TD_REVERSE)
                c->bg = c->fg;
            c->ch = 0x25B2;
            c->fg = colour_is_light(c->bg) ? 16 : 231;
            c->attr = TD_BOLD;
        }
    }
    s_dirty = false;
}

/* ---------------------------------------------------------- dispatch */

/* Deliver an event to a window's client area (coordinates made relative). */
static void client_event(td_window_t *w, const td_event_t *ev)
{
    td_event_t e = *ev;
    if (e.type == TD_EV_MOUSE)
    {
        td_rect_t c = td_win_client(w);
        e.x = (int16_t)(e.x - c.x);
        e.y = (int16_t)(e.y - c.y);
    }
    if (td_widgets_event(w, &e))
        return;
    if (td_win_is_open(w) && w->on_event)
        w->on_event(w, &e);
}


static void drag_motion(const td_event_t *ev)
{
    td_window_t *w = s_drag.win;
    if (!td_win_is_open(w))
    {
        s_drag.mode = DRAG_NONE;
        return;
    }
    if (s_drag.mode == DRAG_MOVE)
    {
        td_win_move(w, ev->x - s_drag.dx, ev->y - s_drag.dy);
    }
    else if (s_drag.mode == DRAG_RESIZE)
    {
        td_win_resize(w, ev->x - w->rect.x + 1, ev->y - w->rect.y + 1);
    }
    else
    {
        client_event(w, ev);
    }
}

/* Press on a window frame. Returns true if it was handled here. */
static bool frame_press(td_window_t *w, const td_event_t *ev)
{
    td_rect_t r = w->rect;
    if (!has_frame(w) || td_rect_contains(td_win_client(w), ev->x, ev->y))
        return false;

    if (in_title(w, ev->y))
    {
        if ((w->flags & TD_WIN_CLOSABLE) && ev->x >= close_x(w) && ev->x < close_x(w) + button_w())
        {
            td_win_request_close(w);
            return true;
        }
        if (!(w->flags & TD_WIN_MODAL) && ev->x >= min_x(w) && ev->x < min_x(w) + button_w())
        {
            td_win_minimize(w);
            return true;
        }
        if (s_title_click.win == w && ev->time_ms - s_title_click.time < TD_DOUBLE_CLICK_MS)
        {
            s_title_click.win = NULL;
            td_win_toggle_maximize(w);
            return true;
        }
        s_title_click.win = w;
        s_title_click.time = ev->time_ms;
        if ((w->flags & TD_WIN_MOVABLE) && !(w->flags & TD_WIN_MAXIMIZED))
        {
            s_drag.mode = DRAG_MOVE;
            s_drag.win = w;
            s_drag.dx = ev->x - r.x;
            s_drag.dy = ev->y - r.y;
        }
        return true;
    }
    bool grip = ev->y == r.y + r.h - 1 && ev->x >= r.x + r.w - grip_w();
    if (grip && (w->flags & TD_WIN_RESIZABLE) && !(w->flags & TD_WIN_MAXIMIZED))
    {
        s_drag.mode = DRAG_RESIZE;
        s_drag.win = w;
    }
    return true;
}

bool td_drag_active(void)
{
    return s_dnd.active;
}

/* Turn an armed press into a drag once the mouse has moved a little. */
static bool dnd_try_start(const td_event_t *ev)
{
    int moved = abs(ev->x - s_dnd.x0) + abs(ev->y - s_dnd.y0);
    if (moved < 2)
        return false;
    s_dnd.armed = false;
    bool ok = false;
    memset(&s_dnd.item, 0, sizeof(s_dnd.item));
    if (s_dnd.icon >= s_app_count && s_provider && s_provider->drag)
    {
        ok = s_provider->drag(s_dnd.icon - s_app_count, &s_dnd.item, s_provider->user);
    }
    else if (td_win_is_open(s_dnd.win) && s_dnd.win->on_drag_start)
    {
        td_rect_t c = td_win_client(s_dnd.win);
        ok = s_dnd.win->on_drag_start(s_dnd.win, s_dnd.x0 - c.x, s_dnd.y0 - c.y, &s_dnd.item);
    }
    s_dnd.active = ok;
    s_dirty = true;
    return ok;
}

/* Hand the dragged item to whatever is under the pointer. */
static void dnd_drop(int x, int y)
{
    if (in_taskbar(y))
        return;
    td_window_t *w = td_wm_window_at(x, y);
    if (w)
    {
        td_rect_t c = td_win_client(w);
        if (w->on_drop && td_rect_contains(c, x, y))
            w->on_drop(w, x - c.x, y - c.y, &s_dnd.item);
        return;
    }
    if (s_provider && s_provider->drop)
    {
        int i = icon_at(x, y);
        s_provider->drop(i >= s_app_count ? i - s_app_count : -1, &s_dnd.item, s_provider->user);
    }
}

static void dispatch_mouse(const td_event_t *ev)
{
    s_mouse_x = ev->x;
    s_mouse_y = ev->y;
    if (s_kiosk && s_kiosk_hook && s_kiosk_hook(ev))
        return;

    /* Plain movement (no button) only moves highlights around. */
    if (ev->action == TD_MOUSE_MOVE && s_drag.mode == DRAG_NONE)
    {
        if (s_menu_open)
            menu_mouse(ev);
        return;
    }
    s_dirty = true;

    if (s_menu_open && menu_mouse(ev))
        return;

    if (s_drag.mode != DRAG_NONE)
    {
        if (ev->action == TD_MOUSE_DRAG || ev->action == TD_MOUSE_MOVE)
        {
            if (s_dnd.active)
                return;               /* the item follows the mouse */
            if (s_dnd.armed && dnd_try_start(ev))
                return;
            if (s_drag.mode != DRAG_DESKTOP)
                drag_motion(ev);
            return;
        }
        if (ev->action == TD_MOUSE_RELEASE)
        {
            if (s_dnd.active)
                dnd_drop(ev->x, ev->y);
            else if (s_drag.mode == DRAG_CAPTURE && td_win_is_open(s_drag.win))
                client_event(s_drag.win, ev);
            memset(&s_dnd, 0, sizeof(s_dnd));
            s_drag.mode = DRAG_NONE;
            s_drag.win = NULL;
            return;
        }
    }

    if (in_taskbar(ev->y))
    {
        if (ev->action == TD_MOUSE_PRESS && ev->button == TD_BUTTON_LEFT)
            taskbar_click(ev->x);
        if (ev->action == TD_MOUSE_PRESS && ev->button == TD_BUTTON_RIGHT)
        {
            if (ev->x >= s_clock_x0 && ev->x < s_clock_x1 && s_clock && s_clock->click)
                s_clock->click(TD_BUTTON_RIGHT, ev->x, ev->y);
            for (int i = 0; i < s_task_btn_count; i++)
                if (ev->x >= s_task_btn[i].x0 && ev->x < s_task_btn[i].x1)
                    window_menu(s_task_btn[i].win, s_task_btn[i].x0, ev->y);
        }
        return;
    }

    td_window_t *w = td_wm_window_at(ev->x, ev->y);
    td_window_t *modal = modal_window();
    if (!w)
    {
        if (s_kiosk)
            return;                   /* no desktop icons or menus */
        if (!modal && ev->action == TD_MOUSE_PRESS && ev->button == TD_BUTTON_LEFT)
            desktop_press(ev);
        if (!modal && ev->action == TD_MOUSE_PRESS && ev->button == TD_BUTTON_RIGHT)
            desktop_context(ev);
        return;
    }
    if (modal && w != modal)
        return;

    bool wheel = ev->button == TD_BUTTON_WHEEL_UP || ev->button == TD_BUTTON_WHEEL_DOWN;
    if (ev->action == TD_MOUSE_PRESS && !wheel)
    {
        td_win_focus(w);
        if (ev->button == TD_BUTTON_LEFT && frame_press(w, ev))
            return;
        if (ev->button == TD_BUTTON_RIGHT && has_frame(w) && in_title(w, ev->y))
        {
            window_menu(w, ev->x, ev->y + 1);
            return;
        }
    }
    if (!td_rect_contains(td_win_client(w), ev->x, ev->y))
        return;
    if (ev->action == TD_MOUSE_PRESS && !wheel)
    {
        s_drag.mode = DRAG_CAPTURE;
        s_drag.win = w;
        if (ev->button == TD_BUTTON_LEFT && w->on_drag_start)
        {
            s_dnd.armed = true;
            s_dnd.icon = -1;
            s_dnd.win = w;
            s_dnd.x0 = ev->x;
            s_dnd.y0 = ev->y;
        }
    }
    client_event(w, ev);
}

/* Windows-key shortcuts (TD_MOD_SUPER, from a USB keyboard only):
 * Up maximizes, Down restores or minimizes, Left / Right fill that half of
 * the screen, D minimizes every window (again: brings them back), E opens
 * Files, R the Terminal, I Settings. */
static void super_key(const td_event_t *ev, td_window_t *w)
{
    bool sizable = w && (w->flags & TD_WIN_RESIZABLE) && has_frame(w);
    switch (ev->key)
    {
    case TD_KEY_UP:
        if (sizable && !(w->flags & TD_WIN_MAXIMIZED))
            td_win_toggle_maximize(w);
        break;
    case TD_KEY_DOWN:
        if (w && (w->flags & TD_WIN_MAXIMIZED))
            td_win_toggle_maximize(w);
        else if (w)
            td_win_minimize(w);
        break;
    case TD_KEY_LEFT:
    case TD_KEY_RIGHT:
        if (sizable)
        {
            if (w->flags & TD_WIN_MAXIMIZED)
                td_win_toggle_maximize(w);
            int half = s_cols / 2;
            td_win_move(w, ev->key == TD_KEY_LEFT ? 0 : s_cols - half, 0);
            td_win_resize(w, half, td_wm_desktop_rows());
        }
        break;
    case 'd':
    {
        bool shown = false;
        for (int i = 0; i < TD_MAX_WINDOWS; i++)
        {
            td_window_t *o = &s_wins[i];
            if (o->used && !(o->flags & (TD_WIN_HIDDEN | TD_WIN_MODAL)))
            {
                td_win_minimize(o);
                shown = true;
            }
        }
        for (int i = 0; !shown && i < s_order_count; i++)   /* all hidden: back */
            s_wins[s_order[i]].flags &= (uint16_t)~TD_WIN_HIDDEN;
        break;
    }
    case 'e':
        td_app_launch("Files");
        break;
    case 'r':
        td_app_launch("Terminal");
        break;
    case 'i':
        td_app_launch("Settings");
        break;
    default:
        break;
    }
    s_dirty = true;
}

static void dispatch_key(const td_event_t *ev)
{
    bool plain = ev->mods == 0;

    if (s_kiosk)
    {
        /* Only the application gets keys: no Start menu, window switching,
         * closing, full screen or Windows-key shortcuts. The hook sees
         * every key first (the way out of kiosk mode). */
        if (s_kiosk_hook && s_kiosk_hook(ev))
            return;
        bool global = (ev->mods & TD_MOD_SUPER) || (ev->key == TD_KEY_F10 && plain) || (ev->key == TD_KEY_F6 && plain) ||
                      (ev->key == TD_KEY_TAB && (ev->mods & TD_MOD_ALT)) ||
                      (ev->key == 'q' && ev->mods == TD_MOD_CTRL) ||
                      (ev->key == TD_KEY_F4 && ev->mods == TD_MOD_ALT) || (ev->key == TD_KEY_F11 && plain);
        td_window_t *kw = td_win_focused();
        if (global || !kw)
            return;
    }

    if (ev->key == TD_KEY_F10 && plain)
    {
        td_wm_toggle_start_menu();
        return;
    }
    if (s_menu_open)
    {
        menu_key(ev);
        return;
    }

    td_window_t *modal = modal_window();
    bool alt_tab = ev->key == TD_KEY_TAB && (ev->mods & TD_MOD_ALT);
    if ((ev->key == TD_KEY_F6 && plain) || alt_tab)
    {
        if (!modal && alt_tab && (ev->mods & TD_MOD_SHIFT))
            cycle_windows_back();
        else if (!modal)
            cycle_windows();
        return;
    }
    if (ev->key == 'l' && ev->mods == TD_MOD_CTRL)
        td_full_redraw();

    td_window_t *w = td_win_focused();
    if (ev->mods & TD_MOD_SUPER)
    {
        if (!modal)
            super_key(ev, w);
        return;
    }
    /* Ctrl+Q (or Alt+F4, where the terminal lets it through) closes the
     * focused window, as its [x] would: it may ask about unsaved work. */
    if ((ev->key == 'q' && ev->mods == TD_MOD_CTRL) || (ev->key == TD_KEY_F4 && ev->mods == TD_MOD_ALT))
    {
        if (w)
            td_win_request_close(w);
        return;
    }
    if (!w)
    {
        if (desktop_key(ev))
            return;
        if (ev->key == TD_KEY_ENTER || ev->key == TD_KEY_ESC)
            td_wm_toggle_start_menu();
        return;
    }
    if (ev->key == TD_KEY_F11 && plain && (w->flags & TD_WIN_RESIZABLE))
    {
        td_win_set_fullscreen(w, !(w->flags & TD_WIN_FULLSCREEN));
        return;
    }

    if (w->flags & TD_WIN_RAW_KEYS)
    {
        if (w->on_event)
            w->on_event(w, ev);
        return;
    }
    if (ev->key == 'l' && ev->mods == TD_MOD_CTRL)
        return;

    if (td_widgets_event(w, ev))
        return;
    if (td_win_is_open(w) && w->on_event && w->on_event(w, ev))
        return;

    if (ev->key == TD_KEY_TAB)
    {
        td_widgets_focus_next(w, (ev->mods & TD_MOD_SHIFT) ? -1 : 1);
        s_dirty = true;
    }
    else if (ev->key == TD_KEY_ESC && (w->flags & TD_WIN_MODAL))
    {
        td_win_request_close(w);
    }
}

/* Pasted text goes to the focused text box, else to the focused window
 * (the Editor inserts it, the Terminal types it). */
static void dispatch_paste(const td_event_t *ev)
{
    if (s_menu_open)
        return;
    td_window_t *w = td_win_focused();
    if (!w)
        return;
    if (!(w->flags & TD_WIN_RAW_KEYS) && td_widgets_paste(w))
        return;
    if (w->on_event)
        w->on_event(w, ev);
}

void td_wm_dispatch(const td_event_t *ev)
{
    switch (ev->type)
    {
    case TD_EV_KEY:
        dispatch_key(ev);
        s_dirty = true;   /* keys nearly always change something */
        break;
    case TD_EV_PASTE:
        dispatch_paste(ev);
        s_dirty = true;
        break;
    case TD_EV_MOUSE:
        if (s_pointer && (ev->x != s_mouse_x || ev->y != s_mouse_y))
            s_dirty = true;   /* the drawn pointer moves */
        dispatch_mouse(ev);   /* plain moves only redraw when the hover changes */
        update_hover();
        break;
    default:
        break;
    }
}
