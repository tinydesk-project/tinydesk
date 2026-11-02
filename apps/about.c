/*
 * about.c - name, version, chip, SDK version, heap and uptime.
 */
#include <stdio.h>

#include "td_apps.h"

/* Wide enough for the longest line, the shell repository's address (50
 * characters) at the labels' indent of 2. */
#define ABOUT_W 56

static td_window_t *s_win;
static td_widget_t *s_heap, *s_uptime;
static int s_url_y; /* row of the repository addresses */

static void update(void)
{
    const td_sysinfo_t *si = td_sysinfo();
    uint32_t psram = si->psram_total ? si->psram_total() : 0;
    if (si->free_heap && psram)
        td_widget_printf(s_heap, "Free RAM:  %u KB + %u KB PSRAM", (unsigned)(si->free_heap() / 1024u),
                         (unsigned)(si->psram_free() / 1024u));
    else if (si->free_heap)
        td_widget_printf(s_heap, "Free RAM:  %u KB", (unsigned)(si->free_heap() / 1024u));
    else
        td_widget_printf(s_heap, "Free RAM:  n/a");
    uint32_t s = td_uptime_ms() / 1000u;
    td_widget_printf(s_uptime, "Uptime:    %uh %02um %02us",
                     (unsigned)(s / 3600u), (unsigned)(s / 60u % 60u), (unsigned)(s % 60u));
}

/* The addresses are drawn, not labels: a label holds TD_TEXT_MAX - 1
 * characters (47 on the ESP32 builds), and the shell's address is 50. */
static void on_draw(td_window_t *win, int w, int h)
{
    (void)win;
    (void)w;
    (void)h;
    const td_theme_t *th = td_theme();
    td_text(2, s_url_y, TD_REPO_URL, th->win_fg, th->win_bg, 0);
    td_text(2, s_url_y + 1, TD_SHELL_REPO_URL, th->win_fg, th->win_bg, 0);
}

static void on_tick(td_window_t *win)
{
    (void)win;
    update();
}

static void on_ok(td_widget_t *w, void *user)
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
    const td_sysinfo_t *si = td_sysinfo();
    /* The firmware's own version when the port has one (the ESP32's
     * PROJECT_VER, as Software Update shows it), else the library's. */
    td_ota_info_t fw;
    bool have_fw = si->ota && si->ota->info;
    if (have_fw)
        si->ota->info(&fw);
    /* 14 rows with the optional Built and extra lines; drop the ones missing. */
    int h = 14 + (have_fw ? 1 : 0) + (si->extra ? 1 : 0);
    td_window_desc_t d = {
        .title = "About TinyDesk",
        .rect = td_rect(-1, -1, ABOUT_W, h),
        .flags = TD_WIN_MOVABLE | TD_WIN_CLOSABLE,
        .on_draw = on_draw,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = 1000,
    };
    s_win = td_win_create(&d);
    if (!s_win)
        return;

    char title_text[TD_TEXT_MAX];
    snprintf(title_text, sizeof(title_text), "TinyDesk %s", have_fw ? fw.version : TD_VERSION);
    td_widget_t *title = td_label(s_win, 0, 1, 0, title_text);
    td_widget_set_align(title, TD_ALIGN_CENTER);
    td_widget_set_color(title, td_theme()->accent, TD_COLOR_DEFAULT);
    td_widget_t *tag = td_label(s_win, 0, 2, 0, "A windowed desktop for microcontrollers");
    td_widget_set_align(tag, TD_ALIGN_CENTER);

    char line[TD_TEXT_MAX];
    snprintf(line, sizeof(line), "Platform:  %s", si->platform ? si->platform : "n/a");
    td_label(s_win, 2, 4, 0, line);
    snprintf(line, sizeof(line), "Chip:      %s", si->chip ? si->chip : "n/a");
    td_label(s_win, 2, 5, 0, line);
    snprintf(line, sizeof(line), "SDK:       %s", si->sdk_version ? si->sdk_version : "n/a");
    td_label(s_win, 2, 6, 0, line);
    int y = 7;
    if (have_fw)
    {
        snprintf(line, sizeof(line), "Built:     %.20s, %.8s", fw.built, fw.running);
        td_label(s_win, 2, y++, 0, line);
    }
    s_heap = td_label(s_win, 2, y++, 0, "");
    s_uptime = td_label(s_win, 2, y++, 0, "");
    if (si->extra)
        td_label(s_win, 2, y++, 0, si->extra);
    s_url_y = y; /* drawn by on_draw */
    y += 2;

    td_widget_t *ok = td_button(s_win, (ABOUT_W - 2 - 6) / 2, y, "OK", on_ok, NULL); /* "[ OK ]", centred */
    td_widget_focus(ok);
    update();
}

static const td_app_t s_app = {"About", launch, "i "};

void td_about_register(void)
{
    td_app_register(&s_app);
}
