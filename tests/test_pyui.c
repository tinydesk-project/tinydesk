/*
 * test_pyui.c - Python's tdui on the desktop side (apps/pyui.c): canvas
 * pages (draw into a hidden page, show it, copy pages), big characters and
 * the cell and page limits. Requests are sent as a script would, the
 * desktop runs them on its own loop, and the composed screen is checked.
 */
#include "td_test.h"
#include "td_apps.h"

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

enum
{
    WIN = 1,
    CANVAS = 2,
    WIN_X = 2,
    WIN_Y = 2,
    CX = WIN_X + 1,   /* the canvas's top left cell on the screen */
    CY = WIN_Y + 1
};

static void send(int op, int id, int flags, int a, int b, int c, int d, int e, int f, const char *text)
{
    td_pyui_msg_t m = {0};
    m.op = (uint8_t)op;
    m.flags = (uint8_t)flags;
    m.id = (uint16_t)id;
    m.win = WIN;
    m.a = (int16_t)a;
    m.b = (int16_t)b;
    m.c = (int16_t)c;
    m.d = (int16_t)d;
    m.e = (int16_t)e;
    m.f = (int16_t)f;
    m.len = (uint16_t)(text ? strlen(text) : 0);
    CHECK(td_pyui_send(&m, text ? text : ""));
}

/* Let the desktop run the requests (its tick is every 10 ms). */
static void settle(void)
{
    for (int i = 0; i < 5; i++)
    {
        s_now += 20;
        td_step();
    }
}

static uint32_t at(int x, int y)
{
    static td_buffer_t back;
    td_buffer_init(&back, 80, 25);
    td_wm_compose(&back);
    return td_buffer_cell(&back, CX + x, CY + y)->ch;
}

/* The text of the next error event, or "" if there is none. */
static const char *error_event(void)
{
    static char text[128];
    td_pyui_msg_t m;
    while (td_pyui_recv(&m, text, sizeof(text) - 1))
    {
        if (m.op == TD_PYUI_EV_ERROR)
        {
            text[m.len < sizeof(text) - 1 ? m.len : sizeof(text) - 1] = '\0';
            return text;
        }
    }
    return "";
}

static void test_pages(void)
{
    /* A 10x3 canvas with 3 pages; page 0 is shown and drawn into. */
    send(TD_PYUI_ADD, CANVAS, TD_PYUI_W_CANVAS, 0, 0, 10, 3, 3, 0, NULL);
    send(TD_PYUI_PUT, CANVAS, 0, 0, 0, -1, -1, 0, 0, "A");
    settle();
    CHECK_EQ(at(0, 0), 'A');

    /* Drawing into hidden page 1 does not show. */
    send(TD_PYUI_PAGE, CANVAS, 0, 1, -1, 0, 0, 0, 0, NULL);
    send(TD_PYUI_PUT, CANVAS, 0, 0, 0, -1, -1, 0, 0, "B");
    settle();
    CHECK_EQ(at(0, 0), 'A');

    /* Showing page 1 shows all of it at once. */
    send(TD_PYUI_PAGE, CANVAS, 0, -1, 1, 0, 0, 0, 0, NULL);
    settle();
    CHECK_EQ(at(0, 0), 'B');

    /* Page 1 copied onto page 2, which is then shown. */
    send(TD_PYUI_PAGE_COPY, CANVAS, 0, 1, 2, 0, 0, 0, 0, NULL);
    send(TD_PYUI_PAGE, CANVAS, 0, -1, 2, 0, 0, 0, 0, NULL);
    settle();
    CHECK_EQ(at(0, 0), 'B');
    send(TD_PYUI_PAGE, CANVAS, 0, -1, 0, 0, 0, 0, 0, NULL);
    settle();
    CHECK_EQ(at(0, 0), 'A');

    /* A page that does not exist is refused with an error. */
    send(TD_PYUI_PAGE, CANVAS, 0, 3, -1, 0, 0, 0, 0, NULL);
    settle();
    CHECK(strstr(error_event(), "no such canvas page") != NULL);

    /* Resizing keeps every page's picture. */
    send(TD_PYUI_RESIZE, CANVAS, 0, 12, 4, 0, 0, 0, 0, NULL);
    send(TD_PYUI_PAGE, CANVAS, 0, -1, 1, 0, 0, 0, 0, NULL);
    settle();
    CHECK_EQ(at(0, 0), 'B');
    send(TD_PYUI_PAGE, CANVAS, 0, 0, 0, 0, 0, 0, 0, NULL);
    settle();
    CHECK_EQ(at(0, 0), 'A');
}

static void test_big(void)
{
    /* '1' at scale 1: rows .#. ##. .#. .#. ### -> three cell rows. */
    send(TD_PYUI_BIG, CANVAS, 0, 0, 0, -1, -1, 1, 0, "1");
    settle();
    CHECK_EQ(at(0, 0), 0x2584);   /* lower half: '.' over '#' */
    CHECK_EQ(at(1, 0), 0x2588);   /* both */
    CHECK_EQ(at(2, 0), ' ');
    CHECK_EQ(at(1, 1), 0x2588);
    CHECK_EQ(at(0, 2), 0x2580);   /* the last pixel row: upper half */
    CHECK_EQ(at(2, 2), 0x2580);

    /* Width 2, right-aligned: a blank character, then '7' after a gap. */
    send(TD_PYUI_BIG, CANVAS, 0, 0, 0, -1, -1, 1, 2, "7");
    settle();
    CHECK_EQ(at(0, 0), ' ');
    CHECK_EQ(at(1, 0), ' ');
    CHECK_EQ(at(3, 0), ' ');      /* the gap is blanked too */
    CHECK_EQ(at(4, 0), 0x2580);   /* '7': ### over ..#, so upper, upper, both */
    CHECK_EQ(at(6, 0), 0x2588);

    /* Lower case is drawn as upper case; unknown characters as '?'. */
    send(TD_PYUI_BIG, CANVAS, 0, 0, 0, -1, -1, 1, 0, "t");
    settle();
    CHECK_EQ(at(0, 0), 0x2580);   /* 'T': ### over .#. */
    CHECK_EQ(at(1, 0), 0x2588);
}

static void test_limits(void)
{
    /* More pages than TD_PYUI_MAX_PAGES. */
    send(TD_PYUI_ADD, 3, TD_PYUI_W_CANVAS, 0, 5, 2, 2, TD_PYUI_MAX_PAGES + 1, 0, NULL);
    settle();
    CHECK(strstr(error_event(), "too many canvas pages") != NULL);

    /* Pages count toward the cell limit. */
    int pages = TD_PYUI_MAX_CELLS / 100 + 1 < TD_PYUI_MAX_PAGES ? TD_PYUI_MAX_CELLS / 100 + 1 : TD_PYUI_MAX_PAGES;
    int h = TD_PYUI_MAX_CELLS / (10 * pages) + 1;
    send(TD_PYUI_ADD, 4, TD_PYUI_W_CANVAS, 0, 5, 10, h, pages, 0, NULL);
    settle();
    CHECK(strstr(error_event(), "cells in all") != NULL);
}

int main(void)
{
    td_init(&s_hal);
    td_pyui_start();
    CHECK(td_pyui_attach());
    send(TD_PYUI_OPEN, WIN, 0, 30, 10, WIN_X, WIN_Y, 0, 0, "Pages");
    settle();

    test_pages();
    test_big();
    test_limits();

    td_pyui_detach_begin();
    for (int i = 0; i < 20 && !td_pyui_detach_done(); i++)
        settle();
    return TD_TEST_RESULT();
}
