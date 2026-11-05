/*
 * osk.c - the on-screen keyboard: keys drawn into part of a window and
 * sized to it (td_osk_*), used by Sign in, the kiosk's way out and the
 * On-screen keyboard app, which types into the window that has the focus.
 *
 * The keys sit on a grid 12 units wide and 5 rows high; a unit is as many
 * columns as the area allows (3 to 8) and a row as many lines (1 to 6),
 * so a large screen gets large keys. A key types when it is released on
 * the key it was pressed on: a finger that slides off types nothing. The
 * gap drawn between keys belongs to the key before it, so every cell of
 * the keyboard is some key. Shift works as on a phone: one tap shifts the
 * next character, a second tap within SHIFT_LOCK_MS locks it (CAPS), and
 * a tap on a locked Shift releases it.
 */
#include "td_apps.h"

#include <stdio.h>
#include <string.h>

#define UNITS         12
#define ROWS          5
#define SHIFT_LOCK_MS 400

/* Ten character keys per row, three sets. Together with Space they hold
 * every printable ASCII character. */
static const char *const s_lower[4] = {"1234567890", "qwertyuiop", "asdfghjkl'", "zxcvbnm,.-"};
static const char *const s_upper[4] = {"!@#$%^&*()", "QWERTYUIOP", "ASDFGHJKL\"", "ZXCVBNM;:_"};
static const char *const s_symbols[4] = {"1234567890", "!@#$%^&*()", "-_=+[]{}\\|", "/?<>`~;:.,"};

enum
{
    K_SHIFT = -10,
    K_SYM = -11,
};

typedef struct
{
    int row, unit, units;   /* place on the grid */
    int code;               /* what it types, or K_SHIFT / K_SYM / TD_OSK_* */
} key_t_;

#define KEY_COUNT (4 * 11 + 5)

/* Key i of the layout. */
static key_t_ key_at(const td_osk_t *k, int i)
{
    if (i < 40)
    {
        const char *const *set = k->sym ? s_symbols : k->shift ? s_upper
                                                               : s_lower;
        return (key_t_){i / 10, i % 10, 1, (unsigned char)set[i / 10][i % 10]};
    }
    static const int right[4] = {TD_KEY_BACKSPACE, TD_KEY_ENTER, K_SHIFT, K_SYM};
    if (i < 44)
        return (key_t_){i - 40, 10, 2, right[i - 40]};
    static const key_t_ bottom[5] = {
        {4, 0, 1, TD_KEY_LEFT},
        {4, 1, 1, TD_KEY_RIGHT},
        {4, 2, 6, ' '},
        {4, 8, 2, TD_OSK_CLEAR},
        {4, 10, 2, TD_OSK_HIDE},
    };
    key_t_ key = bottom[i - 44];
    if (key.code == TD_OSK_CLEAR && !k->password)
        key.code = TD_KEY_ESC;   /* Esc closes dialogs; Clear only empties a password */
    return key;
}

static int unit_w(const td_osk_t *k)
{
    int u = k->area.w / UNITS;
    return u < 1 ? 1 : u > 8 ? 8
                             : u;
}

static int row_h(const td_osk_t *k)
{
    int h = k->area.h / ROWS;
    return h < 1 ? 1 : h > 6 ? 6
                             : h;
}

/* The cells a key takes on the grid, its gap included (where a touch
 * counts), and the ones drawn: without the gap column between keys (from
 * 3 columns a unit) and the gap line between rows (from 2 lines a row). */
static td_rect_t key_cell(const td_osk_t *k, const key_t_ *key)
{
    int u = unit_w(k), h = row_h(k);
    int x0 = k->area.x + (k->area.w - UNITS * u) / 2, y0 = k->area.y + (k->area.h - ROWS * h) / 2;
    return td_rect(x0 + key->unit * u, y0 + key->row * h, key->units * u, h);
}

static td_rect_t key_rect(const td_osk_t *k, const key_t_ *key)
{
    td_rect_t r = key_cell(k, key);
    r.w -= unit_w(k) >= 3 ? 1 : 0;
    r.h -= row_h(k) >= 2 ? 1 : 0;
    return r;
}

static const char *key_label(const td_osk_t *k, const key_t_ *key, char *buf, int width)
{
    bool wide = width >= 6;
    switch (key->code)
    {
    case TD_KEY_BACKSPACE:
        return wide ? "\xE2\x86\x90 Del" : "Del";   /* U+2190 */
    case TD_KEY_ENTER:
        return k->enter ? k->enter : "Enter";
    case K_SHIFT:
        return k->shift == TD_OSK_SHIFT_LOCK ? "CAPS" : k->shift ? (wide ? "SHIFT" : "SHF")
                                                                 : (wide ? "Shift" : "Shf");
    case K_SYM:
        return k->sym ? "abc" : "#+=";
    case TD_KEY_LEFT:
        return "<";
    case TD_KEY_RIGHT:
        return ">";
    case ' ':
        return wide ? "Space" : "";
    case TD_OSK_CLEAR:
        return "Clear";
    case TD_KEY_ESC:
        return "Esc";
    case TD_OSK_HIDE:
        return "Hide";
    default:
        buf[0] = (char)key->code;
        buf[1] = 0;
        return buf;
    }
}

void td_osk_init(td_osk_t *k, td_rect_t area, const char *enter, bool password)
{
    *k = (td_osk_t){.area = area, .enter = enter, .password = password, .held = -1};
}

void td_osk_draw(const td_osk_t *k)
{
    const td_theme_t *t = td_theme();
    for (int i = 0; i < KEY_COUNT; i++)
    {
        key_t_ key = key_at(k, i);
        td_rect_t r = key_rect(k, &key);
        bool on = (key.code == K_SHIFT && k->shift) || (key.code == K_SYM && k->sym);
        uint8_t fg = i == k->held ? t->select_fg : t->button_fg, bg = i == k->held ? t->select_bg : t->button_bg;
        td_fill(r, ' ', fg, bg);
        char one[2];
        const char *label = key_label(k, &key, one, r.w);
        int len = td_utf8_len(label);
        if (len > r.w)
            len = r.w;
        td_textn(r.x + (r.w - len) / 2, r.y + (r.h - 1) / 2, label, len, on ? t->accent : fg, bg, TD_BOLD);
    }
}

static int key_under(const td_osk_t *k, int x, int y)
{
    for (int i = 0; i < KEY_COUNT; i++)
    {
        key_t_ key = key_at(k, i);
        if (td_rect_contains(key_cell(k, &key), x, y))
            return i;
    }
    return -1;
}

int td_osk_mouse(td_osk_t *k, const td_event_t *ev, bool *used)
{
    *used = false;
    if (ev->type != TD_EV_MOUSE || ev->button != TD_BUTTON_LEFT)
        return TD_OSK_NONE;
    int i = key_under(k, ev->x, ev->y);
    if (ev->action == TD_MOUSE_PRESS)
    {
        k->held = i;
        *used = i >= 0;
        return TD_OSK_NONE;
    }
    if (ev->action != TD_MOUSE_RELEASE)
    {
        *used = k->held >= 0;
        return TD_OSK_NONE;
    }
    int held = k->held;
    k->held = -1;
    *used = held >= 0;
    if (i < 0 || i != held)
        return TD_OSK_NONE;
    key_t_ key = key_at(k, i);
    if (key.code == K_SHIFT)
    {
        uint32_t now = td_millis();
        if (k->shift == TD_OSK_SHIFT_ONCE && now - k->shift_ms < SHIFT_LOCK_MS)
            k->shift = TD_OSK_SHIFT_LOCK;   /* a double tap */
        else
            k->shift = k->shift == TD_OSK_SHIFT_OFF ? TD_OSK_SHIFT_ONCE : TD_OSK_SHIFT_OFF;
        k->shift_ms = now;
        k->sym = false;
        return TD_OSK_NONE;
    }
    if (key.code == K_SYM)
    {
        k->sym = !k->sym;
        return TD_OSK_NONE;
    }
    if (i < 40 && k->shift == TD_OSK_SHIFT_ONCE && !k->sym)
        k->shift = TD_OSK_SHIFT_OFF;   /* one character, then lower case again */
    return key.code;
}

/* ------------------------------------------------- On-screen keyboard app */

static td_window_t *s_win;
static td_osk_t s_osk;
static bool s_auto;          /* opened by td_osk_follow_focus, not by the user */
static bool s_follow;        /* the setting: open for text fields */
static int s_timer = -1;

static void osk_layout(int w, int h)
{
    td_osk_init(&s_osk, td_rect(0, 0, w, h), "Enter", false);
}

static void osk_draw(td_window_t *win, int w, int h)
{
    (void)win;
    const td_theme_t *t = td_theme();
    td_fill(td_rect(0, 0, w, h), ' ', t->win_fg, t->win_bg);
    if (s_osk.area.w != w || s_osk.area.h != h)
    {
        td_osk_t was = s_osk;   /* keep Shift (also a lock) and the symbols */
        osk_layout(w, h);
        s_osk.shift = was.shift;
        s_osk.shift_ms = was.shift_ms;
        s_osk.sym = was.sym;
    }
    td_osk_draw(&s_osk);
}

static bool osk_event(td_window_t *win, const td_event_t *ev)
{
    bool used;
    int code = td_osk_mouse(&s_osk, ev, &used);
    if (used)
        td_win_invalidate(win);
    if (code == TD_OSK_HIDE)
    {
        td_win_close(win);
        return true;
    }
    if (code != TD_OSK_NONE && code != TD_OSK_CLEAR)
    {
        /* Typed as if on a keyboard: to the window that has the focus. */
        td_event_t key = {.type = TD_EV_KEY, .key = (uint32_t)code};
        td_wm_dispatch(&key);
    }
    return used;
}

static void osk_closed(td_window_t *win)
{
    (void)win;
    s_win = NULL;
    s_auto = false;
}

static void osk_open(bool automatic)
{
    if (td_win_is_open(s_win))
        return;
    const td_stats_t *st = td_stats();
    int rows = td_wm_desktop_rows();
    int w = st->cols - 2 < 98 ? st->cols - 2 : 98;
    int h = st->rows * 3 / 4 < 22 ? st->rows * 3 / 4 : 22;   /* 4 lines a key row from 30 rows */
    if (h < 7)
        h = 7;
    /* Below the text being typed into if it fits, else above it, else
     * below it and shorter: the text box stays in view. */
    int y = rows - h - 1, ty = -1;
    td_window_t *f = td_win_focused();
    if (f && f->focus)
        ty = td_win_client(f).y + f->focus->rect.y;
    if (ty >= y)
    {
        if (ty - 1 >= h)
            y = 0;
        else
        {
            y = ty + 2;
            h = rows - y - 1 >= 7 ? rows - y - 1 : 7;
        }
    }
    td_window_desc_t d = {
        .title = "On-screen keyboard",
        .rect = td_rect((st->cols - w) / 2, y, w, h),
        .min_w = 30,
        .min_h = 7,
        .flags = TD_WIN_MOVABLE | TD_WIN_RESIZABLE | TD_WIN_CLOSABLE | TD_WIN_NO_FOCUS,
        .on_draw = osk_draw,
        .on_event = osk_event,
        .on_close = osk_closed,
    };
    s_win = td_win_create(&d);
    s_auto = automatic && s_win;
    td_rect_t c = s_win ? td_win_client(s_win) : td_rect(0, 0, w, h);
    osk_layout(c.w, c.h);
}

static void launch(void)
{
    if (td_win_is_open(s_win))
        td_win_close(s_win);   /* the Start menu entry toggles it */
    else
        osk_open(false);
}

/* With the setting on, the keyboard opens while a text box has the focus
 * and closes again after, unless the user opened it. */
static void follow(void *user)
{
    (void)user;
    td_window_t *w = td_win_focused();
    bool text = w && w->focus && w->focus->type == TD_WT_TEXTBOX && !(w->flags & TD_WIN_HIDDEN);
    if (text && !td_win_is_open(s_win))
        osk_open(true);
    else if (!text && s_auto && td_win_is_open(s_win))
        td_win_close(s_win);
}

void td_osk_follow_focus(bool on)
{
    s_follow = on;
    if (on && s_timer < 0)
        s_timer = td_timer_start(250, true, follow, NULL, td_millis());
    else if (!on && s_timer >= 0)
    {
        td_timer_stop(s_timer);
        s_timer = -1;
        if (s_auto && td_win_is_open(s_win))
            td_win_close(s_win);
    }
}

bool td_osk_follows_focus(void)
{
    return s_follow;
}

static const td_app_t s_app = {"On-screen keyboard", launch, "KB"};

void td_osk_register(void)
{
    td_app_register(&s_app);
}
