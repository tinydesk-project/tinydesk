/*
 * test_wm.c - window manager behaviour driven by synthetic events.
 */
#include "td_test.h"
#include "tinydesk/td.h"

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

static void mouse(int action, int button, int x, int y)
{
    td_event_t ev = {0};
    ev.type = TD_EV_MOUSE;
    ev.action = (uint8_t)action;
    ev.button = (uint8_t)button;
    ev.x = (int16_t)x;
    ev.y = (int16_t)y;
    ev.time_ms = s_now;
    s_now += 500;   /* never a double click */
    td_wm_dispatch(&ev);
}

static void key(uint32_t k, uint8_t mods)
{
    td_event_t ev = {0};
    ev.type = TD_EV_KEY;
    ev.key = k;
    ev.mods = mods;
    td_wm_dispatch(&ev);
}

static int s_clicks;
static void on_click(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    s_clicks++;
}
static bool s_closed;
static void on_close(td_window_t *w)
{
    (void)w;
    s_closed = true;
}

static int s_launches;
static void launch_app(void)
{
    s_launches++;
}
static const td_app_t s_app_a = {"Alpha", launch_app, ">_"};
static const td_app_t s_app_b = {"Beta Two Words", launch_app, NULL};

static void double_click(int x, int y)
{
    td_event_t ev = {0};
    ev.type = TD_EV_MOUSE;
    ev.button = TD_BUTTON_LEFT;
    ev.x = (int16_t)x;
    ev.y = (int16_t)y;
    for (int i = 0; i < 2; i++)
    {
        ev.time_ms = s_now;
        ev.action = TD_MOUSE_PRESS;
        td_wm_dispatch(&ev);
        ev.action = TD_MOUSE_RELEASE;
        td_wm_dispatch(&ev);
        s_now += 100;              /* well inside the double-click time */
    }
}

/* A desktop with one folder and one file (provider items 0 and 1). */
static int s_dropped_on = -2;
static char s_dropped_name[48];
static int prov_count(void *u)
{
    (void)u;
    return 2;
}
static const char *prov_label(int i, void *u)
{
    (void)u;
    return i == 0 ? "Folder" : "file.txt";
}
static bool prov_drag(int i, td_drag_item_t *item, void *u)
{
    (void)u;
    snprintf(item->name, sizeof(item->name), "%s", prov_label(i, NULL));
    item->is_dir = (i == 0);
    return true;
}
static void prov_drop(int i, const td_drag_item_t *item, void *u)
{
    (void)u;
    s_dropped_on = i;
    snprintf(s_dropped_name, sizeof(s_dropped_name), "%s", item->name);
}
static const td_desktop_provider_t s_prov = {
    .count = prov_count,
    .label = prov_label,
    .drag = prov_drag,
    .drop = prov_drop,
};

static bool s_win_drop;
static bool win_on_drop(td_window_t *w, int x, int y, const td_drag_item_t *item)
{
    (void)w;
    (void)x;
    (void)y;
    s_win_drop = strcmp(item->name, "file.txt") == 0;
    return true;
}

static void test_drag_and_drop(void)
{
    static td_buffer_t back;
    td_wm_init(80, 25);
    td_desktop_set_provider(&s_prov);
    td_wm_compose(&back);
    /* 2 apps -> provider item 0 is icon 2 (slot 2, column 0 row 2) and
     * item 1 is icon 3 (column 1, row 0). Slots are spread over the 80 x 24
     * desktop: 6 columns from x 1 to 68, 3 rows at y 1, 9 and 18. */
    int folder_x = 6, folder_y = 19;
    int file_x = 14 + 5, file_y = 2;

    /* Drag the file onto the folder. */
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, file_x, file_y);
    mouse(TD_MOUSE_DRAG, TD_BUTTON_LEFT, file_x + 3, file_y + 2);
    CHECK(td_drag_active());
    mouse(TD_MOUSE_DRAG, TD_BUTTON_LEFT, folder_x, folder_y);
    mouse(TD_MOUSE_RELEASE, TD_BUTTON_LEFT, folder_x, folder_y);
    CHECK(!td_drag_active());
    CHECK_EQ(s_dropped_on, 0);
    CHECK(strcmp(s_dropped_name, "file.txt") == 0);

    /* A desktop icon dropped on the empty desktop moves there: arranging
     * stops and every icon gets a place; the provider is not asked. */
    td_icon_place_t places[8];
    s_dropped_on = -2;
    CHECK(td_wm_icon_arrange());
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, file_x, file_y);
    mouse(TD_MOUSE_DRAG, TD_BUTTON_LEFT, 60, 12);
    mouse(TD_MOUSE_RELEASE, TD_BUTTON_LEFT, 60, 12);
    CHECK_EQ(s_dropped_on, -2);
    CHECK(!td_wm_icon_arrange());
    CHECK_EQ(td_wm_icon_places(places, 8), 4);
    /* It is there now: a drag from its new place carries the same file. */
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, 60, 11);
    mouse(TD_MOUSE_DRAG, TD_BUTTON_LEFT, folder_x, folder_y);
    mouse(TD_MOUSE_RELEASE, TD_BUTTON_LEFT, folder_x, folder_y);
    CHECK_EQ(s_dropped_on, 0);
    td_wm_set_icon_places(NULL, 0);
    td_wm_set_icon_arrange(true);

    /* A plain click (no movement) does not start a drag. */
    s_dropped_on = -2;
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, file_x, file_y);
    mouse(TD_MOUSE_RELEASE, TD_BUTTON_LEFT, file_x, file_y);
    CHECK_EQ(s_dropped_on, -2);

    /* Dropping on a window goes to its on_drop. */
    td_window_desc_t d = {.title = "Target", .rect = td_rect(40, 5, 30, 10), .flags = TD_WIN_DEFAULT, .on_drop = win_on_drop};
    td_win_create(&d);
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, file_x, file_y);
    mouse(TD_MOUSE_DRAG, TD_BUTTON_LEFT, 50, 9);
    mouse(TD_MOUSE_RELEASE, TD_BUTTON_LEFT, 50, 9);
    CHECK(s_win_drop);
    td_desktop_set_provider(NULL);
}

/* Desktop icon labels split by characters, not bytes. */
static int utf8_count(void *u)
{
    (void)u;
    return 2;
}
static const char *utf8_label(int i, void *u)
{
    (void)u;
    return i == 0 ? "caf\xC3\xA9 cr\xC3\xA8me br\xC3\xBBl\xC3\xA9"
                    "e" /* café crème brûlée */
                  : "\xE4\xB8\xAD\xE6\x96\x87\xE4\xB8\xAD\xE6\x96\x87\xE4\xB8\xAD\xE6\x96\x87\xE4\xB8\xAD"
                    "\xE6\x96\x87\xE4\xB8\xAD\xE6\x96\x87\xE4\xB8\xAD\xE6\x96\x87\xE4\xB8\xAD\xE6\x96\x87"; /* 中文 x 7 */
}
static const td_desktop_provider_t s_utf8_prov = {.count = utf8_count, .label = utf8_label};

/* First row from y0 on which the code points of `text` appear in a row, or -1. */
static int find_text_from(const td_buffer_t *b, const char *text, int y0)
{
    uint32_t want[32];
    int n = 0;
    while (n < 32 && (want[n] = td_utf8_next(&text)) != 0)
        n++;
    for (int y = y0; y < b->rows; y++)
        for (int x = 0; x + n <= b->cols; x++)
        {
            int k = 0;
            while (k < n && td_buffer_cell((td_buffer_t *)b, x + k, y)->ch == want[k])
                k++;
            if (k == n)
                return y;
        }
    return -1;
}

static int find_text(const td_buffer_t *b, const char *text)
{
    return find_text_from(b, text, 0);
}

static void test_icon_labels_utf8(void)
{
    static td_buffer_t back;
    td_wm_init(80, 25);
    td_desktop_set_provider(&s_utf8_prov);
    td_wm_compose(&back);
    /* 11 columns a line: "café crème" / "brûlée" break at the space ... */
    int y1 = find_text(&back, "caf\xC3\xA9 cr\xC3\xA8me");
    int y2 = find_text(&back, "br\xC3\xBBl\xC3\xA9"
                              "e");
    CHECK(y1 >= 0 && y2 == y1 + 1);
    /* ... 14 CJK characters without a space split 11 + 3, whole. */
    int c1 = find_text(&back, "\xE4\xB8\xAD\xE6\x96\x87\xE4\xB8\xAD\xE6\x96\x87\xE4\xB8\xAD\xE6\x96\x87\xE4\xB8\xAD"
                              "\xE6\x96\x87\xE4\xB8\xAD\xE6\x96\x87\xE4\xB8\xAD");
    int c2 = find_text_from(&back, "\xE6\x96\x87\xE4\xB8\xAD\xE6\x96\x87", c1 + 1); /* also inside line 1 */
    CHECK(c1 >= 0 && c2 == c1 + 1);
    /* No character was cut in half anywhere. */
    CHECK_EQ(find_text(&back, "\xEF\xBF\xBD"), -1);
    td_desktop_set_provider(NULL);
}

static void test_icons_and_hover(void)
{
    static td_buffer_t back;
    td_wm_init(80, 25);
    td_app_register(&s_app_a);
    td_app_register(&s_app_b);
    td_wm_compose(&back);

    /* Icons: slot 0 at (1,1), 12 wide; the glyph box is centred in it. */
    const td_cell_t *glyph = td_buffer_cell(&back, 5 + 1, 1 + 1);
    CHECK_EQ(glyph->ch, '>');

    /* Double-clicking an icon opens its app; one click only selects. */
    s_launches = 0;
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, 6, 2);
    CHECK_EQ(s_launches, 0);
    double_click(6, 2);
    CHECK_EQ(s_launches, 1);

    /* The keyboard reaches icons while no window is focused. */
    key(TD_KEY_DOWN, 0);
    key(TD_KEY_ENTER, 0);
    CHECK_EQ(s_launches, 2);

    /* Hover: moving over the start menu moves its highlight. */
    td_wm_toggle_start_menu();
    td_wm_compose(&back);
    int apps = td_app_count();
    int first_item_y = 24 - (apps + 2 + 3) + 1;   /* menu sits above the taskbar */
    mouse(TD_MOUSE_MOVE, TD_BUTTON_NONE, 3, first_item_y + 1);
    CHECK(td_wm_needs_redraw());
    td_wm_compose(&back);
    const td_cell_t *sel = td_buffer_cell(&back, 3, first_item_y + 1);
    CHECK_EQ(sel->bg, td_theme()->menu_select_bg);

    /* Moving within the same item does not trigger another frame. */
    td_wm_compose(&back);
    mouse(TD_MOUSE_MOVE, TD_BUTTON_NONE, 4, first_item_y + 1);
    CHECK(!td_wm_needs_redraw());
}

/* The start menu's tail: a desktop whose registry and start items together
 * exceed what the menu can show must still reach "Redraw screen" and "Exit".
 * Exit is the only caller of td_quit(), so an entry that cannot be chosen is a
 * desktop that cannot be left from its own menu.
 *
 * No build configuration is needed to reach this: at the default TD_MAX_APPS
 * of 16, two extra start items leave the old fixed menu holding 15 apps, so the
 * sixteenth is dropped and every entry below the separator shifts up by one. */
static void test_start_menu_tail(void)
{
    static td_buffer_t back;
    static char names[TD_MAX_APPS][24];
    static td_app_t apps[TD_MAX_APPS];

    td_wm_init(80, 25);   /* no window or dialog left over from the tests before */

    /* Fill the registry: the earlier tests registered two apps, the rest are
     * named here so a dropped one is visible in the composed frame. */
    for (int i = td_app_count(); i < TD_MAX_APPS; i++)
    {
        snprintf(names[i], sizeof(names[i]), "Tail app %d", i);
        apps[i].name = names[i];
        apps[i].launch = launch_app;
        apps[i].icon = NULL;
        CHECK_EQ(td_app_register(&apps[i]), 0);
    }
    CHECK_EQ(td_app_count(), TD_MAX_APPS);
    td_wm_add_start_item("Extra one", launch_app);
    td_wm_add_start_item("Extra two", launch_app);

    /* Both fixed entries are on the menu.  Their labels appear nowhere else on
     * the desktop, unlike an app's name, which is also drawn under its icon.
     * A menu taller than the screen scrolls, so look after END has brought
     * its end into view. */
    td_wm_toggle_start_menu();

    /* END is the last item, and the last item quits.  An offset computed from
     * the registry rather than from what the menu placed lands on "Redraw
     * screen" instead, which returns true and leaves the desktop running. */
    key(TD_KEY_END, 0);
    td_wm_compose(&back);
    CHECK(find_text(&back, "Redraw screen") >= 0);
    CHECK(find_text(&back, "Exit") >= 0);
    key(TD_KEY_ENTER, 0);
    CHECK(!td_step());
}
int main(void)
{
    td_init(&s_hal);   /* no size answer: falls back to 80x25 */
    CHECK_EQ(td_stats()->cols, 80);
    CHECK_EQ(td_stats()->rows, 25);

    td_window_desc_t d = {.title = "A", .rect = td_rect(5, 3, 30, 10), .flags = TD_WIN_DEFAULT, .on_close = on_close};
    td_window_t *a = td_win_create(&d);
    d.title = "B";
    d.rect = td_rect(20, 6, 30, 10);
    d.on_close = NULL;
    td_window_t *b = td_win_create(&d);
    CHECK(td_win_focused() == b);

    /* Clicking A brings it to the front. */
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, 6, 5);
    mouse(TD_MOUSE_RELEASE, TD_BUTTON_LEFT, 6, 5);
    CHECK(td_win_focused() == a);

    /* Drag A by its title bar. */
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, 10, 3);
    mouse(TD_MOUSE_DRAG, TD_BUTTON_LEFT, 14, 7);
    mouse(TD_MOUSE_RELEASE, TD_BUTTON_LEFT, 14, 7);
    CHECK_EQ(a->rect.x, 9);
    CHECK_EQ(a->rect.y, 7);

    /* Dragging far away is clamped: the title bar stays on screen. */
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, 12, 7);
    mouse(TD_MOUSE_DRAG, TD_BUTTON_LEFT, 200, 200);
    mouse(TD_MOUSE_RELEASE, TD_BUTTON_LEFT, 200, 200);
    CHECK_EQ(a->rect.x, 80 - 30);
    CHECK_EQ(a->rect.y, 23);

    /* Resize with the bottom-right corner, respecting the minimum. */
    td_win_move(a, 0, 0);
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, 29, 9);
    mouse(TD_MOUSE_DRAG, TD_BUTTON_LEFT, 39, 14);
    mouse(TD_MOUSE_RELEASE, TD_BUTTON_LEFT, 39, 14);
    CHECK_EQ(a->rect.w, 40);
    CHECK_EQ(a->rect.h, 15);
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, 39, 14);
    mouse(TD_MOUSE_DRAG, TD_BUTTON_LEFT, 1, 1);
    mouse(TD_MOUSE_RELEASE, TD_BUTTON_LEFT, 1, 1);
    CHECK_EQ(a->rect.w, a->min_w);
    CHECK_EQ(a->rect.h, a->min_h);

    /* A button reacts to a click and to Enter; Tab moves focus. */
    td_win_resize(a, 30, 10);
    td_widget_t *btn1 = td_button(a, 1, 1, "One", on_click, NULL);
    td_widget_t *btn2 = td_button(a, 1, 3, "Two", on_click, NULL);
    CHECK(a->focus == btn1);
    td_rect_t r = td_widget_rect(btn2);
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, r.x + 1, r.y);
    mouse(TD_MOUSE_RELEASE, TD_BUTTON_LEFT, r.x + 1, r.y);
    CHECK_EQ(s_clicks, 1);
    CHECK(a->focus == btn2);
    key(TD_KEY_TAB, 0);
    CHECK(a->focus == btn1);
    key(TD_KEY_ENTER, 0);
    CHECK_EQ(s_clicks, 2);

    /* F6 cycles windows. */
    key(TD_KEY_F6, 0);
    CHECK(td_win_focused() == b);

    /* The [x] button closes a window. */
    td_win_focus(a);
    mouse(TD_MOUSE_PRESS, TD_BUTTON_LEFT, a->rect.x + a->rect.w - 3, a->rect.y);
    CHECK(s_closed);
    CHECK(!td_win_is_open(a));
    CHECK(td_win_focused() == b);

    /* A modal message box takes the input; Esc closes it. */
    td_window_t *m = td_msgbox("Q", "Sure?", "Yes|No", NULL, NULL);
    CHECK(td_win_focused() == m);
    td_win_focus(b);
    CHECK(td_win_focused() == m);
    key(TD_KEY_ESC, 0);
    CHECK(!td_win_is_open(m));

    /* Composing works and fills the whole screen. */
    static td_buffer_t back;
    td_wm_compose(&back);
    CHECK_EQ(back.cols, 80);

    test_icons_and_hover();
    test_icon_labels_utf8();
    test_drag_and_drop();
    /* Authentication gate: unrelated windows, global shortcuts and even
     * a reused login-window slot stay inaccessible until explicitly unlocked. */
    td_wm_close_all();
    td_window_desc_t gd = {.title = "Gate", .rect = {0, 0, 80, 25}, .flags = TD_WIN_FULLSCREEN};
    td_window_t *gate = td_win_create(&gd);
    td_button(gate, 2, 2, "Sign in", on_click, NULL);
    td_wm_set_input_gate(true, gate);
    td_window_desc_t hd = {.title = "Private", .rect = {5, 5, 30, 10}};
    td_window_t *hidden = td_win_create(&hd);
    td_button(hidden, 1, 1, "Private action", on_click, NULL);
    int clicks = s_clicks;
    key(TD_KEY_F10, 0);
    CHECK(!td_menu_is_open());
    key(TD_KEY_F6, 0);
    key(TD_KEY_ENTER, 0);
    CHECK_EQ(s_clicks, clicks + 1);
    td_win_close(gate);
    td_window_t *reuse = td_win_create(&gd);
    CHECK(reuse == gate);
    td_button(reuse, 2, 2, "Other action", on_click, NULL);
    key(TD_KEY_ENTER, 0);
    CHECK_EQ(s_clicks, clicks + 1);
    td_wm_set_input_gate(false, NULL);
    key(TD_KEY_ENTER, 0);
    CHECK_EQ(s_clicks, clicks + 2);
    test_start_menu_tail();
    return TD_TEST_RESULT();
}
