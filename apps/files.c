/*
 * files.c - browse the port's filesystem (LittleFS on the ESP32-C6):
 * open files in the Editor, create files and folders, rename and delete.
 *
 *   Enter / double-click   open (folders are entered)
 *   Backspace              parent folder
 *   Delete                 delete (asks first)
 *   F2                     rename
 *   right-click            menu for the entry under the mouse
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "td_apps.h"

#define FILES_MAX 64
#define NAME_MAX_ 48
#define PATH_MAX_ TD_PATH_MAX

typedef struct
{
    char name[NAME_MAX_];
    bool is_dir;
    uint32_t size;
} entry_t;

/* The entry table is only allocated while the window is open, so the app
 * costs almost no RAM when it is not used. */
static entry_t *s_entries;
static int s_count;

static td_window_t *s_win;
static td_widget_t *s_list, *s_path_label;
static char s_cwd[PATH_MAX_];
static char s_item[NAME_MAX_ + 56];
static char s_target[NAME_MAX_];      /* entry a dialog is about */
static bool s_target_dir;

static const td_fs_ops_t *fs(void)
{
    return td_sysinfo()->fs;
}

/* dir + "/" + name; an empty string if that does not fit (never a cut-off
 * path, which could name another file). */
static void join(char *out, const char *dir, const char *name)
{
    size_t n = strlen(dir);
    int w = snprintf(out, PATH_MAX_, "%s%s%s", dir, (n > 0 && dir[n - 1] == '/') ? "" : "/", name);
    if (w < 0 || w >= PATH_MAX_)
        out[0] = '\0';
}

static void error_box(const char *text)
{
    td_msgbox("Files", text, "OK", NULL, NULL);
}

/* Show an error and leave the calling function. */
#define FAIL(text)       \
    do                   \
    {                    \
        error_box(text); \
        return;          \
    } while (0)

/* ---------------------------------------------------------- listing */

static void add_entry(const char *name, bool is_dir, uint32_t size, void *user)
{
    (void)user;
    if (s_count >= FILES_MAX)
        return;
    entry_t *e = &s_entries[s_count++];
    snprintf(e->name, sizeof(e->name), "%s", name);
    e->is_dir = is_dir;
    e->size = size;
}

/* Folders first, then by name (insertion sort: lists are short). */
static bool before(const entry_t *a, const entry_t *b)
{
    if (strcmp(a->name, "..") == 0)
        return true;
    if (strcmp(b->name, "..") == 0)
        return false;
    if (a->is_dir != b->is_dir)
        return a->is_dir;
    return strcmp(a->name, b->name) < 0;
}

static void sort_entries(void)
{
    for (int i = 1; i < s_count; i++)
    {
        entry_t e = s_entries[i];
        int j = i - 1;
        while (j >= 0 && before(&e, &s_entries[j]))
        {
            s_entries[j + 1] = s_entries[j];
            j--;
        }
        s_entries[j + 1] = e;
    }
}

/* Root may browse everything; other users stay inside their home. */
static const char *jail(void)
{
    return td_session_jail()[0] ? td_session_jail() : fs()->root;
}

static bool at_root(void)
{
    return strcmp(s_cwd, jail()) == 0;
}

/* True if path is the jail or inside it. */
static bool in_jail(const char *path)
{
    size_t n = strlen(jail());
    return strncmp(path, jail(), n) == 0 && (path[n] == '\0' || path[n] == '/');
}

void td_files_reset(void)
{
    s_cwd[0] = '\0';
    if (td_win_is_open(s_win))
        td_win_close(s_win);
}

/* Re-read the folder; keep the selection on `select` if given. */
static void reload_select(const char *select)
{
    if (!td_win_is_open(s_win))
        return;
    s_count = 0;
    if (!at_root())
        add_entry("..", true, 0, NULL);
    int n = fs()->list(s_cwd, add_entry, NULL);
    sort_entries();
    /* Show the path the way the shell does: relative to the root. */
    const char *shown = s_cwd + strlen(fs()->root);
    td_widget_printf(s_path_label, n < 0 ? "%s (cannot open)" : "%s", *shown ? shown : "/");
    td_list_set_count(s_list, s_count);
    int sel = 0;
    for (int i = 0; select && i < s_count; i++)
        if (strcmp(s_entries[i].name, select) == 0)
            sel = i;
    td_list_select(s_list, sel);
    td_desktop_refresh();   /* the folder may be the Desktop */
}

static void reload(void)
{
    reload_select(NULL);
}

static const char *get_item(td_widget_t *w, int index, int *fg, void *user)
{
    (void)w;
    (void)user;
    const entry_t *e = &s_entries[index];
    char name[NAME_MAX_ + 32]; /* the name column: 32 cells, padded by code points */
    if (e->is_dir)
    {
        *fg = td_theme()->accent == 1 ? 11 : td_theme()->accent;
        snprintf(s_item, sizeof(s_item), "%s/", e->name);
    }
    else if (td_is_script(e->name))
    {
        *fg = td_script_colour(td_theme()->input_bg);   /* shell scripts stand out */
        td_utf8_pad(name, sizeof(name), e->name, 32);
        snprintf(s_item, sizeof(s_item), "%s %8u  #!", name, (unsigned)e->size);
    }
    else
    {
        td_utf8_pad(name, sizeof(name), e->name, 32);
        snprintf(s_item, sizeof(s_item), "%s %8u", name, (unsigned)e->size);
    }
    return s_item;
}

static const entry_t *selected(void)
{
    int i = td_list_selected(s_list);
    return i >= 0 && i < s_count ? &s_entries[i] : NULL;
}

/* ---------------------------------------------------------- actions */

static void go_up(void)
{
    if (at_root())
        return;
    char old[NAME_MAX_];
    char *slash = strrchr(s_cwd, '/');
    snprintf(old, sizeof(old), "%s", slash ? slash + 1 : "");
    if (slash && slash != s_cwd)
        *slash = '\0';
    if (!in_jail(s_cwd))
        snprintf(s_cwd, sizeof(s_cwd), "%s", jail());
    reload_select(old);
}

static void open_selected(void)
{
    const entry_t *e = selected();
    if (!e)
        return;
    if (strcmp(e->name, "..") == 0)
    {
        go_up();
        return;
    }
    char path[PATH_MAX_];
    join(path, s_cwd, e->name);
    if (e->is_dir)
    {
        snprintf(s_cwd, sizeof(s_cwd), "%s", path);
        reload();
    }
    else
    {
        td_editor_open(path);
    }
}

static void new_file_answer(const char *name, void *user)
{
    (void)user;
    char path[PATH_MAX_];
    if (!td_valid_name(name))
        FAIL("That is not a valid name.");
    join(path, s_cwd, name);
    if (fs()->exists && fs()->exists(path))
        FAIL("That name is already in use.");
    if (fs()->write(path, "", 0) != 0)
        FAIL("The file could not be created.");
    reload_select(name);
    td_editor_open(path);
}

static void new_folder_answer(const char *name, void *user)
{
    (void)user;
    char path[PATH_MAX_];
    if (!td_valid_name(name))
        FAIL("That is not a valid name.");
    join(path, s_cwd, name);
    if (fs()->exists && fs()->exists(path))
        FAIL("That name is already in use.");
    if (fs()->mkdir(path) != 0)
        FAIL("The folder could not be created.");
    reload_select(name);
}

static void rename_answer(const char *name, void *user)
{
    (void)user;
    char from[PATH_MAX_], to[PATH_MAX_];
    if (!td_valid_name(name))
        FAIL("That is not a valid name.");
    if (strcmp(name, s_target) == 0)
        return;
    join(from, s_cwd, s_target);
    join(to, s_cwd, name);
    if (fs()->rename(from, to) != 0)
        FAIL("Rename failed (is the name in use?).");
    reload_select(name);
}

static void delete_answer(int button, void *user)
{
    (void)user;
    if (button != 0)
        return;
    char path[PATH_MAX_];
    join(path, s_cwd, s_target);
    if (fs()->remove(path) != 0)
        error_box("Delete failed.");
    reload();
}

/* Remember the selected entry for a dialog; false for none or "..". */
static bool target_selected(void)
{
    const entry_t *e = selected();
    if (!e || strcmp(e->name, "..") == 0)
        return false;
    snprintf(s_target, sizeof(s_target), "%s", e->name);
    s_target_dir = e->is_dir;
    return true;
}

static void do_new_file(void)
{
    td_inputbox("New file", "Name of the new file:", "new.txt", new_file_answer, NULL);
}
static void do_new_folder(void)
{
    td_inputbox("New folder", "Name of the new folder:", "New folder", new_folder_answer, NULL);
}

static void do_rename(void)
{
    if (target_selected())
        td_inputbox("Rename", "New name:", s_target, rename_answer, NULL);
}

static void do_delete(void)
{
    if (!target_selected() || !fs()->remove)
        return;
    char text[160], name[24 * 4 + 1]; /* the dialog shortens lines to fit */
    td_utf8_copy(name, sizeof(name), s_target, 24); /* never half a character */
    snprintf(text, sizeof(text), "Delete %s%s%s?", s_target_dir ? "folder " : "", name,
             s_target_dir ? " and\neverything in it" : "");
    td_msgbox("Confirm", text, "Delete|Cancel", delete_answer, NULL);
}

/* Buttons. */
static void on_activate(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    open_selected();
}
static void on_new(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    do_new_file();
}
static void on_folder(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    do_new_folder();
}
static void on_rename(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    do_rename();
}
static void on_delete(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    do_delete();
}

static void run_selected(void)
{
    const entry_t *e = selected();
    if (!e || e->is_dir || !td_is_runnable(e->name))
        return;
    char path[PATH_MAX_];
    join(path, s_cwd, e->name);
    td_script_run(path);
}

/* Right-click menus. A script's menu starts with "Run". */
static void entry_menu_chosen(int item, void *user)
{
    (void)user;
    const entry_t *e = selected();
    if (e && !e->is_dir && td_is_runnable(e->name))
    {
        if (item == 0)
        {
            run_selected();
            return;
        }
        item--;
    }
    switch (item)
    {
    case 0:
        open_selected();
        break;
    case 1:
        do_rename();
        break;
    case 2:
        do_delete();
        break;
    case 4:
        do_new_file();
        break;
    case 5:
        do_new_folder();
        break;
    case 6:
        reload();
        break;
    default:
        break;
    }
}

static void empty_menu_chosen(int item, void *user)
{
    (void)user;
    switch (item)
    {
    case 0:
        do_new_file();
        break;
    case 1:
        do_new_folder();
        break;
    case 2:
        reload();
        break;
    default:
        break;
    }
}

static void context_menu(td_window_t *win, const td_event_t *ev)
{
    if (!s_list)
        return;
    td_rect_t c = td_win_client(win);
    td_rect_t lr = td_widget_rect(s_list);
    int ax = c.x + ev->x, ay = c.y + ev->y;       /* screen position */
    int row = s_list->scroll + (ay - lr.y);
    bool on_entry = td_rect_contains(lr, ax, ay) && row >= 0 && row < s_count;
    if (on_entry)
        td_list_select(s_list, row);

    if (on_entry && strcmp(s_entries[row].name, "..") != 0)
    {
        static const char *const items[] = {
            "Run",
            "Open",
            "Rename (F2)",
            "Delete (Del)",
            "-",
            "New file",
            "New folder",
            "Refresh",
        };
        bool script = !s_entries[row].is_dir && td_is_runnable(s_entries[row].name);
        td_menu_popup(ax, ay, script ? items : items + 1, script ? 8 : 7, entry_menu_chosen, NULL);
    }
    else
    {
        static const char *const items[] = {"New file", "New folder", "Refresh"};
        td_menu_popup(ax, ay, items, 3, empty_menu_chosen, NULL);
    }
}

static bool on_event(td_window_t *win, const td_event_t *ev)
{
    if (ev->type == TD_EV_MOUSE && ev->action == TD_MOUSE_PRESS && ev->button == TD_BUTTON_RIGHT)
    {
        context_menu(win, ev);
        return true;
    }
    if (ev->type != TD_EV_KEY || ev->mods)
        return false;
    switch (ev->key)
    {
    case TD_KEY_DELETE:
        do_delete();
        return true;
    case TD_KEY_F2:
        do_rename();
        return true;
    case TD_KEY_BACKSPACE:
        go_up();
        return true;
    case TD_KEY_F5:
        reload();
        return true;
    default:
        return false;
    }
}

/* Entry under a client-area position, or -1. */
static int row_at(td_window_t *win, int x, int y)
{
    if (!s_list)
        return -1;
    td_rect_t c = td_win_client(win);
    td_rect_t lr = td_widget_rect(s_list);
    int ax = c.x + x, ay = c.y + y;
    int row = s_list->scroll + (ay - lr.y);
    return td_rect_contains(lr, ax, ay) && row >= 0 && row < s_count ? row : -1;
}

/* Drag an entry out of the list. */
static bool on_drag_start(td_window_t *win, int x, int y, td_drag_item_t *item)
{
    int row = row_at(win, x, y);
    if (row < 0 || strcmp(s_entries[row].name, "..") == 0)
        return false;
    char path[PATH_MAX_];
    join(path, s_cwd, s_entries[row].name);
    snprintf(item->path, sizeof(item->path), "%s", path);
    snprintf(item->name, sizeof(item->name), "%s", s_entries[row].name);
    item->is_dir = s_entries[row].is_dir;
    return true;
}

/* Dropped on a folder row: move into it ("..": the parent). Anywhere else
 * in the window: move into the folder being shown. */
static bool on_drop(td_window_t *win, int x, int y, const td_drag_item_t *item)
{
    char dir[PATH_MAX_];
    int row = row_at(win, x, y);
    snprintf(dir, sizeof(dir), "%s", s_cwd);
    if (row >= 0 && s_entries[row].is_dir)
    {
        if (strcmp(s_entries[row].name, "..") == 0)
        {
            char *slash = strrchr(dir, '/');
            if (slash && slash != dir)
                *slash = '\0';
        }
        else
        {
            join(dir, s_cwd, s_entries[row].name);
        }
    }
    const char *err = td_move_into(item->path, dir);
    if (err)
        error_box(err);
    reload_select(item->name);
    return true;
}

void td_files_changed(void)
{
    if (td_win_is_open(s_win))
        reload();
}

static void on_close(td_window_t *win)
{
    (void)win;
    s_win = NULL;
    free(s_entries);
    s_entries = NULL;
}

static void open_window(void)
{
    if (!fs() || !fs()->list)
    {
        td_msgbox("Files", "No filesystem on this platform.", "OK", NULL, NULL);
        return;
    }
    if (!s_entries)
        s_entries = malloc(sizeof(entry_t) * FILES_MAX);
    if (!s_entries)
    {
        td_msgbox("Files", "Not enough memory.", "OK", NULL, NULL);
        return;
    }
    td_window_desc_t d = {
        .title = "Files",
        .rect = td_rect(6, 2, 58, 18),
        .flags = TD_WIN_DEFAULT,
        .min_w = 50,
        .min_h = 8,
        .on_event = on_event,
        .on_close = on_close,
        .on_drag_start = on_drag_start,
        .on_drop = on_drop,
    };
    s_win = td_win_create(&d);
    if (!s_win)
    {
        free(s_entries);
        s_entries = NULL;
        return;
    }
    s_path_label = td_label(s_win, 0, 0, 0, "");
    s_list = td_list(s_win, td_rect(0, 1, -1, -1), get_item, on_activate, NULL);
    td_scrollbar(s_win, -1, 1, -1, s_list);
    td_button(s_win, 0, -1, "Open", on_activate, NULL);
    td_button(s_win, 9, -1, "New", on_new, NULL);
    td_button(s_win, 17, -1, "Folder", on_folder, NULL);
    td_button(s_win, 28, -1, "Rename", on_rename, NULL);
    td_button(s_win, 39, -1, "Delete", on_delete, NULL);
    td_widget_focus(s_list);
}

void td_files_open(const char *dir)
{
    if (td_win_is_open(s_win))
        td_win_focus(s_win);
    else
        open_window();
    if (!td_win_is_open(s_win))
        return;
    if (dir && in_jail(dir))
        snprintf(s_cwd, sizeof(s_cwd), "%s", dir);
    else if (s_cwd[0] == '\0' || !in_jail(s_cwd))
        snprintf(s_cwd, sizeof(s_cwd), "%s", td_home_dir()[0] ? td_home_dir() : jail());
    reload();
}

static void launch(void)
{
    td_files_open(NULL);
}

static const td_app_t s_app = {"Files", launch, "[]"};

void td_files_register(void)
{
    td_app_register(&s_app);
}
