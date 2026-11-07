/*
 * test_select.c - several selected items: a multi-select list (click,
 * Ctrl+click, Shift+click, dragging over rows, Ctrl+A, Space, a click on
 * one of several) and desktop icons (Ctrl+click, a rectangle dragged over
 * the empty desktop, Ctrl+A, Esc, Del), driven by synthetic events.
 */
#include "td_test.h"
#include "tinydesk/td.h"

#include <stdio.h>
#include <string.h>

static uint32_t s_now = 1000;
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

static void mouse(int action, int x, int y, uint8_t mods)
{
    td_event_t ev = {0};
    ev.type = TD_EV_MOUSE;
    ev.action = (uint8_t)action;
    ev.button = TD_BUTTON_LEFT;
    ev.x = (int16_t)x;
    ev.y = (int16_t)y;
    ev.mods = mods;
    ev.time_ms = s_now;
    s_now += 500;   /* never a double click */
    td_wm_dispatch(&ev);
}
static void click(int x, int y, uint8_t mods)
{
    mouse(TD_MOUSE_PRESS, x, y, mods);
    mouse(TD_MOUSE_RELEASE, x, y, mods);
}
static void key(uint32_t k, uint8_t mods)
{
    td_event_t ev = {0};
    ev.type = TD_EV_KEY;
    ev.key = k;
    ev.mods = mods;
    td_wm_dispatch(&ev);
}

static const char *item(td_widget_t *w, int i, int *fg, void *user)
{
    (void)w;
    (void)fg;
    (void)user;
    static char s[16];
    snprintf(s, sizeof(s), "item %d", i);
    return s;
}

/* Drag and drop of several rows: the source names its marked rows, the
 * target counts what arrives. */
static td_widget_t *s_src_list;
static int s_dropped;
static bool drag_start(td_window_t *w, int x, int y, td_drag_item_t *it)
{
    (void)w;
    (void)x;
    (void)y;
    snprintf(it->name, sizeof(it->name), "row%d", td_list_selected(s_src_list));
    return true;
}
static bool drag_more(td_window_t *w, int index, td_drag_item_t *it)
{
    (void)w;
    int n = 0;
    for (int i = 0; i < s_src_list->count; i++)
        if (i != td_list_selected(s_src_list) && td_list_marked(s_src_list, i) && ++n == index)
        {
            snprintf(it->name, sizeof(it->name), "row%d", i);
            return true;
        }
    return false;
}
static bool drop(td_window_t *w, int x, int y, const td_drag_item_t *it)
{
    (void)w;
    (void)x;
    (void)y;
    (void)it;
    s_dropped++;
    return true;
}

/* A desktop with eight file icons. */
static int s_removed = -1;
static int prov_count(void *u)
{
    (void)u;
    return 8;
}
static const char *prov_label(int i, void *u)
{
    (void)u;
    static char s[16];
    snprintf(s, sizeof(s), "file%d", i);
    return s;
}
static void prov_remove(bool for_good, void *u)
{
    (void)u;
    s_removed = for_good;
}

static int selection(void)
{
    int items[16];
    return td_desktop_selection(items, 16);
}

int main(void)
{
    td_init(&s_hal);
    td_wm_set_screen_size(80, 25);

    /* ---- the list */
    td_window_desc_t d = {.title = "List", .rect = td_rect(10, 2, 30, 14), .flags = TD_WIN_DEFAULT};
    td_window_t *win = td_win_create(&d);
    CHECK(win != NULL);
    td_widget_t *list = td_list(win, td_rect(0, 0, -1, -1), item, NULL, NULL);
    td_list_set_count(list, 10);
    td_list_set_multi(list, true);
    td_rect_t c = td_win_client(win);
    int x = c.x + 2, y0 = c.y;   /* row i is at y0 + i */

    click(x, y0 + 2, 0);
    CHECK_EQ(td_list_marked_count(list), 1);
    CHECK(td_list_marked(list, 2));
    click(x, y0 + 5, TD_MOD_CTRL);      /* add one */
    CHECK_EQ(td_list_marked_count(list), 2);
    CHECK(td_list_marked(list, 5));
    click(x, y0 + 5, TD_MOD_CTRL);      /* and remove it again */
    CHECK_EQ(td_list_marked_count(list), 1);
    click(x, y0 + 2, 0);
    click(x, y0 + 6, TD_MOD_SHIFT);     /* from the last clicked: 2 .. 6 */
    CHECK_EQ(td_list_marked_count(list), 5);
    CHECK(td_list_marked(list, 4) && !td_list_marked(list, 7));

    /* Dragging from an item that is not selected selects the rows passed over. */
    mouse(TD_MOUSE_PRESS, x, y0 + 8, 0);
    mouse(TD_MOUSE_DRAG, x, y0 + 9, 0);
    mouse(TD_MOUSE_DRAG, x, y0 + 7, 0);
    mouse(TD_MOUSE_RELEASE, x, y0 + 7, 0);
    CHECK_EQ(td_list_marked_count(list), 2);
    CHECK(td_list_marked(list, 7) && td_list_marked(list, 8) && !td_list_marked(list, 9));
    CHECK(!td_drag_active());

    /* A click (no drag) on one of several selected: that one alone. */
    click(x, y0 + 8, 0);
    CHECK_EQ(td_list_marked_count(list), 1);
    CHECK(td_list_marked(list, 8));

    key('a', TD_MOD_CTRL);
    CHECK_EQ(td_list_marked_count(list), 10);
    key(TD_KEY_UP, 0);                   /* back to one */
    CHECK_EQ(td_list_marked_count(list), 1);
    CHECK_EQ(td_list_selected(list), 7);
    key(TD_KEY_UP, TD_MOD_SHIFT);
    key(TD_KEY_UP, TD_MOD_SHIFT);
    CHECK_EQ(td_list_marked_count(list), 3);
    key(' ', 0);                         /* Space toggles the current item */
    CHECK_EQ(td_list_marked_count(list), 2);
    CHECK(!td_list_marked(list, 5));
    td_list_set_count(list, 4);          /* other items: nothing kept */
    CHECK(td_list_marked_count(list) <= 1);

    /* Type-ahead: a letter goes to the next item starting with it, round and round. */
    td_list_set_count(list, 10);
    td_list_select(list, 3);
    key('I', 0);
    CHECK_EQ(td_list_selected(list), 4);
    td_list_select(list, 9);
    key('i', 0);
    CHECK_EQ(td_list_selected(list), 0);    /* wraps */
    key('x', 0);
    CHECK_EQ(td_list_selected(list), 0);    /* nothing starts with x */

    /* A list without multi has just its current item. */
    td_list_set_multi(list, false);
    td_list_select(list, 1);
    CHECK_EQ(td_list_marked_count(list), 1);
    CHECK(td_list_marked(list, 1));
    td_win_close(win);

    /* ---- dragging three selected rows onto another window */
    td_window_desc_t sd = {.title = "Source", .rect = td_rect(0, 0, 30, 12), .flags = TD_WIN_DEFAULT,
                           .on_drag_start = drag_start, .on_drag_more = drag_more};
    td_window_desc_t td = {.title = "Target", .rect = td_rect(45, 0, 30, 12), .flags = TD_WIN_DEFAULT, .on_drop = drop};
    td_window_t *target = td_win_create(&td);
    td_window_t *source = td_win_create(&sd);
    s_src_list = td_list(source, td_rect(0, 0, -1, -1), item, NULL, NULL);
    td_list_set_count(s_src_list, 6);
    td_list_set_multi(s_src_list, true);
    c = td_win_client(source);
    click(c.x + 2, c.y + 1, 0);
    click(c.x + 2, c.y + 3, TD_MOD_SHIFT);   /* rows 1 .. 3 */
    td_rect_t tc = td_win_client(target);
    mouse(TD_MOUSE_PRESS, c.x + 2, c.y + 2, 0);   /* on a selected row: drags them */
    mouse(TD_MOUSE_DRAG, c.x + 2, c.y + 5, 0);
    CHECK(td_drag_active());
    mouse(TD_MOUSE_DRAG, tc.x + 3, tc.y + 3, 0);
    mouse(TD_MOUSE_RELEASE, tc.x + 3, tc.y + 3, 0);
    CHECK_EQ(s_dropped, 3);
    CHECK(!td_drag_active());
    CHECK_EQ(td_list_marked_count(s_src_list), 3);   /* still selected after the drag */
    td_win_close(source);
    td_win_close(target);

    /* ---- desktop icons */
    static const td_desktop_provider_t prov = {.count = prov_count, .label = prov_label, .remove = prov_remove};
    td_desktop_set_provider(&prov);
    td_desktop_set_icons(true);
    td_step();
    CHECK_EQ(selection(), 0);

    /* A rectangle over the whole desktop selects every file icon (app icons are not counted). */
    mouse(TD_MOUSE_PRESS, 79, 0, 0);     /* the empty top-right corner */
    mouse(TD_MOUSE_DRAG, 40, 12, 0);
    mouse(TD_MOUSE_DRAG, 0, 23, 0);
    mouse(TD_MOUSE_RELEASE, 0, 23, 0);
    CHECK_EQ(selection(), 8);

    /* A click on the empty desktop clears it; Ctrl+A selects all again; Esc clears. */
    click(79, 0, 0);
    CHECK_EQ(selection(), 0);
    key('a', TD_MOD_CTRL);
    CHECK_EQ(selection(), 8);
    key(TD_KEY_DELETE, TD_MOD_SHIFT);
    CHECK_EQ(s_removed, 1);              /* the provider removes them, for good */
    key(TD_KEY_ESC, 0);
    CHECK_EQ(selection(), 0);
    key(TD_KEY_DELETE, 0);
    CHECK_EQ(s_removed, 1);              /* nothing selected: nothing asked */
    return TD_TEST_RESULT();
}
