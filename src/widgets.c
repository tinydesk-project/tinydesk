/*
 * widgets.c - label, button, checkbox, textbox, list, progress bar,
 * scrollbar and the modal message box.
 *
 * Widgets come from one static pool and are linked into their window.
 * All coordinates here are relative to the window's client area.
 */
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "tinydesk/td.h"

static td_widget_t s_pool[TD_MAX_WIDGETS];

/* A text box may keep its text in a buffer of the app's own (td_textbox_set_buffer). */
#define TXT(w) ((w)->ext ? (w)->ext : (w)->text)
#define CAP(w) ((w)->ext ? (w)->ext_cap : (int)sizeof((w)->text))
static td_widget_t *s_pressed;    /* button or scrollbar held by the mouse */

/* ------------------------------------------------------------ helpers */

static int clamp(int v, int lo, int hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

static uint8_t pick(int custom, uint8_t fallback)
{
    return custom == TD_COLOR_DEFAULT ? fallback : (uint8_t)custom;
}

/* Rectangle relative to the client area with <= 0 sizes resolved. */
static td_rect_t local_rect(const td_widget_t *w)
{
    td_rect_t c = td_win_client(w->win);
    td_rect_t r = w->rect;
    if (r.x < 0)
        r.x += c.w;
    if (r.y < 0)
        r.y += c.h;
    if (r.w <= 0)
        r.w = c.w + r.w - r.x;
    if (r.h <= 0)
        r.h = c.h + r.h - r.y;
    if (r.w < 0)
        r.w = 0;
    if (r.h < 0)
        r.h = 0;
    return r;
}

td_rect_t td_widget_rect(const td_widget_t *w)
{
    td_rect_t c = td_win_client(w->win);
    td_rect_t r = local_rect(w);
    r.x += c.x;
    r.y += c.y;
    return r;
}

/* Widgets only a message box may use, so it can always say why another
 * window did not open. */
#define DIALOG_RESERVE 4

static td_widget_t *alloc_widget(td_window_t *win, td_widget_type_t type, td_rect_t rect, bool focusable)
{
    if (!td_win_is_open(win))
        return NULL;
    if (!(win->flags & TD_WIN_MODAL))
    {
        int free_count = 0;
        for (int i = 0; i < TD_MAX_WIDGETS; i++)
            free_count += !s_pool[i].used;
        if (free_count <= DIALOG_RESERVE)
        {
            win->incomplete = true;
            return NULL;
        }
    }
    for (int i = 0; i < TD_MAX_WIDGETS; i++)
    {
        td_widget_t *w = &s_pool[i];
        if (w->used)
            continue;
        memset(w, 0, sizeof(*w));
        w->used = true;
        w->type = (uint8_t)type;
        w->rect = rect;
        w->win = win;
        w->visible = true;
        w->focusable = focusable;
        w->fg = TD_COLOR_DEFAULT;
        w->bg = TD_COLOR_DEFAULT;

        /* Append so Tab order follows creation order. */
        td_widget_t **link = &win->widgets;
        while (*link)
            link = &(*link)->next;
        *link = w;
        if (focusable && !win->focus)
            win->focus = w;
        td_wm_invalidate();
        return w;
    }
    win->incomplete = true;         /* the pool is shared by every window */
    return NULL;
}

void td_widgets_free(td_window_t *win)
{
    for (td_widget_t *w = win->widgets; w; w = w->next)
    {
        if (s_pressed == w)
            s_pressed = NULL;
        free(w->marks);
        w->marks = NULL;
        w->used = false;
    }
    win->widgets = NULL;
    win->focus = NULL;
}

void td_widgets_reset(void)
{
    for (int i = 0; i < TD_MAX_WIDGETS; i++)
        free(s_pool[i].marks);
    memset(s_pool, 0, sizeof(s_pool));
    s_pressed = NULL;
}

/* ------------------------------------------------------------ creation */

td_widget_t *td_label(td_window_t *win, int x, int y, int width, const char *text)
{
    td_widget_t *w = alloc_widget(win, TD_WT_LABEL, td_rect(x, y, width, 1), false);
    if (w)
        td_widget_set_text(w, text);
    return w;
}

td_widget_t *td_button(td_window_t *win, int x, int y, const char *caption,
                       td_widget_fn fn, void *user)
{
    int width = td_utf8_len(caption ? caption : "") + 4;
    td_widget_t *w = alloc_widget(win, TD_WT_BUTTON, td_rect(x, y, width, 1), true);
    if (!w)
        return NULL;
    td_widget_set_text(w, caption);
    w->on_activate = fn;
    w->user = user;
    return w;
}

td_widget_t *td_checkbox(td_window_t *win, int x, int y, const char *caption,
                         bool checked, td_widget_fn fn, void *user)
{
    int width = td_utf8_len(caption ? caption : "") + 4;
    td_widget_t *w = alloc_widget(win, TD_WT_CHECKBOX, td_rect(x, y, width, 1), true);
    if (!w)
        return NULL;
    td_widget_set_text(w, caption);
    w->value = checked ? 1 : 0;
    w->on_activate = fn;
    w->user = user;
    return w;
}

td_widget_t *td_textbox(td_window_t *win, int x, int y, int width, int maxlen,
                        td_widget_fn fn, void *user)
{
    td_widget_t *w = alloc_widget(win, TD_WT_TEXTBOX, td_rect(x, y, width, 1), true);
    if (!w)
        return NULL;
    w->maxlen = clamp(maxlen, 1, TD_TEXT_MAX - 1);
    w->on_activate = fn;
    w->user = user;
    return w;
}

td_widget_t *td_list(td_window_t *win, td_rect_t rect, td_list_item_fn get_item,
                     td_widget_fn fn, void *user)
{
    td_widget_t *w = alloc_widget(win, TD_WT_LIST, rect, true);
    if (!w)
        return NULL;
    w->get_item = get_item;
    w->on_activate = fn;
    w->user = user;
    return w;
}

td_widget_t *td_progress(td_window_t *win, int x, int y, int width)
{
    return alloc_widget(win, TD_WT_PROGRESS, td_rect(x, y, width, 1), false);
}

td_widget_t *td_scrollbar(td_window_t *win, int x, int y, int height, td_widget_t *target)
{
    td_widget_t *w = alloc_widget(win, TD_WT_SCROLLBAR, td_rect(x, y, 1, height), false);
    if (w)
        w->target = target;
    return w;
}

/* ------------------------------------------------------------ setters */

void td_widget_set_text(td_widget_t *w, const char *text)
{
    if (!w)
        return;
    td_utf8_copy(TXT(w), (size_t)CAP(w), text, INT_MAX); /* cut to fit, never inside a character */
    if (w->type == TD_WT_TEXTBOX)
    {
        w->value = (int)strlen(TXT(w));   /* cursor to the end */
        w->scroll = 0;
    }
    if (w->type == TD_WT_BUTTON)
        w->rect.w = td_utf8_len(TXT(w)) + 4;   /* "[ caption ]" */
    td_wm_invalidate();
}

const char *td_widget_text(const td_widget_t *w)
{
    return w ? TXT(w) : "";
}

void td_textbox_set_buffer(td_widget_t *w, char *buf, int cap)
{
    if (!w || w->type != TD_WT_TEXTBOX || !buf || cap < 2)
        return;
    buf[cap - 1] = '\0';
    w->ext = buf;
    w->ext_cap = cap;
    w->maxlen = cap - 1;
    w->value = (int)strlen(buf);
    w->scroll = 0;
    td_wm_invalidate();
}

void td_widget_printf(td_widget_t *w, const char *fmt, ...)
{
    if (!w)
        return;
    char buf[TD_TEXT_MAX];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (strcmp(buf, TXT(w)) != 0)
        td_widget_set_text(w, buf);
}

void td_widget_set_align(td_widget_t *w, td_align_t align)
{
    if (w)
        w->align = (uint8_t)align;
}

void td_widget_set_color(td_widget_t *w, int fg, int bg)
{
    if (!w)
        return;
    w->fg = fg;
    w->bg = bg;
    td_wm_invalidate();
}

void td_widget_set_visible(td_widget_t *w, bool visible)
{
    if (!w)
        return;
    w->visible = visible;
    if (!visible && w->win->focus == w)
        td_widgets_focus_next(w->win, 1);
    td_wm_invalidate();
}

void td_widget_focus(td_widget_t *w)
{
    if (!w || !w->focusable || !w->visible)
        return;
    w->win->focus = w;
    td_wm_invalidate();
}

bool td_checkbox_get(const td_widget_t *w)
{
    return w && w->value != 0;
}

void td_checkbox_set(td_widget_t *w, bool checked)
{
    if (!w)
        return;
    w->value = checked ? 1 : 0;
    td_wm_invalidate();
}

void td_progress_set(td_widget_t *w, int percent)
{
    if (!w)
        return;
    percent = clamp(percent, 0, 100);
    if (w->value != percent)
    {
        w->value = percent;
        td_wm_invalidate();
    }
}

static int list_rows(const td_widget_t *w)
{
    return local_rect(w).h;
}

static void list_clamp_scroll(td_widget_t *w)
{
    int rows = list_rows(w);
    int max_scroll = w->count - rows;
    if (max_scroll < 0)
        max_scroll = 0;
    w->scroll = clamp(w->scroll, 0, max_scroll);
}

static void list_ensure_visible(td_widget_t *w)
{
    int rows = list_rows(w);
    if (w->value < w->scroll)
        w->scroll = w->value;
    if (rows > 0 && w->value >= w->scroll + rows)
        w->scroll = w->value - rows + 1;
    list_clamp_scroll(w);
}

/* ------------------------------------------------- several selected items */

static bool marks_room(td_widget_t *w, int count)
{
    if (count <= w->marks_cap)
        return true;
    int cap = (count + 63) & ~63;
    uint8_t *m = realloc(w->marks, (size_t)cap / 8);
    if (!m)
        return false;
    memset(m + w->marks_cap / 8, 0, (size_t)(cap - w->marks_cap) / 8);
    w->marks = m;
    w->marks_cap = cap;
    return true;
}

void td_list_set_multi(td_widget_t *w, bool on)
{
    if (!w || w->type != TD_WT_LIST)
        return;
    w->multi = on;
    if (!on)
    {
        free(w->marks);
        w->marks = NULL;
        w->marks_cap = 0;
    }
    else if (marks_room(w, w->count > 0 ? w->count : 1) && w->count > 0)
        td_list_mark(w, w->value, true);
    w->anchor = w->value;
    td_wm_invalidate();
}

bool td_list_marked(const td_widget_t *w, int index)
{
    if (!w || index < 0 || index >= w->count)
        return false;
    if (!w->multi || !w->marks)
        return index == w->value;
    return index < w->marks_cap && (w->marks[index / 8] >> (index % 8)) & 1;
}

void td_list_mark(td_widget_t *w, int index, bool on)
{
    if (!w || !w->multi || index < 0 || index >= w->count || !marks_room(w, index + 1))
        return;
    if (on)
        w->marks[index / 8] |= (uint8_t)(1u << (index % 8));
    else
        w->marks[index / 8] &= (uint8_t) ~(1u << (index % 8));
    td_wm_invalidate();
}

void td_list_mark_all(td_widget_t *w, bool on)
{
    if (!w || !w->multi || !marks_room(w, w->count > 0 ? w->count : 1))
        return;
    memset(w->marks, 0, (size_t)w->marks_cap / 8);
    for (int i = 0; on && i < w->count; i++)
        w->marks[i / 8] |= (uint8_t)(1u << (i % 8));
    td_wm_invalidate();
}

int td_list_marked_count(const td_widget_t *w)
{
    int n = 0;
    for (int i = 0; w && i < w->count; i++)
        n += td_list_marked(w, i);
    return n;
}

/* Only the items from a to b (either order) selected. */
static void mark_range(td_widget_t *w, int a, int b, bool keep)
{
    if (!keep)
        td_list_mark_all(w, false);
    if (a > b)
    {
        int t = a;
        a = b;
        b = t;
    }
    for (int i = a; i <= b; i++)
        td_list_mark(w, i, true);
}

void td_widgets_cancel_press(void)
{
    if (s_pressed)
    {
        s_pressed->band = false;
        s_pressed->pressed = false;
    }
    s_pressed = NULL;
}

int td_type_ahead(int count, int from, uint32_t key, const char *(*name)(int index, void *user), void *user)
{
    if (key > 127 || !isalnum((int)key) || count <= 0)
        return -1;
    int want = tolower((int)key);
    for (int n = 1; n <= count; n++)
    {
        int i = ((from < 0 ? -1 : from) + n) % count;
        const char *s = name(i, user);
        while (s && *s && !isalnum((unsigned char)*s))
            s++;
        if (s && *s && tolower((unsigned char)*s) == want)
            return i;
    }
    return -1;
}

static const char *list_name(int i, void *user)
{
    td_widget_t *w = user;
    int fg = -1;
    return w->get_item ? w->get_item(w, i, &fg, w->user) : NULL;
}

bool td_widgets_keeps_drag(td_window_t *win, int x, int y)
{
    td_widget_t *w = s_pressed;
    return w && w->win == win && w->type == TD_WT_LIST && w->multi && w->band && td_rect_contains(local_rect(w), x, y);
}

void td_list_set_count(td_widget_t *w, int count)
{
    if (!w)
        return;
    if (w->multi && count != w->count)
    {
        /* Other items: what was selected is no longer the same. */
        w->count = count < 0 ? 0 : count;
        if (marks_room(w, w->count > 0 ? w->count : 1))
            memset(w->marks, 0, (size_t)w->marks_cap / 8);
    }
    w->count = count < 0 ? 0 : count;
    if (w->value >= w->count)
        w->value = w->count > 0 ? w->count - 1 : 0;
    list_clamp_scroll(w);
    td_wm_invalidate();
}

int td_list_selected(const td_widget_t *w)
{
    return (w && w->count > 0) ? w->value : -1;
}

void td_list_set_view(td_widget_t *w, int selected, int scroll)
{
    if (!w)
        return;
    w->value = selected < 0 || w->count <= 0 ? -1 : clamp(selected, 0, w->count - 1);
    w->scroll = scroll;
    list_clamp_scroll(w);
    td_wm_invalidate();
}

void td_list_select(td_widget_t *w, int index)
{
    if (!w || w->count == 0)
        return;
    w->value = clamp(index, 0, w->count - 1);
    list_ensure_visible(w);
    td_wm_invalidate();
}

/* ------------------------------------------------------------ drawing */

static void draw_label(const td_widget_t *w, td_rect_t r, const td_theme_t *t)
{
    uint8_t fg = pick(w->fg, t->win_fg);
    uint8_t bg = pick(w->bg, t->win_bg);
    int len = td_utf8_len(TXT(w));
    int x = r.x;
    if (w->align == TD_ALIGN_CENTER)
        x += (r.w - len) / 2;
    else if (w->align == TD_ALIGN_RIGHT)
        x += r.w - len;
    if (x < r.x)
        x = r.x;
    td_fill(r, ' ', fg, bg);
    td_textn(x, r.y, TXT(w), r.x + r.w - x, fg, bg, 0);
}

/* True when the mouse is over the widget and nothing covers it there. */
static bool hovered(const td_widget_t *w)
{
    int mx, my;
    if (!td_wm_mouse_pos(&mx, &my))
        return false;
    return td_rect_contains(td_widget_rect(w), mx, my) && td_wm_window_at(mx, my) == w->win;
}

static void draw_button(const td_widget_t *w, td_rect_t r, bool focused, const td_theme_t *t)
{
    /* Focused and hovered buttons use the focus colours. */
    bool lit = focused || hovered(w);
    uint8_t fg = lit ? t->focus_fg : pick(w->fg, t->button_fg);
    uint8_t bg = lit ? t->focus_bg : pick(w->bg, t->button_bg);
    uint8_t attr = w->pressed ? TD_REVERSE : (focused ? TD_BOLD : 0);
    td_fill(r, ' ', fg, bg);
    td_putc(r.x, r.y, '[', fg, bg, attr);
    td_textn(r.x + 2, r.y, TXT(w), r.w - 4, fg, bg, attr);
    td_putc(r.x + r.w - 1, r.y, ']', fg, bg, attr);
}

static void draw_checkbox(const td_widget_t *w, td_rect_t r, bool focused, const td_theme_t *t)
{
    uint8_t fg = pick(w->fg, t->win_fg);
    uint8_t bg = pick(w->bg, t->win_bg);
    td_text(r.x, r.y, w->value ? "[x]" : "[ ]", fg, bg, 0);
    uint8_t cfg = focused ? t->focus_fg : fg;
    uint8_t cbg = focused ? t->focus_bg : bg;
    td_textn(r.x + 4, r.y, TXT(w), r.w - 4, cfg, cbg, 0);
}

static void draw_textbox(const td_widget_t *w, td_rect_t r, bool focused, const td_theme_t *t)
{
    uint8_t fg = pick(w->fg, t->input_fg);
    uint8_t bg = pick(w->bg, t->input_bg);
    td_fill(r, ' ', fg, bg);
    if (w->secret)
    {
        int len = (int)strlen(TXT(w)) - w->scroll;
        for (int i = 0; i < len && i < r.w; i++)
            td_putc(r.x + i, r.y, '*', fg, bg, 0);
    }
    else
    {
        td_textn(r.x, r.y, TXT(w) + w->scroll, r.w, fg, bg, 0);
    }
    if (focused)
    {
        int cx = r.x + w->value - w->scroll;
        uint32_t ch = TXT(w)[w->value] ? (uint8_t)(w->secret ? '*' : TXT(w)[w->value]) : ' ';
        td_putc(cx, r.y, ch, fg, bg, TD_REVERSE);
    }
}

static void draw_list(td_widget_t *w, td_rect_t r, bool focused, const td_theme_t *t)
{
    list_clamp_scroll(w);   /* the list may have grown since the last scroll */
    uint8_t fg = pick(w->fg, t->input_fg);
    uint8_t bg = pick(w->bg, t->input_bg);
    td_fill(r, ' ', fg, bg);
    for (int row = 0; row < r.h; row++)
    {
        int i = w->scroll + row;
        if (i >= w->count)
            break;
        int item_fg = TD_COLOR_DEFAULT;
        const char *text = w->get_item ? w->get_item((td_widget_t *)w, i, &item_fg, w->user) : "";
        uint8_t ifg = pick(item_fg, fg);
        uint8_t ibg = bg;
        bool marked = w->multi ? td_list_marked(w, i) : i == w->value;
        if (marked)
        {
            ifg = focused ? t->select_fg : ifg;
            ibg = focused ? t->select_bg : t->dim;
            td_fill(td_rect(r.x, r.y + row, r.w, 1), ' ', ifg, ibg);
        }
        td_textn(r.x + 1, r.y + row, text ? text : "", r.w - 1, ifg, ibg, 0);
        if (w->multi && focused && i == w->value)
            td_putc(r.x, r.y + row, 0x25BA, marked ? ifg : t->accent, ibg, TD_BOLD);   /* the current item */
    }
}

static void draw_progress(const td_widget_t *w, td_rect_t r, const td_theme_t *t)
{
    int filled = r.w * w->value / 100;
    uint8_t fg = pick(w->fg, t->accent);
    uint8_t bg = pick(w->bg, t->win_bg);
    for (int x = 0; x < r.w; x++)
        td_putc(r.x + x, r.y, x < filled ? 0x2588 : 0x2591, fg, bg, 0);
}

static void draw_scrollbar(const td_widget_t *w, td_rect_t r, const td_theme_t *t)
{
    const td_widget_t *list = w->target;
    int pos = 0, total = 0, page = r.h;
    if (list && list->used)
    {
        pos = list->scroll;
        total = list->count;
        page = list_rows(list);
    }
    td_draw_vscroll(r.x, r.y, r.h, pos, total, page, pick(w->fg, t->win_fg), pick(w->bg, t->win_bg));
}

void td_widgets_draw(td_window_t *win)
{
    const td_theme_t *t = td_theme();
    td_window_t *focused_win = td_win_focused();
    for (td_widget_t *w = win->widgets; w; w = w->next)
    {
        if (!w->visible)
            continue;
        td_rect_t r = local_rect(w);
        bool focused = (win->focus == w) && (win == focused_win);
        switch (w->type)
        {
        case TD_WT_LABEL:
            draw_label(w, r, t);
            break;
        case TD_WT_BUTTON:
            draw_button(w, r, focused, t);
            break;
        case TD_WT_CHECKBOX:
            draw_checkbox(w, r, focused, t);
            break;
        case TD_WT_TEXTBOX:
            draw_textbox(w, r, focused, t);
            break;
        case TD_WT_LIST:
            draw_list(w, r, focused, t);
            break;
        case TD_WT_PROGRESS:
            draw_progress(w, r, t);
            break;
        case TD_WT_SCROLLBAR:
            draw_scrollbar(w, r, t);
            break;
        default:
            break;
        }
    }
}

/* ------------------------------------------------------------- focus */

void td_widgets_focus_next(td_window_t *win, int dir)
{
    /* Collect the focusable widgets in order, then step through them. */
    td_widget_t *list[TD_MAX_WIDGETS];
    int n = 0, cur = -1;
    for (td_widget_t *w = win->widgets; w; w = w->next)
    {
        if (!w->focusable || !w->visible)
            continue;
        if (w == win->focus)
            cur = n;
        list[n++] = w;
    }
    if (n == 0)
    {
        win->focus = NULL;
        return;
    }
    int next = (cur < 0) ? 0 : (cur + (dir > 0 ? 1 : n - 1)) % n;
    win->focus = list[next];
}

/* ------------------------------------------------------------ events */

static void activate(td_widget_t *w)
{
    if (w->on_activate)
        w->on_activate(w, w->user);
    td_wm_invalidate();
}

static void checkbox_toggle(td_widget_t *w)
{
    w->value = !w->value;
    activate(w);
}

static bool textbox_key(td_widget_t *w, const td_event_t *ev)
{
    int len = (int)strlen(TXT(w));
    uint32_t k = ev->key;
    if (ev->mods & (TD_MOD_CTRL | TD_MOD_ALT))
        return false;

    if (k >= 0x20 && k < 0x7F)
    {
        if (len >= w->maxlen)
            return true;
        memmove(TXT(w) + w->value + 1, TXT(w) + w->value, (size_t)(len - w->value + 1));
        TXT(w)
        [w->value++] = (char)k;
    }
    else if (k == TD_KEY_BACKSPACE)
    {
        if (w->value == 0)
            return true;
        memmove(TXT(w) + w->value - 1, TXT(w) + w->value, (size_t)(len - w->value + 1));
        w->value--;
    }
    else if (k == TD_KEY_DELETE)
    {
        if (w->value < len)
            memmove(TXT(w) + w->value, TXT(w) + w->value + 1, (size_t)(len - w->value));
    }
    else if (k == TD_KEY_LEFT)
    {
        if (w->value > 0)
            w->value--;
    }
    else if (k == TD_KEY_RIGHT)
    {
        if (w->value < len)
            w->value++;
    }
    else if (k == TD_KEY_HOME)
    {
        w->value = 0;
    }
    else if (k == TD_KEY_END)
    {
        w->value = len;
    }
    else if (k == TD_KEY_ENTER)
    {
        activate(w);
        return true;
    }
    else
    {
        return false;
    }

    /* Horizontal scroll keeps the cursor inside the box. */
    int width = local_rect(w).w;
    if (w->value < w->scroll)
        w->scroll = w->value;
    if (width > 0 && w->value >= w->scroll + width)
        w->scroll = w->value - width + 1;
    td_wm_invalidate();
    return true;
}

/* Paste into the focused text box: the first line, printable ASCII only
 * (what the box can show), up to its length. */
bool td_widgets_paste(td_window_t *win)
{
    td_widget_t *w = win ? win->focus : NULL;
    if (!w || w->type != TD_WT_TEXTBOX || !w->visible)
        return false;
    int n = 0;
    const char *text = td_paste_text(&n);
    int len = (int)strlen(TXT(w));
    for (int i = 0; i < n && text[i] != '\r' && text[i] != '\n'; i++)
    {
        char c = text[i] == '\t' ? ' ' : text[i];
        if ((unsigned char)c < 0x20 || (unsigned char)c >= 0x7F)
            continue;
        if (len >= w->maxlen)
            break;
        memmove(TXT(w) + w->value + 1, TXT(w) + w->value, (size_t)(len - w->value + 1));
        TXT(w)
        [w->value++] = c;
        len++;
    }
    int width = local_rect(w).w;
    if (width > 0 && w->value >= w->scroll + width)
        w->scroll = w->value - width + 1;
    td_wm_invalidate();
    return true;
}

/* Moving the current item: one item selected, or with Shift the range
 * from the anchor. */
static void list_move(td_widget_t *w, int to, const td_event_t *ev)
{
    td_list_select(w, to);
    if (!w->multi)
        return;
    if (ev->mods & TD_MOD_SHIFT)
        mark_range(w, w->anchor, w->value, false);
    else if (!(ev->mods & TD_MOD_CTRL))
    {
        mark_range(w, w->value, w->value, false);
        w->anchor = w->value;
    }
}

static bool list_key(td_widget_t *w, const td_event_t *ev)
{
    int rows = list_rows(w);
    if (ev->mods == 0 && ev->key != ' ')
    {
        int i = td_type_ahead(w->count, w->value, ev->key, list_name, w);
        if (i >= 0)
        {
            td_list_select(w, i);
            if (w->multi)
            {
                mark_range(w, i, i, false);
                w->anchor = i;
            }
            return true;
        }
    }
    if (w->multi)
    {
        if (ev->key == 'a' && ev->mods == TD_MOD_CTRL)
        {
            td_list_mark_all(w, true);
            return true;
        }
        if (ev->key == ' ' && !(ev->mods & ~TD_MOD_CTRL) && w->count > 0)
        {
            td_list_mark(w, w->value, !td_list_marked(w, w->value));
            w->anchor = w->value;
            return true;
        }
        switch (ev->key)
        {
        case TD_KEY_UP:
            list_move(w, w->value - 1, ev);
            return true;
        case TD_KEY_DOWN:
            list_move(w, w->value + 1, ev);
            return true;
        case TD_KEY_PGUP:
            list_move(w, w->value - rows, ev);
            return true;
        case TD_KEY_PGDN:
            list_move(w, w->value + rows, ev);
            return true;
        case TD_KEY_HOME:
            list_move(w, 0, ev);
            return true;
        case TD_KEY_END:
            list_move(w, w->count - 1, ev);
            return true;
        default:
            break;
        }
    }
    switch (ev->key)
    {
    case TD_KEY_UP:
        td_list_select(w, w->value - 1);
        return true;
    case TD_KEY_DOWN:
        td_list_select(w, w->value + 1);
        return true;
    case TD_KEY_PGUP:
        td_list_select(w, w->value - rows);
        return true;
    case TD_KEY_PGDN:
        td_list_select(w, w->value + rows);
        return true;
    case TD_KEY_HOME:
        td_list_select(w, 0);
        return true;
    case TD_KEY_END:
        td_list_select(w, w->count - 1);
        return true;
    case TD_KEY_ENTER:
        if (w->count > 0)
            activate(w);
        return true;
    default:
        return false;
    }
}

static bool widget_key(td_widget_t *w, const td_event_t *ev)
{
    bool enter_or_space = ev->key == TD_KEY_ENTER || ev->key == ' ';
    switch (w->type)
    {
    case TD_WT_BUTTON:
        if (!enter_or_space)
            return false;
        activate(w);
        return true;
    case TD_WT_CHECKBOX:
        if (!enter_or_space)
            return false;
        checkbox_toggle(w);
        return true;
    case TD_WT_TEXTBOX:
        return textbox_key(w, ev);
    case TD_WT_LIST:
        return list_key(w, ev);
    default:
        return false;
    }
}

static void scroll_target(td_widget_t *list, int delta)
{
    if (!list || !list->used)
        return;
    list->scroll += delta;
    list_clamp_scroll(list);
    td_wm_invalidate();
}

/* Map a y position on a scrollbar track to a scroll offset. */
static void scrollbar_track(td_widget_t *sb, int y)
{
    td_widget_t *list = sb->target;
    td_rect_t r = local_rect(sb);
    int track = r.h - 2;
    if (!list || track <= 0)
        return;
    int max_scroll = list->count - list_rows(list);
    if (max_scroll <= 0)
        return;
    int pos = clamp(y - r.y - 1, 0, track - 1);
    list->scroll = track > 1 ? pos * max_scroll / (track - 1) : 0;
    list_clamp_scroll(list);
    td_wm_invalidate();
}

static void widget_press(td_widget_t *w, const td_event_t *ev, td_rect_t r)
{
    if (w->focusable)
        w->win->focus = w;
    switch (w->type)
    {
    case TD_WT_BUTTON:
        w->pressed = true;
        s_pressed = w;
        break;
    case TD_WT_CHECKBOX:
        checkbox_toggle(w);
        break;
    case TD_WT_TEXTBOX:
    {
        int len = (int)strlen(TXT(w));
        w->value = clamp(w->scroll + ev->x - r.x, 0, len);
        break;
    }
    case TD_WT_LIST:
    {
        int i = w->scroll + ev->y - r.y;
        if (i >= w->count)
            break;
        bool dbl = (i == w->value) && (ev->time_ms - w->last_click_ms < TD_DOUBLE_CLICK_MS) && !(ev->mods & (TD_MOD_CTRL | TD_MOD_SHIFT));
        td_list_select(w, i);
        if (w->multi)
        {
            if (ev->mods & TD_MOD_CTRL)
            {
                td_list_mark(w, i, !td_list_marked(w, i));
                w->anchor = i;
            }
            else if (ev->mods & TD_MOD_SHIFT)
                mark_range(w, w->anchor, i, false);
            else
            {
                /* A press on a selected item keeps the selection: it may
                 * be dragged somewhere. Otherwise just this one, and
                 * dragging over the rows selects them. */
                w->band = !td_list_marked(w, i);
                if (w->band)
                    mark_range(w, i, i, false);
                w->anchor = i;
            }
            if (ev->mods & (TD_MOD_CTRL | TD_MOD_SHIFT))
                w->band = false;
            s_pressed = w;
        }
        w->last_click_ms = ev->time_ms;
        if (dbl)
        {
            w->last_click_ms = 0;
            activate(w);
        }
        break;
    }
    case TD_WT_SCROLLBAR:
    {
        int page = w->target ? list_rows(w->target) : 1;
        if (ev->y == r.y)
            scroll_target(w->target, -1);
        else if (ev->y == r.y + r.h - 1)
            scroll_target(w->target, 1);
        else
        {
            scrollbar_track(w, ev->y);
            s_pressed = w;   /* keep dragging the thumb */
        }
        (void)page;
        break;
    }
    default:
        break;
    }
    td_wm_invalidate();
}

static bool widgets_mouse(td_window_t *win, const td_event_t *ev)
{
    /* A held button or scrollbar receives motion and the release. */
    if (s_pressed && s_pressed->win == win)
    {
        td_widget_t *w = s_pressed;
        td_rect_t r = local_rect(w);
        if (ev->action == TD_MOUSE_DRAG && w->type == TD_WT_LIST && !w->band)
            return true;   /* a drag of the selection: the window manager's */
        if (ev->action == TD_MOUSE_DRAG && w->type == TD_WT_LIST)
        {
            /* Dragging over the rows selects them (scrolling at the edges). */
            if (ev->y < r.y)
                scroll_target(w, -1);
            else if (ev->y >= r.y + r.h)
                scroll_target(w, 1);
            int i = clamp(w->scroll + ev->y - r.y, 0, w->count - 1);
            if (w->count > 0 && (i != w->value || !td_list_marked(w, i)))
            {
                w->value = i;
                mark_range(w, w->anchor, i, false);
            }
            td_wm_invalidate();
            return true;
        }
        if (ev->action == TD_MOUSE_RELEASE && w->type == TD_WT_LIST)
        {
            /* A click on one of several selected items, not dragged: that one. */
            if (!w->band && !(ev->mods & (TD_MOD_CTRL | TD_MOD_SHIFT)) && td_list_marked_count(w) > 1 &&
                td_rect_contains(r, ev->x, ev->y))
                mark_range(w, w->value, w->value, false);
            w->band = false;
            s_pressed = NULL;
            return true;
        }
        if (ev->action == TD_MOUSE_DRAG)
        {
            if (w->type == TD_WT_SCROLLBAR)
                scrollbar_track(w, ev->y);
            else
                w->pressed = td_rect_contains(r, ev->x, ev->y);
            td_wm_invalidate();
            return true;
        }
        if (ev->action == TD_MOUSE_RELEASE)
        {
            s_pressed = NULL;
            if (w->type == TD_WT_BUTTON && w->pressed)
            {
                w->pressed = false;
                activate(w);
            }
            w->pressed = false;
            td_wm_invalidate();
            return true;
        }
    }

    for (td_widget_t *w = win->widgets; w; w = w->next)
    {
        if (!w->visible)
            continue;
        td_rect_t r = local_rect(w);
        if (!td_rect_contains(r, ev->x, ev->y))
            continue;

        if (ev->button == TD_BUTTON_WHEEL_UP || ev->button == TD_BUTTON_WHEEL_DOWN)
        {
            int delta = ev->button == TD_BUTTON_WHEEL_UP ? -3 : 3;
            if (w->type == TD_WT_LIST)
                scroll_target(w, delta);
            else if (w->type == TD_WT_SCROLLBAR)
                scroll_target(w->target, delta);
            else
                continue;
            return true;
        }
        if (ev->action == TD_MOUSE_PRESS && ev->button == TD_BUTTON_LEFT)
        {
            if (w->type == TD_WT_LABEL || w->type == TD_WT_PROGRESS)
                continue;
            widget_press(w, ev, r);
            return true;
        }
    }
    return false;
}

bool td_widgets_event(td_window_t *win, const td_event_t *ev)
{
    if (ev->type == TD_EV_MOUSE)
        return widgets_mouse(win, ev);
    if (ev->type != TD_EV_KEY)
        return false;
    td_widget_t *w = win->focus;
    if (!w || !w->visible)
        return false;
    return widget_key(w, ev);
}

/* --------------------------------------------------------- inputbox */

#define INPUTBOX_MAX 2

typedef struct
{
    bool used;
    void (*fn)(const char *text, void *user);
    void *user;
    td_widget_t *box;
} inputbox_t;

static inputbox_t s_inputboxes[INPUTBOX_MAX];

static void inputbox_ok(td_widget_t *w, void *user)
{
    inputbox_t *ib = user;
    char text[TD_TEXT_MAX];
    snprintf(text, sizeof(text), "%s", td_widget_text(ib->box));
    void (*fn)(const char *, void *) = ib->fn;
    void *fn_user = ib->user;
    td_win_close(w->win);          /* frees ib in on_close */
    if (fn)
        fn(text, fn_user);
}

static void inputbox_cancel(td_widget_t *w, void *user)
{
    (void)user;
    td_win_close(w->win);
}

static void inputbox_close(td_window_t *win)
{
    inputbox_t *ib = win->user;
    ib->used = false;
}

static td_window_t *inputbox_create(const char *title, const char *prompt, const char *initial,
                                    bool secret, void (*fn)(const char *text, void *user), void *user)
{
    inputbox_t *ib = NULL;
    for (int i = 0; i < INPUTBOX_MAX; i++)
    {
        if (!s_inputboxes[i].used)
        {
            ib = &s_inputboxes[i];
            break;
        }
    }
    if (!ib)
        return NULL;

    td_window_desc_t d = {
        .title = title,
        .rect = td_rect(-1, -1, 44, 8),
        .flags = TD_WIN_MOVABLE | TD_WIN_CLOSABLE | TD_WIN_MODAL,
        .on_close = inputbox_close,
        .user = ib,
    };
    td_window_t *win = td_win_create(&d);
    if (!win)
        return NULL;
    ib->used = true;
    ib->fn = fn;
    ib->user = user;

    td_label(win, 1, 0, -1, prompt ? prompt : "");
    ib->box = td_textbox(win, 1, 2, -1, TD_TEXT_MAX - 1, inputbox_ok, ib);
    td_widget_set_text(ib->box, initial ? initial : "");
    ib->box->secret = secret;
    td_button(win, 12, 4, "OK", inputbox_ok, ib);
    td_button(win, 21, 4, "Cancel", inputbox_cancel, NULL);
    td_widget_focus(ib->box);
    return win;
}

td_window_t *td_inputbox(const char *title, const char *prompt, const char *initial,
                         void (*fn)(const char *text, void *user), void *user)
{
    return inputbox_create(title, prompt, initial, false, fn, user);
}

td_window_t *td_passwordbox(const char *title, const char *prompt,
                            void (*fn)(const char *text, void *user), void *user)
{
    return inputbox_create(title, prompt, "", true, fn, user);
}

/* ----------------------------------------------------------- msgbox */

#define MSGBOX_MAX   4
#define MSGBOX_LINES 8

typedef struct
{
    bool used;
    bool done;
    void (*fn)(int button, void *user);
    void *user;
} msgbox_t;

static msgbox_t s_msgboxes[MSGBOX_MAX];

static void msgbox_button(td_widget_t *w, void *user)
{
    msgbox_t *mb = user;
    void (*fn)(int, void *) = mb->fn;
    void *fn_user = mb->user;
    int index = w->value;
    mb->done = true;
    td_win_close(w->win);          /* frees mb in on_close */
    if (fn)
        fn(index, fn_user);
}

static void msgbox_close(td_window_t *win)
{
    msgbox_t *mb = win->user;
    bool done = mb->done;
    void (*fn)(int, void *) = mb->fn;
    void *fn_user = mb->user;
    mb->used = false;
    if (!done && fn)
        fn(-1, fn_user);   /* closed with Esc or [x] */
}

td_window_t *td_msgbox(const char *title, const char *text, const char *buttons,
                       void (*fn)(int button, void *user), void *user)
{
    msgbox_t *mb = NULL;
    for (int i = 0; i < MSGBOX_MAX; i++)
    {
        if (!s_msgboxes[i].used)
        {
            mb = &s_msgboxes[i];
            break;
        }
    }
    if (!mb)
        return NULL;

    /* Split the text into lines and measure. */
    char lines[MSGBOX_LINES][TD_TEXT_MAX];
    int nlines = 0, width = td_utf8_len(title ? title : "") + 10;
    const char *p = text ? text : "";
    while (nlines < MSGBOX_LINES)
    {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        if (len >= TD_TEXT_MAX)
            len = TD_TEXT_MAX - 1;
        memcpy(lines[nlines], p, len);
        lines[nlines][len] = '\0';
        int cols = td_utf8_len(lines[nlines]);
        if (cols + 4 > width)
            width = cols + 4;
        nlines++;
        if (!nl)
            break;
        p = nl + 1;
    }

    char names[4][TD_TEXT_MAX];
    int nbuttons = 0, buttons_w = 0;
    p = buttons && *buttons ? buttons : "OK";
    while (nbuttons < 4)
    {
        const char *bar = strchr(p, '|');
        size_t len = bar ? (size_t)(bar - p) : strlen(p);
        if (len >= TD_TEXT_MAX)
            len = TD_TEXT_MAX - 1;
        memcpy(names[nbuttons], p, len);
        names[nbuttons][len] = '\0';
        buttons_w += td_utf8_len(names[nbuttons]) + 4 + 2;
        nbuttons++;
        if (!bar)
            break;
        p = bar + 1;
    }
    if (buttons_w + 2 > width)
        width = buttons_w + 2;

    td_window_desc_t d = {
        .title = title,
        .rect = td_rect(-1, -1, width + 2, nlines + 5),
        .flags = TD_WIN_MOVABLE | TD_WIN_CLOSABLE | TD_WIN_MODAL,
        .on_close = msgbox_close,
        .user = mb,
    };
    td_window_t *win = td_win_create(&d);
    if (!win)
        return NULL;
    mb->used = true;
    mb->done = false;
    mb->fn = fn;
    mb->user = user;

    for (int i = 0; i < nlines; i++)
    {
        td_widget_t *l = td_label(win, 1, 1 + i, -1, lines[i]);
        td_widget_set_align(l, TD_ALIGN_CENTER);
    }
    int x = (width - buttons_w + 2) / 2;
    for (int i = 0; i < nbuttons; i++)
    {
        td_widget_t *b = td_button(win, x, nlines + 2, names[i], msgbox_button, mb);
        if (b)
        {
            b->value = i;
            x += b->rect.w + 2;
        }
    }
    return win;
}
