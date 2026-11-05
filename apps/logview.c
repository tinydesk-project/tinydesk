/*
 * logview.c - a ring buffer of log lines and a window that shows them with
 * level colours. Ports feed it from their logging system (on ESP32 through
 * esp_log_set_vprintf).
 */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "td_apps.h"

#ifndef TD_LOG_LINES
#define TD_LOG_LINES 64
#endif
#ifndef TD_LOG_LINE_MAX
#define TD_LOG_LINE_MAX 100
#endif

typedef struct
{
    char level;
    char text[TD_LOG_LINE_MAX];
} log_line_t;

static log_line_t s_lines[TD_LOG_LINES];
static uint32_t s_total;                    /* lines ever added */
static char s_partial[TD_LOG_LINE_MAX];     /* line without its '\n' yet */
static int s_partial_len;
static char s_partial_level;

static void (*s_lock)(void *);
static void (*s_unlock)(void *);
static void *s_lock_ctx;

static void lock(void)
{
    if (s_lock)
        s_lock(s_lock_ctx);
}
static void unlock(void)
{
    if (s_unlock)
        s_unlock(s_lock_ctx);
}

void td_log_set_lock(void (*lock_fn)(void *), void (*unlock_fn)(void *), void *ctx)
{
    s_lock = lock_fn;
    s_unlock = unlock_fn;
    s_lock_ctx = ctx;
}

/* "I (1234) tag: text" -> 'I'. */
static char detect_level(const char *s)
{
    if ((s[0] == 'E' || s[0] == 'W' || s[0] == 'I' || s[0] == 'D' || s[0] == 'V') && s[1] == ' ' && s[2] == '(')
        return s[0];
    return 'I';
}

static void finish_line(void)
{
    s_partial[s_partial_len] = '\0';
    log_line_t *l = &s_lines[s_total % TD_LOG_LINES];
    l->level = s_partial_level ? s_partial_level : detect_level(s_partial);
    memcpy(l->text, s_partial, (size_t)s_partial_len + 1);
    s_total++;
    s_partial_len = 0;
    s_partial_level = 0;
}

void td_log_append(char level, const char *text)
{
    lock();
    for (const char *p = text; *p; p++)
    {
        char c = *p;
        if (c == '\x1b')
        {                    /* drop colour escapes */
            while (*p && *p != 'm')
                p++;
            if (!*p)
                break;
            continue;
        }
        if (c == '\r')
            continue;
        if (c == '\n')
        {
            finish_line();
            continue;
        }
        if (s_partial_len == 0)
            s_partial_level = level;
        if (s_partial_len < TD_LOG_LINE_MAX - 1)
            s_partial[s_partial_len++] = (c == '\t') ? ' ' : c;
    }
    unlock();
}

void td_logf(char level, const char *fmt, ...)
{
    char buf[TD_LOG_LINE_MAX + 2];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    size_t n = strlen(buf);
    if (n == 0 || buf[n - 1] != '\n')
        strcat(buf, "\n");
    td_log_append(level, buf);
}

/* --------------------------------------------------------------- window */

static td_window_t *s_win;
static td_widget_t *s_list, *s_follow, *s_count;
static uint32_t s_clear_base;               /* lines hidden by "Clear" */
static char s_item[TD_LOG_LINE_MAX];
static uint32_t s_seen_total, s_seen_first;
static bool s_was_following;

static uint32_t first_visible(uint32_t total)
{
    uint32_t oldest = total > TD_LOG_LINES ? total - TD_LOG_LINES : 0;
    return s_clear_base > oldest ? s_clear_base : oldest;
}

static const char *get_item(td_widget_t *w, int index, int *fg, void *user)
{
    (void)w;
    (void)user;
    lock();
    uint32_t n = first_visible(s_total) + (uint32_t)index;
    const log_line_t *l = &s_lines[n % TD_LOG_LINES];
    memcpy(s_item, l->text, sizeof(s_item));
    char level = l->level;
    unlock();
    switch (level)
    {
    case 'E':
        *fg = 9;
        break;     /* bright red */
    case 'W':
        *fg = 11;
        break;    /* yellow */
    case 'D':
    case 'V':
        *fg = 8;
        break;
    default:
        *fg = 10;
        break;     /* green */
    }
    return s_item;
}

static void refresh(void)
{
    if (!s_list)
        return;             /* the window could not get its widgets */
    lock();
    uint32_t total = s_total;
    unlock();
    uint32_t first = first_visible(total), lost = first - s_seen_first;
    int count = (int)(total - first);
    bool follow = td_checkbox_get(s_follow);
    if (count != s_list->count || total != s_seen_total || follow != s_was_following)
    {
        int selected = td_list_selected(s_list), scroll = s_list->scroll;
        td_list_set_count(s_list, count);
        if (follow)
            td_list_select(s_list, count - 1);
        else if (lost)
            td_list_set_view(s_list, selected < 0 ? -1 : lost > (uint32_t)selected ? 0
                                                                                   : selected - (int)lost,
                             lost > (uint32_t)scroll ? 0 : scroll - (int)lost);
    }
    s_seen_total = total;
    s_seen_first = first;
    s_was_following = follow;
    td_widget_printf(s_count, "%u lines", (unsigned)total);
}

static void on_tick(td_window_t *win)
{
    (void)win;
    refresh();
}

static void on_clear(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    lock();
    s_clear_base = s_total;
    unlock();
    td_list_set_count(s_list, 0);
}

static void on_close(td_window_t *win)
{
    (void)win;
    s_win = NULL;
    s_list = s_follow = s_count = NULL;
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    if (!td_session_is_root())
    {
        td_msgbox("Log Viewer", "Only root can read the system log.", "OK", NULL, NULL);
        return;
    }
    td_window_desc_t d = {
        .title = "Log Viewer",
        .rect = td_rect(8, 3, 64, 16),
        .flags = TD_WIN_DEFAULT,
        .min_w = 30,
        .min_h = 6,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = 250,
    };
    s_win = td_win_create(&d);
    if (!s_win)
        return;

    s_list = td_list(s_win, td_rect(0, 0, -1, -1), get_item, NULL, NULL);
    td_scrollbar(s_win, -1, 0, -1, s_list);   /* right edge, above the buttons */
    td_button(s_win, 0, -1, "Clear", on_clear, NULL);
    s_follow = td_checkbox(s_win, 10, -1, "Follow", true, NULL, NULL);
    s_count = td_label(s_win, 22, -1, 0, "");
    s_seen_first = first_visible(s_total);
    s_seen_total = s_total;
    s_was_following = false;
    refresh();
}

static const td_app_t s_app = {"Log Viewer", launch, "≡≡"};

void td_logview_register(void)
{
    td_app_register(&s_app);
}
