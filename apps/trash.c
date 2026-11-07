/*
 * trash.c - the Recycle Bin: what Files and the Desktop delete goes into
 * the current user's ~/.trash first, to be restored or deleted for good.
 *
 * Each item has a folder of its own, ~/.trash/<id>/, holding the file or
 * folder under its own name and a small text file .tdtrash: where it was,
 * when it was deleted and how big it is. There is no index to go wrong: the
 * bin is what its folders say. Deleting moves (renames) the item, so it is
 * quick and needs no space.
 *
 * Limits, per user (td_trash_prefs(), saved with the user's settings): the
 * bin holds at most a size (the oldest items go first; an item bigger than
 * that is only offered to be deleted for good), and it can delete items
 * after a time (when the clock is set). They are applied after each
 * delete, when a user's settings are applied, and every hour.
 *
 * Restoring puts an item back where it was, making the folders that are
 * gone and choosing "name (2)" when the name is taken. A user other than
 * root only ever restores into their home.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "td_apps.h"

#define INFO_NAME  ".tdtrash"
#define MAX_SCAN   256          /* items the limits look at */
#define MAX_SHOWN  64           /* items the app lists, newest first */
#define WALK_DEPTH 12           /* folder levels a size is added up over */
#define PURGE_MS   (60UL * 60UL * 1000UL)
#define PATH_LEN   (TD_PATH_MAX + 64)

static const uint32_t s_limits[TD_TRASH_SIZES] = {1024u * 1024u, 0, 256u * 1024u, 4u * 1024u * 1024u,
                                                  16u * 1024u * 1024u, 64u * 1024u * 1024u};
static const char *const s_limit_names[TD_TRASH_SIZES] = {"1 MB", "Off", "256 KB", "4 MB", "16 MB", "64 MB"};
static const int s_keep_days[TD_TRASH_KEEPS] = {0, 1, 7, 30, 90};
static const char *const s_keep_names[TD_TRASH_KEEPS] = {"never", "1 day", "7 days", "30 days", "90 days"};

static const td_fs_ops_t *fs(void)
{
    return td_sysinfo()->fs;
}

static bool usable(void)
{
    const td_fs_ops_t *f = fs();
    return f && f->list && f->rename && f->mkdir && f->remove && f->write && f->read && f->exists &&
           td_home_dir()[0];
}

uint32_t td_trash_limit(void)
{
    int code = td_trash_prefs()->size;
    return code >= 0 && code < TD_TRASH_SIZES ? s_limits[code] : s_limits[0];
}

const char *td_trash_size_name(int code)
{
    return code >= 0 && code < TD_TRASH_SIZES ? s_limit_names[code] : s_limit_names[0];
}

const char *td_trash_keep_name(int code)
{
    return code >= 0 && code < TD_TRASH_KEEPS ? s_keep_names[code] : s_keep_names[0];
}

/* The next size limit up (Off, 256 KB, 1 MB, ... 64 MB, then Off). */
int td_trash_next_size(int code)
{
    uint32_t now = code >= 0 && code < TD_TRASH_SIZES ? s_limits[code] : s_limits[0];
    int best = -1;
    for (int i = 0; i < TD_TRASH_SIZES; i++)
        if (s_limits[i] > now && (best < 0 || s_limits[i] < s_limits[best]))
            best = i;
    return best >= 0 ? best : 1;   /* past the largest: Off */
}

static void trash_dir(char *buf, size_t cap)
{
    snprintf(buf, cap, "%s/.trash", td_home_dir());
}

static bool now_utc(int64_t *t)
{
    const td_sysinfo_t *si = td_sysinfo();
    return si->time_now && si->time_now(t) && *t > 1000000000;
}

static const char *base_name(const char *path)
{
    const char *s = strrchr(path, '/');
    return s ? s + 1 : path;
}

static void human_size(uint32_t n, char *buf, size_t cap)
{
    if (n < 1024)
        snprintf(buf, cap, "%u B", (unsigned)n);
    else if (n < 1024u * 1024u)
        snprintf(buf, cap, "%u KB", (unsigned)((n + 1023) / 1024));
    else
        snprintf(buf, cap, "%u.%u MB", (unsigned)(n / (1024u * 1024u)), (unsigned)(n % (1024u * 1024u) * 10 / (1024u * 1024u)));
}

/* ------------------------------------------------------------ sizes */

/* Folders are added up by appending to one path buffer and cutting it
 * back, so a deep folder costs no stack per level for its path. */
static char s_walk[PATH_LEN];
static uint32_t s_walk_total;
static int s_walk_depth;

static void walk_entry(const char *name, bool is_dir, uint32_t size, void *user)
{
    (void)user;
    if (!is_dir)
    {
        s_walk_total += size;
        return;
    }
    size_t len = strlen(s_walk);
    if (s_walk_depth >= WALK_DEPTH || len + strlen(name) + 2 >= sizeof(s_walk))
        return;
    snprintf(s_walk + len, sizeof(s_walk) - len, "/%s", name);
    s_walk_depth++;
    fs()->list(s_walk, walk_entry, NULL);
    s_walk_depth--;
    s_walk[len] = '\0';
}

typedef struct
{
    const char *name;
    uint32_t size;
    bool found;
} find_t;

static void find_entry(const char *name, bool is_dir, uint32_t size, void *user)
{
    find_t *f = user;
    if (!is_dir && strcmp(name, f->name) == 0)
    {
        f->size = size;
        f->found = true;
    }
}

/* The bytes in a file, or in a folder and everything in it. */
static uint32_t path_size(const char *path, bool is_dir)
{
    if (is_dir)
    {
        snprintf(s_walk, sizeof(s_walk), "%s", path);
        s_walk_total = 0;
        s_walk_depth = 0;
        fs()->list(s_walk, walk_entry, NULL);
        return s_walk_total;
    }
    char dir[PATH_LEN];
    snprintf(dir, sizeof(dir), "%s", path);
    char *slash = strrchr(dir, '/');
    if (!slash)
        return 0;
    *slash = '\0';
    find_t f = {slash + 1, 0, false};
    fs()->list(dir[0] ? dir : "/", find_entry, &f);
    return f.size;
}

/* ------------------------------------------------------------ items */

typedef struct
{
    char path[PATH_LEN];   /* where it was */
    int64_t time;          /* when it was deleted (UTC; 0: not known) */
    uint32_t size;
    bool is_dir;
} info_t;

static bool read_info(const char *item_dir, info_t *info)
{
    char path[PATH_LEN], text[PATH_LEN + 96];
    snprintf(path, sizeof(path), "%s/" INFO_NAME, item_dir);
    memset(info, 0, sizeof(*info));
    int n = fs()->read(path, text, (int)sizeof(text) - 1);
    if (n <= 0)
        return false;
    text[n] = '\0';
    for (char *line = strtok(text, "\n"); line; line = strtok(NULL, "\n"))
    {
        if (strncmp(line, "path=", 5) == 0)
            snprintf(info->path, sizeof(info->path), "%s", line + 5);
        else if (strncmp(line, "time=", 5) == 0)
            info->time = strtoll(line + 5, NULL, 10);
        else if (strncmp(line, "size=", 5) == 0)
            info->size = (uint32_t)strtoul(line + 5, NULL, 10);
        else if (strncmp(line, "dir=", 4) == 0)
            info->is_dir = line[4] == '1';
    }
    return info->path[0] != '\0';
}

/* The light record the limits and the app sort by. */
typedef struct
{
    char id[24];
    int64_t time;
    uint32_t size;
} scan_t;

typedef struct
{
    scan_t *items;
    int count, max;
    const char *dir;
} scan_ctx_t;

static void scan_entry(const char *name, bool is_dir, uint32_t size, void *user)
{
    (void)size;
    scan_ctx_t *c = user;
    if (!is_dir || c->count >= c->max || strlen(name) >= sizeof(c->items[0].id))
        return;
    scan_t *s = &c->items[c->count++];
    snprintf(s->id, sizeof(s->id), "%s", name);
    char item[PATH_LEN];
    snprintf(item, sizeof(item), "%s/%s", c->dir, name);
    info_t info;
    if (read_info(item, &info))
    {
        s->time = info.time;
        s->size = info.size;
    }
    else
    {
        s->time = 0;   /* not ours, or broken: the oldest, so it goes first */
        s->size = 0;
    }
}

static int by_time(const void *a, const void *b)
{
    const scan_t *x = a, *y = b;
    if (x->time != y->time)
        return x->time < y->time ? -1 : 1;
    return strcmp(x->id, y->id);
}

/* The bin's items, oldest first; -1 without memory. */
static int scan(scan_t **out, int max)
{
    char dir[PATH_LEN];
    trash_dir(dir, sizeof(dir));
    scan_ctx_t c = {calloc((size_t)max, sizeof(scan_t)), 0, max, dir};
    *out = c.items;
    if (!c.items)
        return -1;
    if (fs()->exists(dir))
        fs()->list(dir, scan_entry, &c);
    qsort(c.items, (size_t)c.count, sizeof(scan_t), by_time);
    return c.count;
}

static void remove_item(const char *id)
{
    char path[PATH_LEN];
    trash_dir(path, sizeof(path));
    size_t len = strlen(path);
    snprintf(path + len, sizeof(path) - len, "/%s", id);
    fs()->remove(path);
}

void td_trash_purge(void)
{
    if (!usable())
        return;
    scan_t *items;
    int n = scan(&items, MAX_SCAN);
    if (n <= 0)
    {
        free(items);
        return;
    }
    uint32_t limit = td_trash_limit();
    int days = s_keep_days[td_trash_prefs()->keep < TD_TRASH_KEEPS ? td_trash_prefs()->keep : 0];
    int64_t now;
    bool clock = now_utc(&now);
    uint64_t total = 0;
    for (int i = 0; i < n; i++)
        total += items[i].size;
    for (int i = 0; i < n; i++)
    {
        bool expired = days > 0 && clock && items[i].time > 0 && now - items[i].time > (int64_t)days * 86400;
        if (expired || total > limit)
        {
            remove_item(items[i].id);
            total -= items[i].size;
        }
    }
    free(items);
}

/* ------------------------------------------------------------ delete */

static bool in_trash(const char *path)
{
    char dir[PATH_LEN];
    trash_dir(dir, sizeof(dir));
    size_t n = strlen(dir);
    return strncmp(path, dir, n) == 0 && (path[n] == '\0' || path[n] == '/');
}

td_trash_result_t td_trash_put(const char *path, bool is_dir)
{
    uint32_t limit = td_trash_limit();
    if (!usable() || limit == 0 || in_trash(path))
        return TD_TRASH_OFF;
    uint32_t size = path_size(path, is_dir);
    if (size > limit)
        return TD_TRASH_TOO_BIG;

    char dir[PATH_LEN], item[PATH_LEN + 32], target[PATH_LEN * 2];
    trash_dir(dir, sizeof(dir));
    if (!fs()->exists(dir) && fs()->mkdir(dir) != 0)
        return TD_TRASH_FAILED;
    int64_t now;
    long long stamp = now_utc(&now) ? (long long)now : (long long)td_millis();
    for (int n = 0;; n++)
    {
        snprintf(item, sizeof(item), "%s/%lld-%d", dir, stamp, n);
        if (!fs()->exists(item))
            break;
        if (n > 999)
            return TD_TRASH_FAILED;
    }
    if (fs()->mkdir(item) != 0)
        return TD_TRASH_FAILED;
    char text[PATH_LEN + 96];
    int len = snprintf(text, sizeof(text), "path=%s\ntime=%lld\nsize=%lu\ndir=%d\n", path,
                       now_utc(&now) ? (long long)now : 0LL, (unsigned long)size, is_dir ? 1 : 0);
    snprintf(target, sizeof(target), "%s/" INFO_NAME, item);
    if (fs()->write(target, text, len) != 0)
    {
        fs()->remove(item);
        return TD_TRASH_FAILED;
    }
    snprintf(target, sizeof(target), "%s/%s", item, base_name(path));
    if (fs()->rename(path, target) != 0)
    {
        fs()->remove(item);
        return TD_TRASH_FAILED;
    }
    td_trash_purge();
    return TD_TRASH_OK;
}

/* The confirmation Files and the Desktop show. */
static char s_ask_path[PATH_LEN];
static bool s_ask_dir, s_ask_bin;
static void (*s_ask_done)(void);

static void ask_answer(int button, void *user)
{
    (void)user;
    if (button == 0)
    {
        if (s_ask_bin)
        {
            td_trash_result_t r = td_trash_put(s_ask_path, s_ask_dir);
            if (r != TD_TRASH_OK)
                td_msgbox("Recycle Bin", "It could not be moved to the Recycle Bin.", "OK", NULL, NULL);
        }
        else if (!fs() || !fs()->remove || fs()->remove(s_ask_path) != 0)
        {
            td_msgbox("Delete", "Delete failed.", "OK", NULL, NULL);
        }
    }
    if (s_ask_done)
        s_ask_done();
}

void td_trash_ask_delete(const char *path, bool is_dir, bool for_good, void (*done)(void))
{
    snprintf(s_ask_path, sizeof(s_ask_path), "%s", path);
    s_ask_dir = is_dir;
    s_ask_done = done;
    char text[200], name[24 * 4 + 1], sz[16], lim[16]; /* the dialog shortens lines to fit */
    td_utf8_copy(name, sizeof(name), base_name(path), 24);   /* never half a character */
    uint32_t limit = td_trash_limit();
    s_ask_bin = !for_good && limit > 0 && usable() && !in_trash(path);
    uint32_t size = s_ask_bin ? path_size(path, is_dir) : 0;
    if (s_ask_bin && size <= limit)
    {
        snprintf(text, sizeof(text), "Move %s%s%s to the Recycle Bin?", is_dir ? "folder " : "", name,
                 is_dir ? " and everything in it" : "");
        td_msgbox("Delete", text, "Move|Cancel", ask_answer, NULL);
        return;
    }
    if (s_ask_bin)
    {
        human_size(size, sz, sizeof(sz));
        human_size(limit, lim, sizeof(lim));
        snprintf(text, sizeof(text), "%s is %s, more than the Recycle Bin\nholds (%s). Delete it for good?", name, sz,
                 lim);
        s_ask_bin = false;
    }
    else
    {
        snprintf(text, sizeof(text), "Delete %s%s%s for good?", is_dir ? "folder " : "", name,
                 is_dir ? " and\neverything in it" : "");
    }
    td_msgbox("Confirm", text, "Delete|Cancel", ask_answer, NULL);
}

/* Several items: their paths in one heap block, one question. */
static char *s_many;          /* count paths, each PATH_LEN bytes */
static bool *s_many_dir;
static int s_many_count;
static bool s_many_bin;
static void (*s_many_done)(void);

static void many_free(void)
{
    free(s_many);
    free(s_many_dir);
    s_many = NULL;
    s_many_dir = NULL;
    s_many_count = 0;
}

static void many_answer(int button, void *user)
{
    (void)user;
    int failed = 0;
    for (int i = 0; button == 0 && i < s_many_count; i++)
    {
        const char *p = s_many + (size_t)i * PATH_LEN;
        bool bin = s_many_bin && path_size(p, s_many_dir[i]) <= td_trash_limit();
        if (bin ? td_trash_put(p, s_many_dir[i]) != TD_TRASH_OK : (!fs() || !fs()->remove || fs()->remove(p) != 0))
            failed++;
    }
    many_free();
    if (failed)
    {
        char text[64];
        snprintf(text, sizeof(text), "%d of the items could not be deleted.", failed);
        td_msgbox("Delete", text, "OK", NULL, NULL);
    }
    if (s_many_done)
        s_many_done();
}

void td_trash_ask_delete_many(const char *const *paths, const bool *is_dir, int count, bool for_good,
                              void (*done)(void))
{
    if (count == 1)
    {
        td_trash_ask_delete(paths[0], is_dir[0], for_good, done);
        return;
    }
    if (count < 1)
        return;
    many_free();
    s_many = malloc((size_t)count * PATH_LEN);
    s_many_dir = malloc((size_t)count * sizeof(bool));
    if (!s_many || !s_many_dir)
    {
        many_free();
        td_msgbox("Delete", "Not enough memory for that many items.", "OK", NULL, NULL);
        return;
    }
    bool bin = !for_good && td_trash_limit() > 0 && usable();
    int dirs = 0;
    for (int i = 0; i < count; i++)
    {
        snprintf(s_many + (size_t)i * PATH_LEN, PATH_LEN, "%s", paths[i]);
        s_many_dir[i] = is_dir[i];
        dirs += is_dir[i];
        if (in_trash(paths[i]))
            bin = false;
    }
    s_many_count = count;
    s_many_bin = bin;
    s_many_done = done;
    char text[160];
    if (bin)
        snprintf(text, sizeof(text), "Move %d items%s to the Recycle Bin?\n(Any too big for it are deleted for good.)",
                 count, dirs ? ", folders with everything in them," : "");
    else
        snprintf(text, sizeof(text), "Delete %d items%s for good?", count, dirs ? ",\nfolders with everything in them," : "");
    td_msgbox(bin ? "Delete" : "Confirm", text, bin ? "Move|Cancel" : "Delete|Cancel", many_answer, NULL);
}

/* ------------------------------------------------------------ restore */

/* Every folder on the way to path (inside the file system's root). */
static void make_parents(const char *path)
{
    char p[PATH_LEN];
    snprintf(p, sizeof(p), "%s", path);
    const char *root = fs()->root ? fs()->root : "";
    size_t start = strlen(root);
    for (char *s = p + start + 1; (s = strchr(s, '/')) != NULL; s++)
    {
        *s = '\0';
        if (!fs()->exists(p))
            fs()->mkdir(p);
        *s = '/';
    }
}

/* path, or "name (2).ext", "name (3).ext"... when it is taken. */
static bool free_name(char *path, size_t cap, bool is_dir)
{
    if (!fs()->exists(path))
        return true;
    char base[PATH_LEN], ext[32] = "";
    snprintf(base, sizeof(base), "%s", path);
    char *dot = strrchr(base, '.');
    if (!is_dir && dot && dot > strrchr(base, '/') + 1 && strlen(dot) < sizeof(ext))
    {
        snprintf(ext, sizeof(ext), "%s", dot);
        *dot = '\0';
    }
    for (int n = 2; n < 100; n++)
    {
        snprintf(path, cap, "%s (%d)%s", base, n, ext);
        if (!fs()->exists(path))
            return true;
    }
    return false;
}

/* NULL, or why the item could not be put back. *where gets where it went. */
const char *td_trash_restore(const char *id, char *where, int where_cap)
{
    if (!usable())
        return "There is no file system.";
    char item[PATH_LEN], from[PATH_LEN * 2], to[PATH_LEN];
    trash_dir(item, sizeof(item));
    size_t len = strlen(item);
    snprintf(item + len, sizeof(item) - len, "/%s", id);
    info_t info;
    if (!read_info(item, &info))
        return "It has no record of where it was.";
    const char *name = base_name(info.path);
    snprintf(from, sizeof(from), "%s/%s", item, name);
    if (!fs()->exists(from))
        return "It is no longer in the Recycle Bin.";

    /* Anyone but root only restores into their home. */
    const char *home = td_home_dir();
    size_t hl = strlen(home);
    bool inside = strncmp(info.path, home, hl) == 0 && info.path[hl] == '/' && !strstr(info.path, "/../");
    if (td_session_is_root() || inside)
        snprintf(to, sizeof(to), "%s", info.path);
    else
        snprintf(to, sizeof(to), "%s/%s", home, name);
    make_parents(to);
    if (!free_name(to, sizeof(to), info.is_dir))
        return "There is no free name for it.";
    if (fs()->rename(from, to) != 0)
        return "It could not be moved back.";
    fs()->remove(item);
    if (where)
        snprintf(where, (size_t)where_cap, "%s", to);
    return NULL;
}

void td_trash_delete(const char *id)
{
    if (usable())
        remove_item(id);
}

void td_trash_empty(void)
{
    char dir[PATH_LEN];
    trash_dir(dir, sizeof(dir));
    if (usable() && fs()->exists(dir))
        fs()->remove(dir);
}

/* ------------------------------------------------------------ the app */

typedef struct
{
    char id[24];
    char line[96];
} row_t;

static td_window_t *s_win;
static td_widget_t *s_list, *s_status, *s_size_btn, *s_keep_btn;
static row_t *s_rows;
static int s_count;

static const char *list_item(td_widget_t *w, int index, int *fg, void *user)
{
    (void)w;
    (void)fg;
    (void)user;
    return index >= 0 && index < s_count ? s_rows[index].line : "";
}

static void show_settings(void)
{
    const td_trash_prefs_t *p = td_trash_prefs();
    td_widget_printf(s_size_btn, "Size: %s", td_trash_size_name(p->size));
    td_widget_printf(s_keep_btn, "Delete after: %s", td_trash_keep_name(p->keep));
}

/* The bin's items, newest first, as list lines. */
static void reload(void)
{
    if (!td_win_is_open(s_win))
        return;
    scan_t *items = NULL;
    int n = usable() ? scan(&items, MAX_SCAN) : 0;
    uint64_t total = 0;
    s_count = 0;
    for (int i = n - 1; i >= 0 && s_count < MAX_SHOWN; i--)
    {
        char item[PATH_LEN];
        trash_dir(item, sizeof(item));
        size_t len = strlen(item);
        snprintf(item + len, sizeof(item) - len, "/%s", items[i].id);
        info_t info;
        bool ok = read_info(item, &info);
        row_t *r = &s_rows[s_count++];
        snprintf(r->id, sizeof(r->id), "%s", items[i].id);
        char name[24 * 4 + 1], from[28 * 4 + 1], when[32] = "", sz[16];
        char folder[PATH_LEN];
        snprintf(folder, sizeof(folder), "%s", ok ? td_shell_path(info.path) : "?");
        char *slash = strrchr(folder, '/');
        if (slash)
            *(slash == folder ? slash + 1 : slash) = '\0';
        td_utf8_copy(name, sizeof(name), ok ? base_name(info.path) : items[i].id, 22);
        td_utf8_copy(from, sizeof(from), folder, 22);
        if (ok && info.time > 0)
            td_date_time(info.time, when, sizeof(when));
        human_size(items[i].size, sz, sizeof(sz));
        /* Long names are cut short in the list: that is fine. */
        if (snprintf(r->line, sizeof(r->line), "%s%-22s %-22s %-16s %7s", ok && info.is_dir ? "\xE2\x96\xB8" : " ",
                     name, from, when, sz) < 0)
            r->line[0] = '\0';
    }
    for (int i = 0; i < n; i++)
        total += items[i].size;
    free(items);
    td_list_set_count(s_list, s_count);
    char tot[16], lim[16];
    human_size((uint32_t)(total > 0xFFFFFFFFu ? 0xFFFFFFFFu : total), tot, sizeof(tot));
    human_size(td_trash_limit(), lim, sizeof(lim));
    if (td_trash_limit() == 0)
        td_widget_printf(s_status, "The Recycle Bin is off: deleting deletes for good.");
    else
        td_widget_printf(s_status, "%d item%s, %s of %s%s", n, n == 1 ? "" : "s", tot, lim,
                         n > MAX_SHOWN ? " (the newest shown)" : "");
    show_settings();
    td_win_invalidate(s_win);
}

static const row_t *selected_row(void)
{
    int i = td_list_selected(s_list);
    return i >= 0 && i < s_count ? &s_rows[i] : NULL;
}

static void on_restore(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    const row_t *r = selected_row();
    if (!r)
        return;
    char where[PATH_LEN], text[PATH_LEN + 32];
    const char *err = td_trash_restore(r->id, where, sizeof(where));
    if (err)
        td_msgbox("Restore", err, "OK", NULL, NULL);
    else
    {
        snprintf(text, sizeof(text), "Restored to\n%s", td_shell_path(where));
        td_widget_set_text(s_status, "");
        td_desktop_refresh();
        reload();
        td_msgbox("Restore", text, "OK", NULL, NULL);
    }
}

static char s_pending[24];

static void delete_answer(int button, void *user)
{
    (void)user;
    if (button == 0)
        td_trash_delete(s_pending);
    reload();
}

static void on_delete(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    const row_t *r = selected_row();
    if (!r)
        return;
    snprintf(s_pending, sizeof(s_pending), "%s", r->id);
    td_msgbox("Recycle Bin", "Delete the selected item for good?", "Delete|Cancel", delete_answer, NULL);
}

static void empty_answer(int button, void *user)
{
    (void)user;
    if (button == 0)
        td_trash_empty();
    reload();
}

static void on_empty(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    if (s_count > 0)
        td_msgbox("Recycle Bin", "Delete everything in the Recycle Bin for good?", "Empty|Cancel", empty_answer, NULL);
}

static void on_size(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    td_trash_prefs_t *p = td_trash_prefs();
    p->size = (uint8_t)td_trash_next_size(p->size);
    td_settings_save();
    td_trash_purge();
    reload();
}

static void on_keep(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    td_trash_prefs_t *p = td_trash_prefs();
    p->keep = (uint8_t)((p->keep + 1) % TD_TRASH_KEEPS);
    td_settings_save();
    td_trash_purge();
    reload();
}

static void on_activate(td_widget_t *w, void *user)
{
    on_restore(w, user);
}

static void on_refresh(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    reload();
}

static void on_close(td_window_t *win)
{
    (void)win;
    free(s_rows);
    s_rows = NULL;
    s_win = NULL;
}

static void on_draw(td_window_t *win, int w, int h)
{
    (void)win;
    (void)w;
    (void)h;
    const td_theme_t *t = td_theme();
    td_text(2, 0, "Name                   From                   Deleted              Size", t->win_fg, t->win_bg,
            TD_BOLD);
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        reload();
        return;
    }
    s_rows = calloc(MAX_SHOWN, sizeof(row_t));
    if (!s_rows)
    {
        td_msgbox("Recycle Bin", "Not enough memory.", "OK", NULL, NULL);
        return;
    }
    td_window_desc_t d = {
        .title = "Recycle Bin",
        .rect = td_rect(-1, -1, 78, 19),
        .min_w = 54,
        .min_h = 10,
        .flags = TD_WIN_MOVABLE | TD_WIN_CLOSABLE | TD_WIN_RESIZABLE,
        .on_draw = on_draw,
        .on_close = on_close,
    };
    s_win = td_win_create(&d);
    if (!s_win)
    {
        free(s_rows);
        s_rows = NULL;
        return;
    }
    s_list = td_list(s_win, td_rect(1, 1, -2, -6), list_item, on_activate, NULL);
    td_scrollbar(s_win, -1, 1, -6, s_list);
    td_button(s_win, 1, -4, "Restore", on_restore, NULL);
    td_button(s_win, 13, -4, "Delete", on_delete, NULL);
    td_button(s_win, 24, -4, "Empty bin", on_empty, NULL);
    td_button(s_win, 38, -4, "Refresh", on_refresh, NULL);
    s_size_btn = td_button(s_win, 1, -2, "Size: 1 MB", on_size, NULL);
    s_keep_btn = td_button(s_win, 18, -2, "Delete after: never", on_keep, NULL);
    s_status = td_label(s_win, 45, -2, 0, "");
    td_widget_set_color(s_status, td_theme()->dim, TD_COLOR_DEFAULT);
    td_widget_focus(s_list);
    reload();
}

static const td_app_t s_app = {"Recycle Bin", launch, "\\/"};

static void purge_tick(void *user)
{
    (void)user;
    td_trash_purge();
    if (td_win_is_open(s_win))
        reload();
}

void td_trash_register(void)
{
    td_app_register(&s_app);
    td_wm_pin_icon(s_app.name);   /* the bottom right of the desktop */
    td_timer_start(PURGE_MS, true, purge_tick, NULL, td_millis());
}
