/*
 * test_osk.c - the on-screen keyboard (apps/osk.c) and what the window
 * manager does for it: keys type on release over the same key, Shift and
 * the symbols, a window that never takes the focus (TD_WIN_NO_FOCUS)
 * stays on top, also above a dialog, and still gets its clicks; and the
 * pinned desktop icon goes to the bottom right.
 */
#include "td_apps.h"
#include "td_test.h"

static uint32_t s_now;
static int fake_read(void *ctx)
{
    (void)ctx;
    return -1;
}
static int fake_write(void *ctx, const uint8_t *b, int n)
{
    (void)ctx;
    (void)b;
    return n;
}
static uint32_t fake_millis(void *ctx)
{
    (void)ctx;
    return s_now;
}
static void fake_sleep(void *ctx, uint32_t ms)
{
    (void)ctx;
    s_now += ms;
}
static const td_hal_t s_hal = {fake_read, fake_write, fake_millis, fake_sleep, NULL};

static td_event_t ev_mouse(int action, int x, int y)
{
    td_event_t ev = {0};
    ev.type = TD_EV_MOUSE;
    ev.action = (uint8_t)action;
    ev.button = TD_BUTTON_LEFT;
    ev.x = (int16_t)x;
    ev.y = (int16_t)y;
    return ev;
}

/* Press and release at x, y; what the keys type. */
static int tap(td_osk_t *k, int x, int y)
{
    bool used;
    td_event_t ev = ev_mouse(TD_MOUSE_PRESS, x, y);
    td_osk_mouse(k, &ev, &used);
    ev = ev_mouse(TD_MOUSE_RELEASE, x, y);
    return td_osk_mouse(k, &ev, &used);
}

static void test_keys(void)
{
    td_osk_t k;
    /* 96 x 20: units of 8 columns, rows of 4 lines, from 0, 0. */
    td_osk_init(&k, td_rect(0, 0, 96, 20), "Sign in", true);
    CHECK_EQ(tap(&k, 2, 1), '1');
    CHECK_EQ(tap(&k, 2, 5), 'q');
    CHECK_EQ(tap(&k, 7, 1), TD_OSK_NONE);   /* the gap between keys */
    CHECK_EQ(tap(&k, 85, 1), TD_KEY_BACKSPACE);
    CHECK_EQ(tap(&k, 85, 5), TD_KEY_ENTER);
    CHECK_EQ(tap(&k, 85, 9), TD_OSK_NONE);   /* Shift */
    CHECK(k.shift);
    CHECK_EQ(tap(&k, 2, 1), '!');
    CHECK_EQ(tap(&k, 2, 5), 'Q');
    CHECK_EQ(tap(&k, 85, 13), TD_OSK_NONE);   /* #+= */
    CHECK(k.sym);
    CHECK_EQ(tap(&k, 2, 9), '-');
    CHECK_EQ(tap(&k, 30, 17), ' ');
    CHECK_EQ(tap(&k, 70, 17), TD_OSK_CLEAR);
    CHECK_EQ(tap(&k, 85, 17), TD_OSK_HIDE);

    /* A finger that slides off the key types nothing. */
    bool used;
    td_event_t ev = ev_mouse(TD_MOUSE_PRESS, 2, 1);
    td_osk_mouse(&k, &ev, &used);
    CHECK(used);
    CHECK_EQ(k.held, 0);
    ev = ev_mouse(TD_MOUSE_RELEASE, 10, 1);
    CHECK_EQ(td_osk_mouse(&k, &ev, &used), TD_OSK_NONE);
    CHECK_EQ(k.held, -1);

    /* Without a password the key beside Hide is Esc. */
    td_osk_init(&k, td_rect(0, 0, 96, 20), NULL, false);
    CHECK_EQ(tap(&k, 70, 17), TD_KEY_ESC);
    /* Small areas still give whole keys of one line. */
    td_osk_init(&k, td_rect(0, 0, 40, 5), NULL, false);
    CHECK_EQ(tap(&k, 3, 0), '1');   /* units of 3 from x 2 */
    CHECK_EQ(tap(&k, 3, 4), TD_KEY_LEFT);
}

static int s_typed;
static bool text_event(td_window_t *w, const td_event_t *ev)
{
    (void)w;
    if (ev->type == TD_EV_KEY)
        s_typed = (int)ev->key;
    return ev->type == TD_EV_KEY;
}
static int s_tool_clicks;
static bool tool_event(td_window_t *w, const td_event_t *ev)
{
    (void)w;
    if (ev->type == TD_EV_MOUSE && ev->action == TD_MOUSE_RELEASE)
        s_tool_clicks++;
    return true;
}

static void test_no_focus(void)
{
    td_window_desc_t a = {.title = "Text", .rect = td_rect(2, 2, 30, 10), .flags = TD_WIN_DEFAULT, .on_event = text_event};
    td_window_desc_t t = {.title = "Keys", .rect = td_rect(10, 6, 40, 10), .flags = TD_WIN_MOVABLE | TD_WIN_NO_FOCUS,
                          .on_event = tool_event};
    td_window_t *text = td_win_create(&a);
    td_window_t *tool = td_win_create(&t);
    CHECK(td_win_focused() == text);   /* the tool did not take it */
    CHECK(td_wm_window_at(12, 8) == tool);   /* but it is on top */
    td_event_t ev = ev_mouse(TD_MOUSE_PRESS, 20, 10);
    td_wm_dispatch(&ev);
    ev = ev_mouse(TD_MOUSE_RELEASE, 20, 10);
    td_wm_dispatch(&ev);
    CHECK_EQ(s_tool_clicks, 1);
    CHECK(td_win_focused() == text);
    /* Keys sent the way the keyboard app sends them reach the text. */
    td_event_t key = {.type = TD_EV_KEY, .key = 'x'};
    td_wm_dispatch(&key);
    CHECK_EQ(s_typed, 'x');
    /* A later window opens below the tool; a dialog too, and the tool
     * still gets its clicks while the dialog holds the focus. */
    td_window_desc_t b = {.title = "Later", .rect = td_rect(12, 7, 20, 8), .flags = TD_WIN_DEFAULT};
    td_window_t *later = td_win_create(&b);
    CHECK(td_win_focused() == later);
    CHECK(td_wm_window_at(14, 9) == tool);
    td_window_desc_t m = {.title = "Ask", .rect = td_rect(14, 8, 20, 6), .flags = TD_WIN_MODAL};
    td_window_t *dialog = td_win_create(&m);
    CHECK(td_win_focused() == dialog);
    CHECK(td_wm_window_at(16, 9) == tool);
    ev = ev_mouse(TD_MOUSE_PRESS, 45, 14);
    td_wm_dispatch(&ev);
    ev = ev_mouse(TD_MOUSE_RELEASE, 45, 14);
    td_wm_dispatch(&ev);
    CHECK_EQ(s_tool_clicks, 2);
    CHECK(td_win_focused() == dialog);
    td_wm_close_all();
}

static void launch_none(void)
{
}

static void test_pinned_icon(void)
{
    static const td_app_t apps[] = {{"One", launch_none, "1 "}, {"Two", launch_none, "2 "}, {"Bin", launch_none, "B "}};
    for (int i = 0; i < 3; i++)
        td_app_register(&apps[i]);
    td_wm_pin_icon("Bin");
    static td_buffer_t back;
    /* 80 x 24 desktop, medium icons: the last slot (bottom right) starts
     * at x 68, y 18; its glyph is drawn there, the others top left. */
    td_wm_compose(&back);
    bool bin = false, one = false;
    for (int y = 18; y < 22; y++)
        for (int x = 68; x < 80; x++)
            bin = bin || back.cells[y * back.cols + x].ch == 'B';
    for (int y = 1; y < 5; y++)
        for (int x = 1; x < 13; x++)
            one = one || back.cells[y * back.cols + x].ch == '1';
    CHECK(bin);
    CHECK(one);
}

int main(void)
{
    td_init(&s_hal);
    td_wm_init(80, 25);
    test_keys();
    test_no_focus();
    test_pinned_icon();
    return TD_TEST_RESULT();
}
