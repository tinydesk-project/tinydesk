/*
 * taskmgr.c - Task Manager: the open windows (switch to one, end it) and,
 * where the platform can list them, the system's tasks with their state,
 * priority, core, CPU share and unused stack. CPU and memory totals on top.
 *
 * Everything it keeps is allocated while the window is open.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "td_apps.h"

#define TASKS_MAX 40
#define WINS_MAX  TD_MAX_WINDOWS
#define APPS_ROWS 5             /* rows of the window list */

static td_window_t *s_win;
static td_widget_t *s_apps, *s_tasks;

typedef struct
{
    td_task_info_t tasks[TASKS_MAX];
    int task_count;
    bool tasks_failed;          /* the port could not list them (memory) */
    int cpu_tenths;             /* whole system, -1 unknown */
    td_window_t *wins[WINS_MAX];
    uint32_t win_generation[WINS_MAX];
    int win_count;
    char item[128];
    char note[64];
} tm_t;

static tm_t *T;

static const char *state_name(char s)
{
    switch (s)
    {
    case 'R':
        return "running";
    case 'r':
        return "ready";
    case 'B':
        return "blocked";
    case 'S':
        return "suspended";
    case 'D':
        return "deleted";
    default:
        return "?";
    }
}

/* Busiest first; tasks without a CPU figure (the first sample) by name. */
static int task_cmp(const void *a, const void *b)
{
    const td_task_info_t *x = a, *y = b;
    if (x->cpu_tenths != y->cpu_tenths)
        return y->cpu_tenths - x->cpu_tenths;
    return strcmp(x->name, y->name);
}

static void refresh(void)
{
    const td_sysinfo_t *si = td_sysinfo();
    int selected = td_list_selected(s_apps), scroll = s_apps ? s_apps->scroll : 0;
    td_window_t *chosen = selected >= 0 && selected < T->win_count ? T->wins[selected] : NULL;
    uint32_t generation = chosen ? T->win_generation[selected] : 0;
    T->win_count = td_wm_windows(T->wins, WINS_MAX);
    td_list_set_count(s_apps, T->win_count);
    selected = -1;
    for (int i = 0; i < T->win_count; i++)
    {
        T->win_generation[i] = T->wins[i]->generation;
        if (T->wins[i] == chosen && T->win_generation[i] == generation)
            selected = i;
    }
    td_list_set_view(s_apps, selected, scroll);

    T->task_count = si->tasks ? si->tasks(T->tasks, TASKS_MAX) : 0;
    T->tasks_failed = T->task_count < 0;
    if (T->task_count < 0)
        T->task_count = 0;
    T->cpu_tenths = -1;
    if (T->task_count > 0)
    {
        int idle = 0;
        bool known = true;
        for (int i = 0; i < T->task_count; i++)
        {
            if (T->tasks[i].cpu_tenths < 0)
                known = false;
            else if (strncmp(T->tasks[i].name, "IDLE", 4) == 0)
                idle += T->tasks[i].cpu_tenths;
        }
        if (known)
            T->cpu_tenths = idle >= 1000 ? 0 : 1000 - idle;
        qsort(T->tasks, (size_t)T->task_count, sizeof(T->tasks[0]), task_cmp);
    }
    td_list_set_count(s_tasks, T->task_count);
    td_wm_invalidate();
}

static const char *app_item(td_widget_t *w, int i, int *fg, void *user)
{
    (void)w;
    (void)user;
    if (!T || i < 0 || i >= T->win_count || !td_win_is_open(T->wins[i]) ||
        T->wins[i]->generation != T->win_generation[i])
        return "";
    td_window_t *win = T->wins[i];
    bool hidden = (win->flags & TD_WIN_HIDDEN) != 0;
    if (hidden)
        *fg = td_theme()->dim;
    char title[TD_TITLE_MAX + 40]; /* 40 cells, padded by code points */
    td_utf8_pad(title, sizeof(title), win->title, 40);
    snprintf(T->item, sizeof(T->item), " %s %s %s", win->icon ? win->icon : "  ", title, /* glyphs: 2 columns */
             win == td_win_focused() ? "active" : hidden ? "minimised"
                                                         : "open");
    return T->item;
}

static const char *task_item(td_widget_t *w, int i, int *fg, void *user)
{
    (void)w;
    (void)fg;
    (void)user;
    if (!T || i >= T->task_count)
        return "";
    const td_task_info_t *t = &T->tasks[i];
    char cpu[16] = "   -", core[12] = "any", stack[16] = "-";
    if (t->cpu_tenths >= 0)
        snprintf(cpu, sizeof(cpu), "%3d.%d", t->cpu_tenths / 10, t->cpu_tenths % 10);
    if (t->core >= 0)
        snprintf(core, sizeof(core), "%d", t->core);
    if (t->stack_free != UINT32_MAX)
        snprintf(stack, sizeof(stack), "%u", (unsigned)t->stack_free);
    snprintf(T->item, sizeof(T->item), " %-16.16s %-9s %4u  %-4s %6s %7s", t->name, state_name(t->state),
             (unsigned)t->priority, core, cpu, stack);
    return T->item;
}

static td_window_t *selected_window(void)
{
    int i = td_list_selected(s_apps);
    if (!T || i < 0 || i >= T->win_count || !td_win_is_open(T->wins[i]) ||
        T->wins[i]->generation != T->win_generation[i])
        return NULL;
    return T->wins[i];
}

static void on_switch(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    td_window_t *win = selected_window();
    if (!win)
        return;
    td_win_focus(win);          /* also brings a minimised window back */
}

static void on_end(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    td_window_t *win = selected_window();
    if (!win)
        return;
    snprintf(T->note, sizeof(T->note), "Ending \"%.40s\"", win->title);
    td_win_request_close(win);  /* it may ask to save first */
    if (td_win_is_open(s_win))
        refresh();
}

static void on_draw(td_window_t *win, int w, int h)
{
    (void)win;
    (void)h;
    const td_theme_t *t = td_theme();
    const td_sysinfo_t *si = td_sysinfo();
    char line[96];

    /* Totals. */
    int x = td_text(0, 0, "CPU ", t->win_fg, t->win_bg, TD_BOLD);
    if (T->cpu_tenths >= 0)
    {
        snprintf(line, sizeof(line), "%3d.%d %%", T->cpu_tenths / 10, T->cpu_tenths % 10);
    }
    else
    {
        snprintf(line, sizeof(line), "%s", si->tasks ? "  ..." : "  n/a");
    }
    x += td_text(x, 0, line, t->win_fg, t->win_bg, 0);
    if (si->cpu_mhz)
    {
        snprintf(line, sizeof(line), " of %d MHz", si->cpu_mhz());
        x += td_text(x, 0, line, t->dim, t->win_bg, 0);
    }
    x += 3;
    x += td_text(x, 0, "RAM ", t->win_fg, t->win_bg, TD_BOLD);
    uint32_t psram = si->psram_total ? si->psram_total() : 0;
    if (si->free_heap && psram)
        snprintf(line, sizeof(line), "%u KB free + %u KB PSRAM", (unsigned)(si->free_heap() / 1024u),
                 (unsigned)(si->psram_free() / 1024u));
    else if (si->free_heap)
        snprintf(line, sizeof(line), "%u KB free of %u", (unsigned)(si->free_heap() / 1024u),
                 (unsigned)(si->total_heap ? si->total_heap() / 1024u : 0));
    else
        snprintf(line, sizeof(line), "n/a");
    td_textn(x, 0, line, w - x, t->win_fg, t->win_bg, 0);
    for (int i = 0; i < T->task_count; i++)
        if (T->tasks[i].stack_free == UINT32_MAX)
        {
            td_textn(0, 1, "Stack watermarks not sampled during live LCD scanout.", w, t->dim, t->win_bg, 0);
            break;
        }

    td_text(0, 2, "Windows", t->win_fg, t->win_bg, TD_BOLD);
    snprintf(line, sizeof(line), "(%d)", T->win_count);
    td_text(8, 2, line, t->dim, t->win_bg, 0);

    int ty = 4 + APPS_ROWS;
    td_text(0, ty, "System tasks", t->win_fg, t->win_bg, TD_BOLD);
    if (si->tasks && T->tasks_failed)
    {
        td_textn(0, ty + 1, " Not enough memory to list the tasks: close a window.", w, t->accent, t->win_bg, 0);
    }
    else if (si->tasks)
    {
        snprintf(line, sizeof(line), "(%d, busiest first)", T->task_count);
        td_text(13, ty, line, t->dim, t->win_bg, 0);
        td_textn(0, ty + 1, " Name             State     Prio  Core  CPU %  Stack free", w, t->dim, t->win_bg, 0);
    }
    else
    {
        td_textn(0, ty + 1, " The task list is not available on this platform.", w, t->dim, t->win_bg, 0);
    }
    if (T->note[0])
        td_textn(28, 3 + APPS_ROWS, T->note, w - 28, t->accent, t->win_bg, 0);
}

static void on_tick(td_window_t *win)
{
    (void)win;
    refresh();
}

static void on_close(td_window_t *win)
{
    (void)win;
    s_win = NULL;
    free(T);
    T = NULL;
}

static bool on_event(td_window_t *win, const td_event_t *ev)
{
    (void)win;
    if (ev->type == TD_EV_KEY && ev->key == TD_KEY_DELETE && ev->mods == 0 && win->focus == s_apps)
    {
        on_end(NULL, NULL);
        return true;
    }
    return false;
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    T = calloc(1, sizeof(*T));
    if (!T)
    {
        td_msgbox("Task Manager", "Not enough memory. Close a window, then try again.", "OK", NULL, NULL);
        return;
    }
    int ty = 4 + APPS_ROWS;
    td_window_desc_t d = {
        .title = "Task Manager",
        .rect = td_rect(-1, -1, 72, 23),
        .flags = TD_WIN_DEFAULT,
        .min_w = 64,
        .min_h = ty + 6,
        .on_draw = on_draw,
        .on_event = on_event,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = 1000,
    };
    s_win = td_win_create(&d);
    if (!s_win)
    {
        free(T);
        T = NULL;
        return;
    }
    s_apps = td_list(s_win, td_rect(0, 3, -1, APPS_ROWS), app_item, on_switch, NULL);
    td_scrollbar(s_win, -1, 3, APPS_ROWS, s_apps);
    td_button(s_win, 0, 3 + APPS_ROWS, "Switch to", on_switch, NULL);
    td_button(s_win, 13, 3 + APPS_ROWS, "End task", on_end, NULL);
    s_tasks = td_list(s_win, td_rect(0, ty + 2, -1, 0), task_item, NULL, NULL);
    td_scrollbar(s_win, -1, ty + 2, 0, s_tasks);
    refresh();
}

static const td_app_t s_app = {"Task Manager", launch, "\xE2\x96\x90\xE2\x96\x8C"};   /* U+2590 U+258C: in every console font */

void td_taskmgr_register(void)
{
    td_app_register(&s_app);
}
