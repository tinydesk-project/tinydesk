/*
 * search.c - Search: one box that finds apps and files as you type and
 * opens them (Alt+S or Win+S, from anywhere on the desktop).
 *
 * Matches ignore case. Apps come first (the start menu's apps, names
 * starting with the text before names containing it), then the files and
 * folders of the user's home (root: the whole filesystem) whose names
 * contain it, walked breadth-first up to SCAN_MAX entries, so a large
 * card cannot stall the desktop. The walk runs a moment after the last
 * key, not on every key. Enter or a double-click opens the selected
 * result as the Desktop and Files do (td_open_path); Up / Down / PgUp /
 * PgDn move in the results while the cursor stays in the box, Esc closes.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "td_apps.h"

#define RESULTS_MAX 64
#define SCAN_MAX    1500        /* entries one search looks at */
#define DIRS_MAX    256         /* folders waiting to be walked */
#define DEPTH_MAX   8
#define QUERY_MAX   48
#define WAIT_MS     180         /* after the last key */

typedef struct
{
    uint8_t kind;               /* 0 app, 1 folder, 2 file */
    const char *app;            /* kind 0 */
    char *path;                 /* kinds 1, 2: heap */
} result_t;

static td_window_t *s_win;
static td_widget_t *s_box, *s_list, *s_status;
static result_t s_res[RESULTS_MAX];
static int s_count;
static char s_query[QUERY_MAX + 1];
static int s_timer = -1;
static bool s_more;             /* the walk stopped at SCAN_MAX */

/* Breadth-first walk state (heap, only during a search). */
typedef struct
{
    char **dirs;
    int head, tail;
    int scanned;
    const char *dir;
} walk_t;

static bool contains(const char *s, const char *q)
{
    size_t n = strlen(q);
    for (; *s; s++)
    {
        size_t i = 0;
        while (i < n && s[i] && tolower((unsigned char)s[i]) == tolower((unsigned char)q[i]))
            i++;
        if (i == n)
            return true;
    }
    return n == 0;
}

static bool starts(const char *s, const char *q)
{
    for (; *q; s++, q++)
        if (tolower((unsigned char)*s) != tolower((unsigned char)*q))
            return false;
    return true;
}

static void clear(void)
{
    for (int i = 0; i < s_count; i++)
        free(s_res[i].path);
    memset(s_res, 0, sizeof(s_res));
    s_count = 0;
    s_more = false;
}

static void on_entry(const char *name, bool is_dir, uint32_t size, void *user)
{
    (void)size;
    walk_t *w = user;
    if (w->scanned++ >= SCAN_MAX)
    {
        s_more = true;
        return;
    }
    if (name[0] == '.')
        return;   /* hidden: the Recycle Bin, settings */
    size_t dl = strlen(w->dir);
    char *path = malloc(dl + strlen(name) + 2);
    if (!path)
        return;
    sprintf(path, "%s%s%s", w->dir, dl && w->dir[dl - 1] == '/' ? "" : "/", name);
    if (contains(name, s_query) && s_count < RESULTS_MAX)
    {
        s_res[s_count].kind = is_dir ? 1 : 2;
        s_res[s_count].path = path;
        s_count++;
        if (is_dir && w->tail - w->head < DIRS_MAX)
        {
            char *copy = malloc(strlen(path) + 1);
            if (copy)
                w->dirs[w->tail++ % DIRS_MAX] = strcpy(copy, path);
        }
        return;
    }
    if (is_dir && w->tail - w->head < DIRS_MAX)
        w->dirs[w->tail++ % DIRS_MAX] = path;
    else
        free(path);
}

static int depth_of(const char *path, const char *root)
{
    int d = 0;
    for (const char *p = path + strlen(root); *p; p++)
        d += *p == '/';
    return d;
}

static void search_files(void)
{
    const td_fs_ops_t *fs = td_sysinfo()->fs;
    if (!fs || !fs->list || !s_query[0])
        return;
    const char *root = td_session_is_root() ? fs->root : td_session_home();
    if (!root || !root[0])
        return;
    walk_t w = {0};
    w.dirs = calloc(DIRS_MAX, sizeof(char *));
    if (!w.dirs)
        return;
    char *first = malloc(strlen(root) + 1);
    if (first)
        w.dirs[w.tail++] = strcpy(first, root);
    while (w.head < w.tail && w.scanned < SCAN_MAX && s_count < RESULTS_MAX)
    {
        char *dir = w.dirs[w.head++ % DIRS_MAX];
        if (depth_of(dir, root) < DEPTH_MAX)
        {
            w.dir = dir;
            fs->list(dir, on_entry, &w);
        }
        free(dir);
    }
    if (w.head < w.tail || s_count >= RESULTS_MAX)
        s_more = true;
    while (w.head < w.tail)
        free(w.dirs[w.head++ % DIRS_MAX]);
    free(w.dirs);
}

static void search(void)
{
    clear();
    /* Apps: those starting with the text, then those containing it. */
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < td_app_count() && s_count < RESULTS_MAX; i++)
        {
            const td_app_t *a = td_app_get(i);
            if (!a || !a->name)
                continue;
            bool st = starts(a->name, s_query);
            if (pass == 0 ? st : (!st && contains(a->name, s_query)))
            {
                s_res[s_count].kind = 0;
                s_res[s_count].app = a->name;
                s_count++;
            }
        }
    search_files();
    td_list_set_count(s_list, s_count);
    td_list_select(s_list, 0);
    char st[64];
    if (!s_query[0])
        snprintf(st, sizeof(st), "Type to find apps, files and folders.");
    else if (!s_count)
        snprintf(st, sizeof(st), "Nothing found.");
    else
        snprintf(st, sizeof(st), "%d found%s. Enter opens.", s_count, s_more ? " (more: be more exact)" : "");
    td_widget_set_text(s_status, st);
    td_win_invalidate(s_win);
}

static const char *list_item(td_widget_t *w, int i, int *fg, void *user)
{
    (void)w;
    (void)user;
    static char line[TD_PATH_MAX + 32];
    if (i < 0 || i >= s_count)
        return "";
    const result_t *r = &s_res[i];
    if (r->kind == 0)
    {
        snprintf(line, sizeof(line), "\xE2\x96\xBA %-24s app", r->app);
        *fg = td_theme()->accent;
        return line;
    }
    const char *slash = strrchr(r->path, '/');
    const char *name = slash ? slash + 1 : r->path;
    int dir_len = slash ? (int)(slash - r->path) : 0;
    /* The folder it is in, as the user knows it: "~/Desktop" in their home. */
    const char *home = td_session_home();
    size_t hl = home ? strlen(home) : 0;
    const char *dir = r->path, *tilde = "";
    if (hl && strncmp(r->path, home, hl) == 0 && (r->path[hl] == '/' || r->path[hl] == '\0'))
    {
        dir += hl;
        dir_len -= (int)hl;
        tilde = "~";
    }
    snprintf(line, sizeof(line), "%s %-24s %s%.*s", r->kind == 1 ? "[/" : "\xC2\xB6 ", name, tilde, dir_len, dir);
    return line;
}

static void open_selected(void)
{
    int i = td_list_selected(s_list);
    if (i < 0 || i >= s_count)
        return;
    result_t r = s_res[i];
    char *path = NULL;
    if (r.path && (path = malloc(strlen(r.path) + 1)) != NULL)
        strcpy(path, r.path);
    td_win_close(s_win);   /* frees the results */
    if (r.kind == 0)
        td_app_launch(r.app);
    else if (path)
        td_open_path(path, r.kind == 1);
    free(path);
}

static void on_activate(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    open_selected();
}

static void on_wait(void *user)
{
    (void)user;
    s_timer = -1;
    if (td_win_is_open(s_win))
        search();
}

/* The box's text changed: search a moment after the last key. */
static void poll(void *user)
{
    (void)user;
    if (!td_win_is_open(s_win))
        return;
    const char *q = td_widget_text(s_box);
    if (strcmp(q, s_query) == 0)
        return;
    snprintf(s_query, sizeof(s_query), "%s", q);
    if (s_timer >= 0)
        td_timer_stop(s_timer);
    s_timer = td_timer_start(WAIT_MS, false, on_wait, NULL, td_millis());
}

static int s_poll = -1;

static bool on_event(td_window_t *win, const td_event_t *ev)
{
    (void)win;
    if (ev->type != TD_EV_KEY || ev->mods)
        return false;
    int sel = td_list_selected(s_list), page = 8;
    switch (ev->key)
    {
    case TD_KEY_DOWN:
        sel++;
        break;
    case TD_KEY_UP:
        sel--;
        break;
    case TD_KEY_PGDN:
        sel += page;
        break;
    case TD_KEY_PGUP:
        sel -= page;
        break;
    case TD_KEY_ESC:
        td_win_close(s_win);
        return true;
    default:
        return false;
    }
    if (s_count)
        td_list_select(s_list, sel < 0 ? 0 : sel >= s_count ? s_count - 1
                                                            : sel);
    return true;
}

static void on_enter(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    if (strcmp(td_widget_text(s_box), s_query) != 0 || s_timer >= 0)
    {
        /* Typed and pressed Enter at once: search first. */
        if (s_timer >= 0)
            td_timer_stop(s_timer);
        s_timer = -1;
        snprintf(s_query, sizeof(s_query), "%s", td_widget_text(s_box));
        search();
    }
    open_selected();
}

static void on_close(td_window_t *win)
{
    (void)win;
    if (s_timer >= 0)
        td_timer_stop(s_timer);
    if (s_poll >= 0)
        td_timer_stop(s_poll);
    s_timer = s_poll = -1;
    clear();
    s_win = NULL;
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        td_widget_focus(s_box);
        return;
    }
    td_window_desc_t d = {
        .title = "Search",
        .rect = td_rect(-1, -1, 64, 16),
        .min_w = 40,
        .min_h = 8,
        .flags = TD_WIN_DEFAULT,
        .on_event = on_event,
        .on_close = on_close,
    };
    s_win = td_win_create(&d);
    if (!s_win)
        return;
    s_query[0] = '\0';
    s_box = td_textbox(s_win, 1, 0, -2, QUERY_MAX, on_enter, NULL);
    s_list = td_list(s_win, td_rect(1, 2, -2, -2), list_item, on_activate, NULL);
    s_status = td_label(s_win, 1, -1, 0, "");
    td_widget_set_color(s_status, td_theme()->dim, TD_COLOR_DEFAULT);
    td_widget_focus(s_box);
    s_poll = td_timer_start(60, true, poll, NULL, td_millis());
    search();
}

static const td_app_t s_app = {"Search", launch, "?/"};

void td_search_register(void)
{
    td_app_register(&s_app);
}
