/*
 * keyboard.c - Keyboard: the layout and key repeat of a keyboard the board
 * reads itself (a USB keyboard on the ESP32-S3; td_sysinfo()->keyboard).
 * Only there does the app exist. < and > choose the layout; the repeat
 * delay and rate have a bar to click or drag and - / + buttons; changes
 * apply at once and the port saves them (root only). A text box is there
 * to try the keys out.
 */
#include <stdint.h>

#include "td_apps.h"

enum
{
    DELAY,
    RATE,
    SETTINGS
};

static const struct
{
    const char *name, *unit;
    int min, max, step, def;
} s_rows[SETTINGS] = {
    {"Repeat delay", "ms", 200, 1000, 50, 500},
    {"Repeat rate", "/s", 2, 40, 2, 30},
};

#define LAYOUT_Y 3
#define ROW_Y(i) (5 + 2 * (i))
#define BAR_X    23
#define BAR_W    20

static td_window_t *s_win;
static td_widget_t *s_status, *s_layout, *s_bar[SETTINGS], *s_value[SETTINGS], *s_note;
static td_keyboard_settings_t s_set;

static int *value(int i)
{
    return i == DELAY ? &s_set.delay_ms : &s_set.rate;
}

static int layouts(void)
{
    int n = 0;
    while (td_sysinfo()->keyboard->layout_name(n))
        n++;
    return n;
}

static void show(void)
{
    char buf[64];
    const td_keyboard_ops_t *k = td_sysinfo()->keyboard;
    td_widget_set_text(s_status, k->status && k->status(buf, sizeof(buf)) ? buf : "No keyboard attached");
    const char *name = k->layout_name(s_set.layout);
    td_widget_set_text(s_layout, name ? name : "?");
    for (int i = 0; i < SETTINGS; i++)
    {
        int v = *value(i);
        td_progress_set(s_bar[i], (v - s_rows[i].min) * 100 / (s_rows[i].max - s_rows[i].min));
        td_widget_printf(s_value[i], "%4d %s", v, s_rows[i].unit);
    }
}

static void save(void)
{
    bool saved = td_sysinfo()->keyboard->set(&s_set);
    td_widget_set_text(s_note, saved ? "Applied and saved." : "Applied, but could not be saved.");
    show();
}

static bool may_change(void)
{
    if (td_session_has_right(TD_RIGHT_SYSTEM))
        return true;
    td_widget_set_text(s_note, "The system permission is required to change these.");
    return false;
}

static void change(int i, int v)
{
    if (!may_change())
        return;
    if (v < s_rows[i].min)
        v = s_rows[i].min;
    if (v > s_rows[i].max)
        v = s_rows[i].max;
    if (v == *value(i))
        return;
    *value(i) = v;
    save();
}

static void on_layout(td_widget_t *w, void *user)
{
    (void)w;
    if (!may_change())
        return;
    int n = layouts();
    s_set.layout = (s_set.layout + (int)(intptr_t)user + n) % n;
    save();
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
    if (!may_change())
        return;
    s_set.layout = 0;
    for (int i = 0; i < SETTINGS; i++)
        *value(i) = s_rows[i].def;
    save();
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

/* The status line follows the keyboard being plugged in or out. */
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
        .title = "Keyboard",
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
    td_sysinfo()->keyboard->get(&s_set);

    s_status = td_label(s_win, 2, 1, 0, "");
    td_label(s_win, 2, LAYOUT_Y, 14, "Layout");
    td_button(s_win, 17, LAYOUT_Y, "<", on_layout, (void *)(intptr_t)-1);
    s_layout = td_label(s_win, BAR_X, LAYOUT_Y, BAR_W, "");
    td_widget_set_align(s_layout, TD_ALIGN_CENTER);
    td_button(s_win, BAR_X + BAR_W + 1, LAYOUT_Y, ">", on_layout, (void *)(intptr_t)1);
    for (int i = 0; i < SETTINGS; i++)
    {
        td_label(s_win, 2, ROW_Y(i), 14, s_rows[i].name);
        td_button(s_win, 17, ROW_Y(i), "-", on_minus, (void *)(intptr_t)i);
        s_bar[i] = td_progress(s_win, BAR_X, ROW_Y(i), BAR_W);
        td_button(s_win, BAR_X + BAR_W + 1, ROW_Y(i), "+", on_plus, (void *)(intptr_t)i);
        s_value[i] = td_label(s_win, BAR_X + BAR_W + 7, ROW_Y(i), 8, "");
    }
    td_label(s_win, 2, 10, 0, "Try it:");
    td_widget_t *try_box = td_textbox(s_win, 11, 10, 44, 200, NULL, NULL);
    td_button(s_win, 2, 12, "Defaults", on_defaults, NULL);
    s_note = td_label(s_win, 17, 12, 0, td_session_has_right(TD_RIGHT_SYSTEM) ? "" : "The system permission is required to change these.");
    td_widget_set_color(s_note, td_theme()->dim, TD_COLOR_DEFAULT);
    td_widget_focus(try_box);
    show();
}

static const td_app_t s_app = {"Keyboard", launch, "Kb"};

void td_keyboard_register(void)
{
    if (td_sysinfo()->keyboard)
        td_app_register(&s_app);
}
