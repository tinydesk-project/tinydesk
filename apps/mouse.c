/*
 * mouse.c - Mouse: the pointer settings of a mouse the board reads itself
 * (a USB mouse on the ESP32-S3; td_sysinfo()->mouse). Only there does the
 * app exist. Each setting has a bar to click or drag and - / + buttons;
 * changes apply at once and the port saves them (root only). A row of
 * small targets is there to try the pointer out.
 */
#include <stdint.h>

#include "td_apps.h"

enum
{
    SPEED,
    ACCEL,
    PRECISION,
    SETTINGS
};

static const struct
{
    const char *name;
    int min, max, step, def;
} s_rows[SETTINGS] = {
    {"Speed", 25, 300, 5, 100},
    {"Acceleration", 0, 300, 10, 100},
    {"Precision", 0, 100, 10, 50},
};

#define ROW_Y(i) (3 + 2 * (i))
#define BAR_X    23
#define BAR_W    20
#define TARGET_Y 10

static td_window_t *s_win;
static td_widget_t *s_status, *s_bar[SETTINGS], *s_value[SETTINGS], *s_note, *s_hits;
static td_mouse_settings_t s_set;
static int s_hit_count;

static int *value(int i)
{
    return i == SPEED ? &s_set.speed : i == ACCEL ? &s_set.accel
                                                  : &s_set.precision;
}

static void show(void)
{
    char buf[64];
    const td_mouse_ops_t *m = td_sysinfo()->mouse;
    td_widget_set_text(s_status, m->status && m->status(buf, sizeof(buf)) ? buf : "No mouse attached");
    for (int i = 0; i < SETTINGS; i++)
    {
        int v = *value(i);
        td_progress_set(s_bar[i], (v - s_rows[i].min) * 100 / (s_rows[i].max - s_rows[i].min));
        td_widget_printf(s_value[i], "%3d %%", v);
    }
}

static void change(int i, int v)
{
    if (!td_session_has_right(TD_RIGHT_SYSTEM))
    {
        td_widget_set_text(s_note, "The system permission is required to change these.");
        return;
    }
    if (v < s_rows[i].min)
        v = s_rows[i].min;
    if (v > s_rows[i].max)
        v = s_rows[i].max;
    if (v == *value(i))
        return;
    *value(i) = v;
    bool saved = td_sysinfo()->mouse->set(&s_set);
    td_widget_set_text(s_note, saved ? "Applied and saved." : "Applied, but could not be saved.");
    show();
}

static void on_minus(td_widget_t *w, void *user)
{
    (void)w;
    int i = (int)(intptr_t)user;
    change(i, *value(i) - s_rows[i].step);
}

static void on_plus(td_widget_t *w, void *user)
{
    (void)w;
    int i = (int)(intptr_t)user;
    change(i, *value(i) + s_rows[i].step);
}

static void on_defaults(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    for (int i = 0; i < SETTINGS; i++)
        change(i, s_rows[i].def);
}

static void on_target(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    s_hit_count++;
    td_widget_printf(s_hits, "Hits: %d", s_hit_count);
}

/* A click or drag on a bar sets its value. */
static bool on_event(td_window_t *win, const td_event_t *ev)
{
    (void)win;
    if (ev->type != TD_EV_MOUSE || ev->button != TD_BUTTON_LEFT ||
        (ev->action != TD_MOUSE_PRESS && ev->action != TD_MOUSE_DRAG))
        return false;
    for (int i = 0; i < SETTINGS; i++)
    {
        if (ev->y != ROW_Y(i) || ev->x < BAR_X || ev->x >= BAR_X + BAR_W)
            continue;
        int span = s_rows[i].max - s_rows[i].min, step = s_rows[i].step;
        int v = s_rows[i].min + (ev->x - BAR_X) * span / (BAR_W - 1);
        change(i, (v + step / 2) / step * step);
        return true;
    }
    return false;
}

/* The status line follows the mouse being plugged in or out. */
static void on_tick(td_window_t *win)
{
    (void)win;
    show();
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
        .title = "Mouse",
        .rect = td_rect(-1, -1, 60, 16),
        .flags = TD_WIN_MOVABLE | TD_WIN_CLOSABLE,
        .on_event = on_event,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = 1000,
    };
    s_win = td_win_create(&d);
    if (!s_win)
        return;
    td_sysinfo()->mouse->get(&s_set);
    s_hit_count = 0;

    s_status = td_label(s_win, 2, 1, 0, "");
    for (int i = 0; i < SETTINGS; i++)
    {
        td_label(s_win, 2, ROW_Y(i), 14, s_rows[i].name);
        td_button(s_win, 17, ROW_Y(i), "-", on_minus, (void *)(intptr_t)i);
        s_bar[i] = td_progress(s_win, BAR_X, ROW_Y(i), BAR_W);
        td_button(s_win, BAR_X + BAR_W + 1, ROW_Y(i), "+", on_plus, (void *)(intptr_t)i);
        s_value[i] = td_label(s_win, BAR_X + BAR_W + 7, ROW_Y(i), 6, "");
    }
    td_label(s_win, 2, TARGET_Y, 0, "Try it:");
    for (int i = 0; i < 4; i++)
        td_button(s_win, 11 + 6 * i, TARGET_Y, "o", on_target, NULL);
    s_hits = td_label(s_win, 37, TARGET_Y, 0, "Hits: 0");
    td_button(s_win, 2, 12, "Defaults", on_defaults, NULL);
    s_note = td_label(s_win, 17, 12, 0, td_session_has_right(TD_RIGHT_SYSTEM) ? "" : "The system permission is required to change these.");
    td_widget_set_color(s_note, td_theme()->dim, TD_COLOR_DEFAULT);
    show();
}

static const td_app_t s_app = {"Mouse", launch, "^"};

void td_mouse_register(void)
{
    if (td_sysinfo()->mouse)
        td_app_register(&s_app);
}
