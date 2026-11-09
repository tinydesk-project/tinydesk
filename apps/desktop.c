/*
 * desktop.c - the current user's Desktop folder (~/Desktop) shown as
 * desktop icons, and the right-click menus of the desktop and its icons.
 *
 * The folder is re-read every few seconds, so files created from the shell
 * (touch ~/Desktop/notes.txt) appear without doing anything.
 */
#include <stdio.h>
#include <stdlib.h>
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
/* Drives besides the board's own (td_sysinfo()->drives): their icons come
 * first, so a provider index p is drive p, or s_items[p - s_nd]. */
#define DRIVES_MAX 4
static td_drive_t s_drives[DRIVES_MAX];
static int s_nd;
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

/* The drives the user may open now; a drive that went away closes what
 * Files had open on it. */
static void refresh_drives(void)
{
    td_drive_t now[DRIVES_MAX];
    int n = td_sysinfo()->drives ? td_sysinfo()->drives(now, DRIVES_MAX) : 0;
    int kept = 0;
    for (int i = 0; i < n && i < DRIVES_MAX; i++)
        if (td_session_path_allowed(now[i].path, TD_FS_LIST))
            now[kept++] = now[i];
    bool same = kept == s_nd;
    for (int i = 0; same && i < kept; i++)
        same = !strcmp(now[i].path, s_drives[i].path) && !strcmp(now[i].label, s_drives[i].label);
    for (int i = 0; i < s_nd; i++)
    {
        bool still = false;
        for (int j = 0; j < kept; j++)
            still |= !strcmp(s_drives[i].path, now[j].path);
        if (!still)
            td_files_drive_gone(s_drives[i].path);
    }
    memcpy(s_drives, now, sizeof(td_drive_t) * (size_t)kept);
    s_nd = kept;
    if (!same)
        td_wm_invalidate();
}

void td_desktop_refresh(void)
{
    refresh_drives();
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

static void no_entry(const char *name, bool is_dir, uint32_t size, void *user)
{
    (void)name;
    (void)is_dir;
    (void)size;
    (void)user;
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
    {
        /* Another drive (the SD card): rename cannot cross it; a file is
         * copied, then removed here. */
        if (!f->list || f->list(path, no_entry, NULL) >= 0)
            return "Folders cannot move to another drive yet: move their files.";
        const char *err = td_copy_file(path, to);
        if (err)
            return err;
        if (!f->remove || f->remove(path) != 0)
            return "Copied, but the original could not be removed.";
    }
    td_desktop_refresh();
    return NULL;
}

/* ---------------------------------------------------- provider hooks */

static int prov_count(void *user)
{
    (void)user;
    return s_nd + s_count;
}

static const char *prov_label(int i, void *user)
{
    (void)user;
    if (i < s_nd)
        return s_drives[i].label;
    i -= s_nd;
    if (!td_is_shortcut(s_items[i].name))
        return s_items[i].name;
    static char label[NAME_LEN];   /* used at once by the caller */
    snprintf(label, sizeof(label), "%.*s", (int)(strlen(s_items[i].name) - 7), s_items[i].name);
    return label;
}

static const char *prov_icon(int i, void *user)
{
    (void)user;
    if (i < s_nd)
        return "[=";   /* a drive */
    i -= s_nd;
    if (s_items[i].is_dir)
        return "[/";
    if (td_is_shortcut(s_items[i].name))
        return "->";
    if (td_is_image(s_items[i].name))
        return "ââ"; /* the Image Viewer's icon */
    if (td_is_script(s_items[i].name))
        return "#!";   /* shell scripts */
    return td_is_python(s_items[i].name) || td_is_compiled_app(s_items[i].name) ? "py" : "\xC2\xB6 ";
}

static int prov_icon_fg(int i, void *user)
{
    (void)user;
    if (i < s_nd)
        return -1;
    i -= s_nd;
    return !s_items[i].is_dir && td_is_script(s_items[i].name) ? td_script_colour(td_theme()->desktop_bg) : -1;
}

static void open_item(int i)
{
    if (i < 0 || i >= s_count)
        return;
    char path[PATH_LEN];
    item_path(path, s_items[i].name);
    td_open_path(path, s_items[i].is_dir);
}

static void prov_open(int i, void *user)
{
    (void)user;
    if (i < s_nd)
        td_files_open(s_drives[i].path);
    else
        open_item(i - s_nd);
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

/* The selected icons (td_desktop_selection), into the bin or for good:
 * one question for them all. */
static void delete_selection(bool for_good)
{
    int sel[64];
    int n = td_desktop_selection(sel, 64);
    char *paths = malloc((size_t)(n > 0 ? n : 1) * PATH_LEN);
    const char *list[64];
    bool dirs[64];
    int k = 0;
    for (int j = 0; paths && j < n; j++)
    {
        int f = sel[j] - s_nd;   /* drives are not deleted */
        if (f < 0 || f >= s_count)
            continue;
        item_path(paths + (size_t)k * PATH_LEN, s_items[f].name);
        list[k] = paths + (size_t)k * PATH_LEN;
        dirs[k] = s_items[f].is_dir;
        k++;
    }
    if (k)
        td_trash_ask_delete_many(list, dirs, k, for_good, td_desktop_refresh);   /* copies the paths */
    free(paths);
}

static void prov_remove(bool for_good, void *user)
{
    (void)user;
    delete_selection(for_good);
}

/* Into the Recycle Bin, or with for_good straight away (asks first). */
static void ask_delete(bool for_good)
{
    int sel[2];
    if (td_desktop_selection(sel, 2) > 1)
    {
        delete_selection(for_good);   /* the icon is one of several selected */
        return;
    }
    char path[PATH_LEN];
    item_path(path, s_target_name);
    td_trash_ask_delete(path, s_items[s_target].is_dir, for_good, td_desktop_refresh);
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
        ask_delete(false);
        break;
    case 3:
        ask_delete(true);
        break;
    default:
    {
        char path[PATH_LEN];
        item_path(path, s_target_name);
        td_shortcut_menu_chosen(path, true, item - 4);   /* Start menu, Apps */
        break;
    }
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
    case 9:
        td_wm_set_icon_arrange(!td_wm_icon_arrange());
        (void)td_settings_save();
        break;
    default:
        break;
    }
}

/* Right-click on a drive: open it, or let it go (eject). */
static char s_drive_path[48];

static void drive_menu_chosen(int item, void *user)
{
    (void)user;
    if (item == 0)
    {
        td_files_open(s_drive_path);
        return;
    }
    const char *why = "It cannot be ejected.";
    if (td_sysinfo()->drive_eject && td_sysinfo()->drive_eject(s_drive_path, &why))
    {
        td_files_drive_gone(s_drive_path);
        td_desktop_refresh();
        td_msgbox("Eject", "It can be taken out now.", "OK", NULL, NULL);
    }
    else
        error_box(why);
}

static void prov_context(int i, int x, int y, void *user)
{
    (void)user;
    if (i >= 0 && i < s_nd)
    {
        static const char *const items[] = {"Open", "Eject"};
        snprintf(s_drive_path, sizeof(s_drive_path), "%s", s_drives[i].path);
        td_menu_popup(x, y, items, td_sysinfo()->drive_eject && s_drives[i].removable ? 2 : 1, drive_menu_chosen,
                      NULL);
        return;
    }
    i -= s_nd;
    if (i >= 0 && i < s_count)
    {
        const char *items[9] = {"Run", "Open", "Rename", "Delete", "Delete for good"};
        int sel[64];
        int many = td_desktop_selection(sel, 64);
        static char del[32], del_good[40];
        if (many > 1)
        {
            snprintf(del, sizeof(del), "Delete %d items", many);
            snprintf(del_good, sizeof(del_good), "Delete %d items for good", many);
            items[3] = del;
            items[4] = del_good;
        }
        bool script = !s_items[i].is_dir && td_is_runnable(s_items[i].name);
        s_target = i;
        snprintf(s_target_name, sizeof(s_target_name), "%s", s_items[i].name);
        char path[PATH_LEN];
        item_path(path, s_target_name);
        int extra = td_shortcut_menu(path, true, items + 5, 4);
        td_menu_popup(x, y, script ? items : items + 1, (script ? 5 : 4) + extra, item_menu_chosen, NULL);
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
            "-",
            "[x] Auto arrange icons",
        };
        static const char *items_off[sizeof(items) / sizeof(items[0])];
        const char *const *shown = items;
        if (!td_wm_icon_arrange())
        {
            /* Unticked: icons stay where they are dragged. */
            memcpy(items_off, items, sizeof(items));
            items_off[9] = "[ ] Auto arrange icons";
            shown = items_off;
        }
        td_menu_popup(x, y, shown, 10, desktop_menu_chosen, NULL);
    }
}

/* Drag a desktop file or folder. */
static bool prov_drag(int i, td_drag_item_t *item, void *user)
{
    (void)user;
    i -= s_nd;   /* drives stay where they are */
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
    if (i >= 0 && i < s_nd)
        snprintf(dir, sizeof(dir), "%s", s_drives[i].path);   /* onto a drive */
    else if (i >= s_nd && i - s_nd < s_count && s_items[i - s_nd].is_dir)
        item_path(dir, s_items[i - s_nd].name);
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
    .remove = prov_remove,
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

/* The port's own files for a new Desktop (td_sysinfo_t.desktop_files). */
static void write_port_files(void)
{
    const td_desktop_file_t *d = td_sysinfo()->desktop_files;
    for (; d && d->name; d++)
    {
        if (!td_valid_name(d->name) || !d->text)
            continue;
        char path[PATH_LEN];
        item_path(path, d->name);
        if (path[0])
            fs()->write(path, d->text, (int)strlen(d->text));
    }
}

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
            write_port_files();
        }
    }
    s_count = 0;                 /* a different user: forget the old icons */
    td_shortcuts_session();      /* ~/Apps, ~/Start menu */
    td_desktop_set_provider(&s_provider);
    td_desktop_refresh();
    if (!s_timer_started)
    {
        s_timer_started = true;
        td_timer_start(REFRESH_MS, true, refresh_timer, NULL, td_millis());
    }
}
