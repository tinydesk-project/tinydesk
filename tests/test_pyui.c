/*
 * test_pyui.c - Python's tdui on the desktop side (apps/pyui.c): canvas
 * pages (draw into a hidden page, show it, copy pages), big characters,
 * pixels and images (shapes, text over pixels, transparency, nine-slice
 * panels, the pixel budget) and the cell and page limits. Requests are sent as a script would, the
 * desktop runs them on its own loop, and the composed screen is checked.
 */
#include "td_test.h"
#include "td_apps.h"
#include "tinydesk/td_canvas.h"

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

/* A composed cell of the canvas. */
static td_cell_t cell(int x, int y)
{
    static td_buffer_t back;
    td_buffer_init(&back, 80, 25);
    td_wm_compose(&back);
    return *td_buffer_cell(&back, CX + x, CY + y);
}

/* An image as a script makes it, handed to the desktop as id. */
static void send_image(int id, int w, int h, const uint8_t *px, const uint8_t *mask)
{
    td_pyui_image_t *img = td_pyui_image_new(w, h, mask != NULL);
    CHECK(img != NULL);
    CHECK(td_pyui_pixels_take(td_pyui_image_bytes(img)));
    memcpy(img->px, px, (size_t)(w * h));
    if (mask)
        memcpy(img->mask, mask, (size_t)(w * h));
    td_pyui_send_image(TD_PYUI_IMAGE, (uint16_t)id, 0, 0, img, NULL);
}

static void send_part(int op, int id, int flags, int img, int a, int b, int c, int d, int e, int f,
                      td_pyui_part_t part)
{
    td_pyui_msg_t m = {0};
    m.op = (uint8_t)op;
    m.flags = (uint8_t)flags;
    m.id = (uint16_t)id;
    m.win = (uint16_t)img;
    m.a = (int16_t)a;
    m.b = (int16_t)b;
    m.c = (int16_t)c;
    m.d = (int16_t)d;
    m.e = (int16_t)e;
    m.f = (int16_t)f;
    m.len = sizeof(part);
    CHECK(td_pyui_send(&m, (const char *)&part));
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

static void test_pixels(void)
{
    uint8_t bg = td_theme()->win_bg;
    send(TD_PYUI_FILL, CANVAS, 0, 0, 0, 12, 4, -1, -1, " ");

    /* A rectangle of pixels: cells (0,0) and (1,0) show them, two pixel
     * rows to a cell (half blocks, the default detail). */
    send(TD_PYUI_RECT, CANVAS, 0, 0, 0, 4, 2, 196, 0, NULL);
    settle();
    td_cell_t c = cell(0, 0);
    CHECK_EQ(c.ch, 0x2580);
    CHECK_EQ(c.fg, 196);
    CHECK_EQ(c.bg, 196);

    /* Half a cell: the other pixels are the cell's background. */
    send(TD_PYUI_RECT, CANVAS, 0, 4, 0, 2, 1, 46, 0, NULL);
    settle();
    c = cell(2, 0);
    CHECK_EQ(c.fg, 46);
    CHECK_EQ(c.bg, bg);

    /* Text with a CLEAR background stands on the pixels' main colour; a
     * blank with a CLEAR background shows the pixels again. */
    send(TD_PYUI_PUT, CANVAS, 0, 0, 0, 15, TD_PYUI_CLEAR, 0, 0, "X");
    settle();
    c = cell(0, 0);
    CHECK_EQ(c.ch, 'X');
    CHECK_EQ(c.fg, 15);
    CHECK_EQ(c.bg, 196);
    send(TD_PYUI_PUT, CANVAS, 0, 0, 0, -1, TD_PYUI_CLEAR, 0, 0, " ");
    settle();
    CHECK_EQ(cell(0, 0).ch, 0x2580);
    CHECK_EQ(cell(0, 0).fg, 196);

    /* An image with a transparent pixel: what is under it stays. */
    static const uint8_t px[4] = {1, 2, 3, 4}, mask[4] = {0, 1, 0, 0};
    send_image(20, 2, 2, px, mask);
    send_part(TD_PYUI_DRAW_IMAGE, CANVAS, 0, 20, 0, 2, 0, 0, 0, 0, (td_pyui_part_t){0, 0, 0});
    settle();
    CHECK(!*error_event());
    c = cell(0, 1);
    CHECK_EQ(c.fg, 1);
    CHECK_EQ(c.bg, 3);

    /* Fine pictures: four pixels to a cell (quadrant characters). */
    td_canvas_set_detail(TD_CANVAS_QUAD);
    td_wm_invalidate();
    uint8_t q[4] = {1, bg, 3, 4};
    uint32_t qch;
    uint8_t qfg, qbg;
    td_canvas_quad_cell(q, &qch, &qfg, &qbg);
    c = cell(0, 1);
    CHECK_EQ(c.ch, qch);
    CHECK_EQ(c.fg, qfg);
    CHECK_EQ(c.bg, qbg);
    td_canvas_set_detail(TD_CANVAS_HALF);
    td_wm_invalidate();

    /* Nine-slice: a 3x3 picture as a 6x6 panel at pixel (16, 0): corners
     * kept, edges and middle stretched. Cell (8,0) has pixels (16..17,
     * 0..1): the corner over the left edge; cell (10,2) has pixels (20..21,
     * 4..5): the middle over the bottom edge. */
    static const uint8_t n9[9] = {10, 11, 12, 13, 14, 15, 16, 17, 18};
    send_image(21, 3, 3, n9, NULL);
    send_part(TD_PYUI_DRAW_IMAGE, CANVAS, 1, 21, 16, 0, 6, 6, 0, 0, (td_pyui_part_t){0, 0, 1});
    settle();
    c = cell(8, 0);
    CHECK_EQ(c.fg, 10);
    CHECK_EQ(c.bg, 13);
    c = cell(10, 2);
    CHECK_EQ(c.fg, 14);
    CHECK_EQ(c.bg, 17);

    /* A sprite: part (1,1) 1x1 of the picture, stretched to 2x2. */
    send_part(TD_PYUI_DRAW_IMAGE, CANVAS, 0, 21, 22, 0, 2, 2, 1, 1, (td_pyui_part_t){1, 1, 0});
    settle();
    c = cell(11, 0);
    CHECK_EQ(c.fg, 14);
    CHECK_EQ(c.bg, 14);

    /* A gradient turns the cells into pixel cells. */
    send(TD_PYUI_GRADIENT, CANVAS, 0, 0, 6, 24, 2, 0, 0, "\x01\x01\x01\xff\xff\xff");   /* no 0 bytes: send() uses strlen() */
    settle();
    CHECK_EQ(cell(0, 3).ch, 0x2580);

    /* Copying a page copies its pixels. */
    send(TD_PYUI_PAGE_COPY, CANVAS, 0, 0, 1, 0, 0, 0, 0, NULL);
    send(TD_PYUI_PAGE, CANVAS, 0, -1, 1, 0, 0, 0, 0, NULL);
    settle();
    CHECK_EQ(cell(1, 0).fg, 196);
    send(TD_PYUI_PAGE, CANVAS, 0, 0, 0, 0, 0, 0, 0, NULL);

    /* Drawing an image that does not exist is an error. */
    send_part(TD_PYUI_DRAW_IMAGE, CANVAS, 0, 99, 0, 0, 0, 0, 0, 0, (td_pyui_part_t){0, 0, 0});
    settle();
    CHECK(strstr(error_event(), "no such image") != NULL);

    /* The pixel budget: more than it holds is refused; an image freed
     * gives its bytes back. */
    CHECK(!td_pyui_pixels_take(TD_PYUI_MAX_PIXELS + 1L));
    send(TD_PYUI_CLOSE, 20, 0, 0, 0, 0, 0, 0, 0, NULL);
    settle();
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
    test_pixels();
    test_limits();

    td_pyui_detach_begin();
    for (int i = 0; i < 20 && !td_pyui_detach_done(); i++)
        settle();
    /* Every image and pixel layer gave its bytes back. */
    CHECK(td_pyui_pixels_take(TD_PYUI_MAX_PIXELS));
    td_pyui_pixels_give(TD_PYUI_MAX_PIXELS);
    return TD_TEST_RESULT();
}
