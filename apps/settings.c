/*
 * settings.c - theme, ASCII/UTF-8 mode, desktop pattern and icons, and the
 * clock format, per user.
 *
 * Each user's settings are saved in ~/.tinydesk_settings as soon as they
 * change. A user without that file gets the device defaults: the blob the
 * port stores (NVS on ESP32, a file on the host), which is what older
 * versions saved for everyone, or the built-in defaults.
 */
#include <stdio.h>
#include <string.h>

#include "td_apps.h"

#define SETTINGS_MAGIC 0x54445331u   /* "TDS1" */
#define SETTINGS_FILE  "/.tinydesk_settings"

typedef struct
{
    uint32_t magic;
    uint8_t theme;
    uint8_t ascii;
    uint8_t pattern;
    uint8_t hide_icons;   /* 0 in blobs saved before icons existed: shown */
    /* Added later; 0 (the defaults) in shorter, older blobs. */
    uint8_t clock_12h;
    uint8_t date_format;
    uint8_t hide_clock;
    /* Sizes: 0 medium (and in older blobs), 1 small, 2 large. */
    uint8_t icon_size;
    uint8_t menu_size;
    uint8_t bar_size;
    uint8_t editor;       /* bit 0: completion pops up in the Editor */
    uint8_t reserved[1];
} settings_blob_t;

#define LEGACY_SIZE 8   /* magic + theme, ascii, pattern, hide_icons */

static const uint32_t s_patterns[] = {0x2591, 0x2592, 0x2593, 0x00B7, ' '};
static const char *const s_pattern_names[] = {"light shade", "medium shade", "dark shade", "dots", "plain"};
#define PATTERN_COUNT ((int)(sizeof(s_patterns) / sizeof(s_patterns[0])))

static td_clock_prefs_t s_clock;
static td_editor_prefs_t s_editor;
static td_window_t *s_win;
static td_widget_t *s_theme_box[2], *s_ascii, *s_icons, *s_pattern_label, *s_status;
static td_widget_t *s_size_btn[3];      /* desktop icons, start menu, taskbar */

static uint8_t size_code(td_ui_size_t s)
{
    return s == TD_UI_SMALL ? 1 : s == TD_UI_LARGE ? 2
                                                   : 0;
}
static td_ui_size_t code_size(uint8_t c)
{
    return c == 1 ? TD_UI_SMALL : c == 2 ? TD_UI_LARGE
                                         : TD_UI_MEDIUM;
}

td_clock_prefs_t *td_clock_prefs(void)
{
    return &s_clock;
}

td_editor_prefs_t *td_editor_prefs(void)
{
    return &s_editor;
}

static int pattern_index(void)
{
    for (int i = 0; i < PATTERN_COUNT; i++)
        if (s_patterns[i] == td_desktop_pattern())
            return i;
    return 0;
}

static void settings_path(char *out, size_t cap)
{
    snprintf(out, cap, "%.200s" SETTINGS_FILE, td_session_home());
}

/* The current user's blob, else the device defaults; false if neither. */
static bool load_blob(settings_blob_t *b)
{
    const td_sysinfo_t *si = td_sysinfo();
    memset(b, 0, sizeof(*b));
    if (si->fs && si->fs->read && td_session_home()[0])
    {
        char path[240];
        settings_path(path, sizeof(path));
        int n = si->fs->read(path, (char *)b, (int)sizeof(*b));
        if (n >= LEGACY_SIZE && b->magic == SETTINGS_MAGIC)
            return true;
        memset(b, 0, sizeof(*b));
    }
    if (si->settings_load && si->settings_load(b, LEGACY_SIZE) && b->magic == SETTINGS_MAGIC)
    {
        memset((uint8_t *)b + LEGACY_SIZE, 0, sizeof(*b) - LEGACY_SIZE);
        return true;
    }
    return false;
}

void td_settings_apply_saved(void)
{
    settings_blob_t b;
    if (!load_blob(&b))
    {
        b.theme = TD_THEME_DEFAULT;   /* built-in defaults */
        b.ascii = td_get_ascii_mode() ? 1 : 0;
        b.pattern = 0;
        b.hide_icons = 0;
    }
    td_theme_set(b.theme < td_theme_count() ? b.theme : TD_THEME_DEFAULT);
    if ((b.ascii != 0) != td_get_ascii_mode())
    {
        td_set_ascii_mode(b.ascii != 0);
        td_full_redraw();
    }
    if (b.pattern < PATTERN_COUNT)
        td_desktop_set_pattern(s_patterns[b.pattern]);
    td_desktop_set_icons(!b.hide_icons);
    s_clock.clock_12h = b.clock_12h ? 1 : 0;
    s_clock.date_format = b.date_format < TD_DATE_FORMATS ? b.date_format : TD_DATE_DMY;
    s_clock.hide_clock = b.hide_clock ? 1 : 0;
    s_editor.completion = b.editor & 1u;
    td_wm_set_icon_size(code_size(b.icon_size));
    td_wm_set_start_menu_size(code_size(b.menu_size));
    td_wm_set_taskbar_size(code_size(b.bar_size));
    td_wm_invalidate();
}

bool td_settings_save(void)
{
    const td_sysinfo_t *si = td_sysinfo();
    settings_blob_t b = {
        .magic = SETTINGS_MAGIC,
        .theme = (uint8_t)td_theme_index(),
        .ascii = td_get_ascii_mode() ? 1 : 0,
        .pattern = (uint8_t)pattern_index(),
        .hide_icons = td_desktop_icons() ? 0 : 1,
        .clock_12h = s_clock.clock_12h,
        .date_format = s_clock.date_format,
        .hide_clock = s_clock.hide_clock,
        .icon_size = size_code(td_wm_icon_size()),
        .menu_size = size_code(td_wm_start_menu_size()),
        .bar_size = size_code(td_wm_taskbar_size()),
        .editor = s_editor.completion ? 1 : 0,
    };
    if (!si->fs || !si->fs->write || !td_session_home()[0])
        return false;
    char path[240];
    settings_path(path, sizeof(path));
    return si->fs->write(path, (const char *)&b, (int)sizeof(b)) == 0;
}

static void update_labels(void)
{
    int idx = td_theme_index();
    td_checkbox_set(s_theme_box[0], idx == 0);
    td_checkbox_set(s_theme_box[1], idx == 1);
    td_checkbox_set(s_ascii, td_get_ascii_mode());
    td_checkbox_set(s_icons, td_desktop_icons());
    td_widget_set_text(s_size_btn[0], td_ui_size_name(td_wm_icon_size()));
    td_widget_set_text(s_size_btn[1], td_ui_size_name(td_wm_start_menu_size()));
    td_widget_set_text(s_size_btn[2], td_ui_size_name(td_wm_taskbar_size()));
    uint8_t utf8[4];
    int n = td_utf8_encode(td_desktop_pattern(), utf8);
    char ch[5];
    memcpy(ch, utf8, (size_t)n);
    ch[n] = '\0';
    td_widget_printf(s_pattern_label, "Desktop: %s %s%s%s", s_pattern_names[pattern_index()], ch, ch, ch);
}

/* Every change is saved for the current user right away. */
static void changed(void)
{
    update_labels();
    if (td_settings_save())
        td_widget_printf(s_status, "Saved for %.24s.", td_session_user());
    else
        td_widget_set_text(s_status, "Saving is not available here.");
}

static void on_theme(td_widget_t *w, void *user)
{
    (void)w;
    td_theme_set((int)(intptr_t)user);
    changed();
}

static void on_ascii(td_widget_t *w, void *user)
{
    (void)user;
    td_set_ascii_mode(td_checkbox_get(w));
    td_full_redraw();
    changed();
}

static void on_icons(td_widget_t *w, void *user)
{
    (void)user;
    td_desktop_set_icons(td_checkbox_get(w));
    changed();
}

static void on_pattern(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    td_desktop_set_pattern(s_patterns[(pattern_index() + 1) % PATTERN_COUNT]);
    changed();
}

/* Small -> Medium -> Large -> Small. */
static td_ui_size_t next_size(td_ui_size_t s)
{
    return s == TD_UI_SMALL ? TD_UI_MEDIUM : s == TD_UI_MEDIUM ? TD_UI_LARGE
                                                               : TD_UI_SMALL;
}

static void on_size(td_widget_t *w, void *user)
{
    (void)w;
    switch ((int)(intptr_t)user)
    {
    case 0:
        td_wm_set_icon_size(next_size(td_wm_icon_size()));
        break;
    case 1:
        td_wm_set_start_menu_size(next_size(td_wm_start_menu_size()));
        break;
    default:
        td_wm_set_taskbar_size(next_size(td_wm_taskbar_size()));
        break;
    }
    changed();
}

static void on_draw(td_window_t *win, int w, int h)
{
    (void)win;
    (void)w;
    (void)h;
    const td_theme_t *t = td_theme();
    td_text(1, 7, "Sizes", t->win_fg, t->win_bg, TD_BOLD);
    td_text(3, 8, "Desktop icons", t->win_fg, t->win_bg, 0);
    td_text(3, 9, "Start menu", t->win_fg, t->win_bg, 0);
    td_text(3, 10, "Taskbar", t->win_fg, t->win_bg, 0);
}

static void on_network(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    td_app_launch("Network");
}

static void on_update(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    td_app_launch("Software Update");
}

static void on_datetime(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    td_datetime_open();
}

static void on_close_btn(td_widget_t *w, void *user)
{
    (void)user;
    td_win_close(w->win);
}

static void on_close(td_window_t *win)
{
    (void)win;
    s_win = NULL;
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    td_window_desc_t d = {
        .title = "Settings",
        .rect = td_rect(-1, -1, 46, 21),
        .on_draw = on_draw,
        .flags = TD_WIN_MOVABLE | TD_WIN_CLOSABLE,
        .on_close = on_close,
    };
    s_win = td_win_create(&d);
    if (!s_win)
        return;

    td_label(s_win, 1, 1, 0, "Theme");
    s_theme_box[0] = td_checkbox(s_win, 3, 2, td_theme_get(0)->name, false, on_theme, (void *)(intptr_t)0);
    s_theme_box[1] = td_checkbox(s_win, 20, 2, td_theme_get(1)->name, false, on_theme, (void *)(intptr_t)1);
    s_ascii = td_checkbox(s_win, 1, 4, "ASCII-only drawing (no UTF-8)", false, on_ascii, NULL);
    s_icons = td_checkbox(s_win, 1, 5, "Desktop icons", true, on_icons, NULL);
    for (int i = 0; i < 3; i++)
        s_size_btn[i] = td_button(s_win, 18, 8 + i, "Medium", on_size, (void *)(intptr_t)i);
    s_pattern_label = td_label(s_win, 1, 12, 0, "");
    td_button(s_win, 3, 13, "Next pattern", on_pattern, NULL);
    td_button(s_win, 1, 15, "Network...", on_network, NULL);
    td_button(s_win, 16, 15, "Date & time...", on_datetime, NULL);
    td_button(s_win, 1, 16, "Software update...", on_update, NULL);
    td_button(s_win, 1, 18, "Close", on_close_btn, NULL);
    s_status = td_label(s_win, 11, 18, 33, "");
    td_widget_printf(s_status, "Settings of %.24s", td_session_user());
    update_labels();
}

static const td_app_t s_app = {"Settings", launch, "☼ "};

void td_settings_register(void)
{
    td_app_register(&s_app);
}
