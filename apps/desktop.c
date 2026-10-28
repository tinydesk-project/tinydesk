/*
 * desktop.c - the current user's Desktop folder (~/Desktop) shown as
 * desktop icons, and the right-click menus of the desktop and its icons.
 *
 * The folder is re-read every few seconds, so files created from the shell
 * (touch ~/Desktop/notes.txt) appear without doing anything.
 */
#include <stdio.h>
#include <string.h>

#include "td_apps.h"

#define DESK_MAX   24
#define NAME_LEN   40
#define PATH_LEN   TD_PATH_MAX
#define REFRESH_MS 3000

typedef struct
{
    char name[NAME_LEN];
    bool is_dir;
} item_t;

static item_t s_items[DESK_MAX];
static int s_count;
static char s_desk[PATH_LEN];        /* <user's home>/Desktop */
static bool s_timer_started;
static int s_target = -1;           /* item a menu or dialog is about */
static char s_target_name[NAME_LEN];

static const td_fs_ops_t *fs(void)
{
    return td_sysinfo()->fs;
}

const char *td_home_dir(void)
{
    return td_session_home();
}
const char *td_desktop_dir(void)
{
    return s_desk;
}

bool td_valid_name(const char *name)
{
    if (!name || !name[0] || strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return false;
    return strchr(name, '/') == NULL && strchr(name, '\\') == NULL;
}

/* out must hold PATH_LEN bytes; a path that does not fit becomes "". */
static void item_path(char *out, const char *name)
{
    int n = snprintf(out, PATH_LEN, "%s/%s", s_desk, name);
    if (n < 0 || n >= PATH_LEN)
        out[0] = '\0';
}

/* ------------------------------------------------------- the folder */

static item_t s_scan[DESK_MAX];
static int s_scan_count;

static void add_scan(const char *name, bool is_dir, uint32_t size, void *user)
{
    (void)size;
    (void)user;
    if (s_scan_count >= DESK_MAX || name[0] == '.')
        return;   /* hide dot files */
    item_t *it = &s_scan[s_scan_count++];
    snprintf(it->name, sizeof(it->name), "%s", name);
    it->is_dir = is_dir;
}

static bool item_before(const item_t *a, const item_t *b)
{
    if (a->is_dir != b->is_dir)
        return a->is_dir;
    return strcmp(a->name, b->name) < 0;
}

void td_desktop_refresh(void)
{
    if (!fs() || !fs()->list || !s_desk[0])
        return;
    s_scan_count = 0;
    if (fs()->list(s_desk, add_scan, NULL) < 0)
        s_scan_count = 0;
    for (int i = 1; i < s_scan_count; i++)
    {      /* folders first, by name */
        item_t it = s_scan[i];
        int j = i - 1;
        while (j >= 0 && item_before(&it, &s_scan[j]))
        {
            s_scan[j + 1] = s_scan[j];
            j--;
        }
        s_scan[j + 1] = it;
    }
    if (s_scan_count == s_count && memcmp(s_scan, s_items, sizeof(item_t) * (size_t)s_count) == 0)
        return;
    memcpy(s_items, s_scan, sizeof(item_t) * (size_t)s_scan_count);
    s_count = s_scan_count;
    td_wm_invalidate();
}

static void refresh_timer(void *user)
{
    (void)user;
    td_desktop_refresh();
}

/* ------------------------------------------------------ moving files */

const char *td_shell_path(const char *path)
{
    const char *root = fs() ? fs()->root : "";
    size_t n = strlen(root);
    if (n > 0 && strncmp(path, root, n) == 0 && (path[n] == '/' || path[n] == '\0'))
        return path[n] ? path + n : "/";
    return path;
}

const char *td_move_into(const char *path, const char *dir)
{
    const td_fs_ops_t *f = fs();
    if (!f || !f->rename)
        return "Moving is not supported here.";
    const char *slash = strrchr(path, '/');
    const char *name = slash ? slash + 1 : path;
    size_t parent_len = slash ? (size_t)(slash - path) : 0;
    size_t plen = strlen(path);

    /* Already in that folder: nothing to do. */
    if (strlen(dir) == parent_len && strncmp(dir, path, parent_len) == 0)
        return NULL;
    /* A folder cannot go inside itself. */
    if (strcmp(dir, path) == 0 || (strncmp(dir, path, plen) == 0 && dir[plen] == '/'))
        return "A folder cannot be moved into itself.";

    char to[PATH_LEN + NAME_LEN];
    int n = snprintf(to, sizeof(to), "%s/%s", dir, name);
    if (n < 0 || n >= (int)sizeof(to))
        return "The path is too long.";
    if (f->exists && f->exists(to))
        return "Something with that name is already there.";
    if (f->rename(path, to) != 0)
        return "Move failed.";
    td_desktop_refresh();
    return NULL;
}

/* ---------------------------------------------------- provider hooks */

static int prov_count(void *user)
{
    (void)user;
    return s_count;
}

static const char *prov_label(int i, void *user)
{
    (void)user;
    return s_items[i].name;
}

static const char *prov_icon(int i, void *user)
{
    (void)user;
    if (s_items[i].is_dir)
        return "[/";
    if (td_is_script(s_items[i].name))
        return "#!";   /* shell scripts */
    return td_is_python(s_items[i].name) ? "py" : "\xC2\xB6 ";
}

static int prov_icon_fg(int i, void *user)
{
    (void)user;
    return !s_items[i].is_dir && td_is_script(s_items[i].name) ? td_script_colour(td_theme()->desktop_bg) : -1;
}

static void open_item(int i)
{
    if (i < 0 || i >= s_count)
        return;
    char path[PATH_LEN];
    item_path(path, s_items[i].name);
    if (s_items[i].is_dir)
        td_files_open(path);
    else
        td_editor_open(path);
}

static void prov_open(int i, void *user)
{
    (void)user;
    open_item(i);
}

/* --------------------------------------------------------- actions */

static void error_box(const char *text)
{
    td_msgbox("Desktop", text, "OK", NULL, NULL);
}

/* Show an error and leave the calling function. */
#define FAIL(text)       \
    do                   \
    {                    \
        error_box(text); \
        return;          \
    } while (0)

static void new_file_answer(const char *name, void *user)
{
    (void)user;
    char path[PATH_LEN];
    if (!td_valid_name(name))
        FAIL("That is not a valid name.");
    item_path(path, name);
    if (fs()->exists && fs()->exists(path))
        FAIL("That name is already in use.");
    if (fs()->write(path, "", 0) != 0)
        FAIL("The file could not be created.");
    td_desktop_refresh();
    td_editor_open(path);
}

static void new_folder_answer(const char *name, void *user)
{
    (void)user;
    char path[PATH_LEN];
    if (!td_valid_name(name))
        FAIL("That is not a valid name.");
    item_path(path, name);
    if (fs()->exists && fs()->exists(path))
        FAIL("That name is already in use.");
    if (fs()->mkdir(path) != 0)
        FAIL("The folder could not be created.");
    td_desktop_refresh();
}

static void rename_answer(const char *name, void *user)
{
    (void)user;
    char from[PATH_LEN], to[PATH_LEN];
    if (!td_valid_name(name))
        FAIL("That is not a valid name.");
    if (strcmp(name, s_target_name) == 0)
        return;
    item_path(from, s_target_name);
    item_path(to, name);
    if (fs()->rename(from, to) != 0)
        FAIL("Rename failed (is the name in use?).");
    td_desktop_refresh();
}

static void delete_answer(int button, void *user)
{
    (void)user;
    if (button != 0)
        return;
    char path[PATH_LEN];
    item_path(path, s_target_name);
    if (fs()->remove(path) != 0)
        error_box("Delete failed.");
    td_desktop_refresh();
}

static void ask_delete(void)
{
    char text[160], name[24 * 4 + 1]; /* the dialog shortens lines to fit */
    bool dir = s_items[s_target].is_dir;
    td_utf8_copy(name, sizeof(name), s_target_name, 24); /* never half a character */
    snprintf(text, sizeof(text), "Delete %s%s%s?", dir ? "folder " : "", name,
             dir ? " and\neverything in it" : "");
    td_msgbox("Confirm", text, "Delete|Cancel", delete_answer, NULL);
}

/* Right-click on an icon. A script's menu starts with "Run". */
static void item_menu_chosen(int item, void *user)
{
    (void)user;
    if (s_target < 0 || s_target >= s_count || strcmp(s_items[s_target].name, s_target_name) != 0)
        return;
    if (!s_items[s_target].is_dir && td_is_runnable(s_target_name))
    {
        if (item == 0)
        {
            char path[PATH_LEN];
            item_path(path, s_target_name);
            td_script_run(path);
            return;
        }
        item--;
    }
    switch (item)
    {
    case 0:
        open_item(s_target);
        break;
    case 1:
        td_inputbox("Rename", "New name:", s_target_name, rename_answer, NULL);
        break;
    case 2:
        ask_delete();
        break;
    default:
        break;
    }
}

/* Right-click on the empty desktop. */
static void desktop_menu_chosen(int item, void *user)
{
    (void)user;
    switch (item)
    {
    case 0:
        td_inputbox("New file", "Name of the new file on the Desktop:", "notes.txt", new_file_answer, NULL);
        break;
    case 1:
        td_inputbox("New folder", "Name of the new folder on the Desktop:", "New folder", new_folder_answer, NULL);
        break;
    case 3:
        td_app_launch("Terminal");
        break;
    case 4:
        td_files_open(s_desk);
        break;
    case 6:
        td_desktop_refresh();
        break;
    case 7:
        td_app_launch("Settings");
        break;
    default:
        break;
    }
}

static void prov_context(int i, int x, int y, void *user)
{
    (void)user;
    if (i >= 0 && i < s_count)
    {
        static const char *const items[] = {"Run", "Open", "Rename", "Delete"};
        bool script = !s_items[i].is_dir && td_is_runnable(s_items[i].name);
        s_target = i;
        snprintf(s_target_name, sizeof(s_target_name), "%s", s_items[i].name);
        td_menu_popup(x, y, script ? items : items + 1, script ? 4 : 3, item_menu_chosen, NULL);
    }
    else
    {
        static const char *const items[] = {
            "New file",
            "New folder",
            "-",
            "Terminal",
            "Open Desktop folder",
            "-",
            "Refresh",
            "Settings",
        };
        td_menu_popup(x, y, items, 8, desktop_menu_chosen, NULL);
    }
}

/* Drag a desktop file or folder. */
static bool prov_drag(int i, td_drag_item_t *item, void *user)
{
    (void)user;
    if (i < 0 || i >= s_count)
        return false;
    char path[PATH_LEN];
    item_path(path, s_items[i].name);
    snprintf(item->path, sizeof(item->path), "%s", path);
    snprintf(item->name, sizeof(item->name), "%s", s_items[i].name);
    item->is_dir = s_items[i].is_dir;
    return true;
}

/* Drop on a desktop folder (move inside) or anywhere else (move to the
 * Desktop folder). */
static void prov_drop(int i, const td_drag_item_t *item, void *user)
{
    (void)user;
    char dir[PATH_LEN];
    if (i >= 0 && i < s_count && s_items[i].is_dir)
        item_path(dir, s_items[i].name);
    else
        snprintf(dir, sizeof(dir), "%s", s_desk);
    const char *err = td_move_into(item->path, dir);
    if (err)
        error_box(err);
    td_files_changed();
}

static const td_desktop_provider_t s_provider = {
    .count = prov_count,
    .label = prov_label,
    .icon = prov_icon,
    .icon_fg = prov_icon_fg,
    .open = prov_open,
    .context = prov_context,
    .drag = prov_drag,
    .drop = prov_drop,
};

/* ------------------------------------------------------------ init */

static const char s_welcome[] =
    "Welcome to TinyDesk!\n"
    "\n"
    "Files in this Desktop folder show up as icons on the desktop.\n"
    "\n"
    "  double-click      open (text files open in the Editor)\n"
    "  right-click       Open / Rename / Delete\n"
    "  right-click a .tdsh script (icon #!)   Run it in the Terminal\n"
    "  right-click on the empty desktop   New file, New folder, ...\n"
    "\n"
    "The shell sees this folder as ~/Desktop. Every user has their own.\n"
    "Save with Ctrl+S, close with Ctrl+W.\n";

void td_desktop_folder_init(void)
{
    const td_fs_ops_t *f = fs();
    if (!f || !f->list || !f->mkdir)
        return;
    const char *home = td_session_home();
    snprintf(s_desk, sizeof(s_desk), "%.*s/Desktop", PATH_LEN - 9, home);

    if (!f->exists || !f->exists(s_desk))
    {
        /* Parents may already exist: errors are fine. */
        char parent[PATH_LEN];
        f->mkdir(f->root);
        snprintf(parent, sizeof(parent), "%s/home", f->root);
        if (!td_session_is_root())
            f->mkdir(parent);
        f->mkdir(home);
        if (f->mkdir(s_desk) == 0 && f->write)
        {
            char path[PATH_LEN];
            item_path(path, "Welcome.txt");
            f->write(path, s_welcome, (int)sizeof(s_welcome) - 1);
        }
    }
    s_count = 0;                 /* a different user: forget the old icons */
    td_desktop_set_provider(&s_provider);
    td_desktop_refresh();
    if (!s_timer_started)
    {
        s_timer_started = true;
        td_timer_start(REFRESH_MS, true, refresh_timer, NULL, td_millis());
    }
}
