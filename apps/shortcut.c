/*
 * shortcut.c - shortcuts, the Start menu folder and the Apps folder.
 *
 * A shortcut is a small text file NAME.tdlink whose line "target=" names a
 * file or folder by its shell path (/root/Apps/hmi.tdapp). On the Desktop
 * it shows as an icon, in ~/Start menu as an entry of the Start menu;
 * opening it opens its target. ~/Apps is where a user's app packages
 * (.tdapp) live: "Copy to Apps" puts one there and adds it to the Start
 * menu. Both folders are made at sign-in, beside ~/Desktop.
 */
#include "td_apps.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINK_EXT  ".tdlink"
#define START_DIR "Start menu"
#define APPS_DIR  "Apps"
#define PATH_LEN_ TD_PATH_MAX

static const td_fs_ops_t *fs(void)
{
    return td_sysinfo()->fs;
}

static bool ends_with(const char *s, const char *end)
{
    size_t n = strlen(s), m = strlen(end);
    return n >= m && strcmp(s + n - m, end) == 0;
}

bool td_is_shortcut(const char *name)
{
    return name && ends_with(name, LINK_EXT);
}

static void home_dir(char *out, size_t cap, const char *sub)
{
    snprintf(out, cap, "%.*s/%s", (int)(cap - strlen(sub) - 2), td_session_home(), sub);
}

const char *td_apps_dir(void)
{
    static char path[PATH_LEN_];
    home_dir(path, sizeof(path), APPS_DIR);
    return path;
}

const char *td_start_dir(void)
{
    static char path[PATH_LEN_];
    home_dir(path, sizeof(path), START_DIR);
    return path;
}

/* A shell path as a real one (the inverse of td_shell_path). */
static void real_path(char *out, size_t cap, const char *shell)
{
    const char *root = fs() ? fs()->root : "";
    if (shell[0] == '/' && root[0] && strcmp(root, "/") != 0)
        snprintf(out, cap, "%s%s", root, shell);
    else
        snprintf(out, cap, "%s", shell);
}

static void count_entry(const char *name, bool is_dir, uint32_t size, void *user)
{
    (void)name;
    (void)is_dir;
    (void)size;
    ++*(int *)user;
}

static bool is_dir_path(const char *path)
{
    int n = 0;
    return fs() && fs()->list && fs()->list(path, count_entry, &n) >= 0;
}

bool td_shortcut_target(const char *link, char *target, int cap)
{
    char buf[PATH_LEN_ + 16];
    int n = fs() && fs()->read ? fs()->read(link, buf, (int)sizeof(buf) - 1) : -1;
    if (n <= 0)
        return false;
    buf[n] = 0;
    char *t = strstr(buf, "target=");
    if (!t)
        return false;
    t += 7;
    t[strcspn(t, "\r\n")] = 0;
    if (!t[0])
        return false;
    real_path(target, (size_t)cap, t);
    return true;
}

void td_open_path(const char *path, bool is_dir)
{
    const char *slash = strrchr(path, '/');
    const char *name = slash ? slash + 1 : path;
    if (td_is_shortcut(name))
    {
        char target[PATH_LEN_];
        if (!td_shortcut_target(path, target, sizeof(target)) || !fs()->exists || !fs()->exists(target))
        {
            td_msgbox("Shortcut", "What this shortcut opens is no longer there.", "OK", NULL, NULL);
            return;
        }
        const char *ts = strrchr(target, '/');
        if (!td_is_shortcut(ts ? ts + 1 : target))   /* one step only: no loops */
            td_open_path(target, is_dir_path(target));
        return;
    }
    if (is_dir)
        td_files_open(path);
    else if (td_is_image(name))
        td_imageview_open(path);
    else if (td_is_compiled_app(name))
        td_script_run(path);   /* binary apps cannot be edited as text */
    else
        td_editor_open(path);
}

/* The label of target: its file name, without .tdapp. */
static void label_of(char *out, size_t cap, const char *target)
{
    const char *slash = strrchr(target, '/');
    snprintf(out, cap, "%s", slash ? slash + 1 : target);
    if (ends_with(out, ".tdapp"))
        out[strlen(out) - 6] = 0;
}

const char *td_shortcut_make(const char *dir, const char *target)
{
    const td_fs_ops_t *f = fs();
    if (!f || !f->write || !f->exists)
        return "Shortcuts are not supported here.";
    char label[64], link[PATH_LEN_ + 80], text[PATH_LEN_ + 16];
    label_of(label, sizeof(label), target);
    f->mkdir(dir);   /* may already exist */
    for (int i = 1; i < 100; i++)
    {
        if (i == 1)
            snprintf(link, sizeof(link), "%s/%s" LINK_EXT, dir, label);
        else
            snprintf(link, sizeof(link), "%s/%s (%d)" LINK_EXT, dir, label, i);
        if (!f->exists(link))
            break;
    }
    if (f->exists(link))
        return "There are too many shortcuts of that name.";
    int n = snprintf(text, sizeof(text), "target=%s\n", td_shell_path(target));
    if (n <= 0 || n >= (int)sizeof(text) || f->write(link, text, n) != 0)
        return "The shortcut could not be saved.";
    td_desktop_refresh();
    return NULL;
}

/* A shortcut to target in dir already? */
static bool linked_in(const char *dir, const char *target)
{
    char label[64], link[PATH_LEN_ + 80], to[PATH_LEN_];
    label_of(label, sizeof(label), target);
    snprintf(link, sizeof(link), "%s/%s" LINK_EXT, dir, label);
    return fs()->exists && fs()->exists(link) && td_shortcut_target(link, to, sizeof(to)) && !strcmp(to, target);
}

bool td_in_start_menu(const char *target)
{
    return linked_in(td_start_dir(), target);
}

const char *td_start_menu_remove(const char *target)
{
    char label[64], link[PATH_LEN_ + 80];
    label_of(label, sizeof(label), target);
    snprintf(link, sizeof(link), "%s/%s" LINK_EXT, td_start_dir(), label);
    if (!linked_in(td_start_dir(), target) || !fs()->remove || fs()->remove(link) != 0)
        return "It is not in the Start menu.";
    return NULL;
}

const char *td_copy_to_apps(const char *path, char *copy, int cap)
{
    const td_fs_ops_t *f = fs();
    if (!f || !f->read_at || !f->append || !f->write || !f->exists)
        return "Copying is not supported here.";
    const char *slash = strrchr(path, '/');
    const char *name = slash ? slash + 1 : path;
    snprintf(copy, (size_t)cap, "%s/%s", td_apps_dir(), name);
    if (!strcmp(copy, path))
        return NULL;   /* already there */
    if (f->exists(copy))
        return "Apps already has a file of that name.";
    f->mkdir(td_apps_dir());
    enum
    {
        CHUNK = 2048
    };
    char *buf = malloc(CHUNK);
    if (!buf)
        return "Not enough memory.";
    bool ok = f->write(copy, "", 0) == 0;
    for (uint32_t at = 0; ok;)
    {
        int n = f->read_at(path, at, buf, CHUNK);
        if (n < 0)
            ok = false;
        if (n <= 0)
            break;
        ok = f->append(copy, buf, n) == 0;
        at += (uint32_t)n;
    }
    free(buf);
    if (!ok)
    {
        f->remove(copy);
        return "The copy failed (is the file system full?).";
    }
    td_desktop_refresh();
    return NULL;
}

/* -------------------------------------------- the Start menu entries */

#define START_MAX TD_START_USER_MAX
static char s_start[START_MAX][40];   /* names in ~/Start menu, without .tdlink */
static int s_start_count;

static void add_start(const char *name, bool is_dir, uint32_t size, void *user)
{
    (void)size;
    (void)user;
    size_t n = strlen(name) - strlen(LINK_EXT);
    if (is_dir || !td_is_shortcut(name) || s_start_count >= START_MAX || n >= sizeof(s_start[0]))
        return;
    snprintf(s_start[s_start_count++], sizeof(s_start[0]), "%.*s", (int)n, name);
}

static int start_count(void *user)
{
    (void)user;
    s_start_count = 0;   /* read afresh each time the menu opens */
    if (fs() && fs()->list)
        (void)fs()->list(td_start_dir(), add_start, NULL);
    return s_start_count;
}

static const char *start_label(int i, void *user)
{
    (void)user;
    return s_start[i];
}

static const char *start_icon(int i, void *user)
{
    (void)i;
    (void)user;
    return "py";
}

static void start_open(int i, void *user)
{
    (void)user;
    char path[PATH_LEN_ + 64];
    snprintf(path, sizeof(path), "%s/%s" LINK_EXT, td_start_dir(), s_start[i]);
    td_open_path(path, false);
}

static const td_start_provider_t s_start_provider = {start_count, start_label, start_icon, start_open, NULL};

void td_shortcuts_session(void)
{
    const td_fs_ops_t *f = fs();
    if (f && f->mkdir && td_session_home()[0])
    {
        f->mkdir(td_apps_dir());
        f->mkdir(td_start_dir());
    }
    td_wm_set_start_provider(&s_start_provider);
}

/* ------------------------------------------- right-click menu entries */

enum
{
    A_SEND,
    A_START,
    A_UNSTART,
    A_COPY,
};

static int actions(const char *path, bool on_desktop, int *out)
{
    int n = 0;
    if (!on_desktop)
        out[n++] = A_SEND;
    if (ends_with(path, ".tdapp"))
    {
        out[n++] = td_in_start_menu(path) ? A_UNSTART : A_START;
        char apps[PATH_LEN_ + 2];
        snprintf(apps, sizeof(apps), "%s/", td_apps_dir());
        if (strncmp(path, apps, strlen(apps)) != 0)
            out[n++] = A_COPY;
    }
    return n;
}

int td_shortcut_menu(const char *path, bool on_desktop, const char **items, int max)
{
    static const char *const labels[] = {"Send to Desktop", "Add to Start menu", "Remove from Start menu",
                                         "Copy to Apps"};
    int a[4], n = actions(path, on_desktop, a);
    for (int i = 0; i < n && i < max; i++)
        items[i] = labels[a[i]];
    return n < max ? n : max;
}

static void report(const char *error, const char *done)
{
    td_msgbox(error ? "Not done" : "Done", error ? error : done, "OK", NULL, NULL);
}

void td_shortcut_menu_chosen(const char *path, bool on_desktop, int index)
{
    int a[4], n = actions(path, on_desktop, a);
    if (index < 0 || index >= n)
        return;
    char desk[PATH_LEN_], copy[PATH_LEN_ + 64];
    switch (a[index])
    {
    case A_SEND:
        home_dir(desk, sizeof(desk), "Desktop");
        report(td_shortcut_make(desk, path), "A shortcut is on the Desktop.");
        break;
    case A_START:
        report(td_shortcut_make(td_start_dir(), path), "It is in the Start menu now.");
        break;
    case A_UNSTART:
        report(td_start_menu_remove(path), "It is no longer in the Start menu.");
        break;
    case A_COPY:
    {
        const char *error = td_copy_to_apps(path, copy, sizeof(copy));
        if (!error && !td_in_start_menu(copy))
            error = td_shortcut_make(td_start_dir(), copy);
        report(error, "Copied to Apps and added to the Start menu.");
        break;
    }
    default:
        break;
    }
}
