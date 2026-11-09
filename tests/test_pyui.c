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
#include "tinydesk/td_picture.h"
#include "tinydesk/td_vterm.h"
#include <stdlib.h>

static uint32_t s_now = 1000;
static td_vterm_t s_terminal;
static uint8_t s_reply[512];
static unsigned s_reply_head, s_reply_tail;
static void terminal_reply(void *ctx, const char *data, int n)
{
    (void)ctx;
    for (int i = 0; i < n; i++)
        s_reply[s_reply_head++ % sizeof(s_reply)] = (uint8_t)data[i];
}
static int fake_read(void *ctx)
{
    (void)ctx;
    return s_reply_head == s_reply_tail ? -1 : s_reply[s_reply_tail++ % sizeof(s_reply)];
}
static char s_out[65536];   /* what the desktop sent, for the picture test */
static int s_out_len;
static int fake_write(void *ctx, const uint8_t *b, int n)
{
    (void)ctx;
    td_vterm_write(&s_terminal, b, n);
    for (int i = 0; i < n && s_out_len < (int)sizeof(s_out) - 1; i++)
        s_out[s_out_len++] = (char)b[i];
    s_out[s_out_len] = 0;
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

    /* Text in pixels: '1' of the 8x16 font at pixel (0, 0). Its rows 2
     * and 3 are 0x18 and 0x38, so cell (1, 1) (pixels 2..3, 2..3) has
     * the background at its upper left and the text's colour elsewhere. */
    send(TD_PYUI_FILL, CANVAS, 0, 0, 0, 12, 4, -1, -1, " ");
    send(TD_PYUI_PTEXT, CANVAS, 0, 0, 0, 9, TD_PYUI_CLEAR, 1, 0, "1");
    settle();
    c = cell(1, 1);
    CHECK_EQ(c.fg, bg);
    CHECK_EQ(c.bg, 9);
    CHECK_EQ(cell(3, 0).ch, ' ');   /* a CLEAR background leaves other cells alone */

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

static void send_picture(int img, int x, int y, int w, int h, const char *label)
{
    char text[64];
    td_pyui_picture_t part = {0, 0, 1, 15, 1};
    memcpy(text, &part, sizeof(part));
    int n = (int)strlen(label);
    memcpy(text + sizeof(part), label, (size_t)n);
    td_pyui_msg_t m = {0};
    m.op = TD_PYUI_PICTURE;
    m.flags = 1;
    m.id = CANVAS;
    m.win = (uint16_t)img;
    m.a = (int16_t)x;
    m.b = (int16_t)y;
    m.c = (int16_t)w;
    m.d = (int16_t)h;
    m.len = (uint16_t)(sizeof(part) + (size_t)n);
    CHECK(td_pyui_send(&m, text));
}

static int s_local_pictures;
static bool local_picture(void *user, int x, int y, const uint8_t *pixels, int w, int h)
{
    (void)user;
    CHECK(x >= 0 && y >= 0);
    CHECK(pixels != NULL && w == 64 && h == 32);
    s_local_pictures++;
    return true;
}

static unsigned font_white;
static bool font_picture(void *user, int x, int y, const uint8_t *px, int w, int h)
{
    (void)user;
    (void)x;
    (void)y;
    if (w == 200 && h == 80)
    {
        font_white = 0;
        for (int i = 0; i < w * h; i++)
            font_white += px[i] == 231;
    }
    return true;
}
static void test_generated_fonts(void)
{
    /* One portable glyph, repeated beyond the old 48-byte picture label.
     * Exercise native pixels, retained page copies and refcounted text. */
    const uint8_t bytes[51] = {'T', 'D', 'F', 'O', 'N', 'T', '0', '1', 32, 0, 1, 0, 3, 0, 2, 0, 1, 0, 8, 0,
                               32, 0, 0, 0, 48, 0, 0, 0, 51, 0, 0, 0, 63, 0, 0, 0, 48, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 128, 128, 128};
    size_t size = sizeof(td_pyui_font_t) + sizeof(bytes);
    CHECK(td_pyui_pixels_take((long)size));
    td_pyui_font_t *font = malloc(size);
    CHECK(font != NULL);
    if (!font)
    {
        td_pyui_pixels_give((long)size);
        return;
    }
    font->bytes = size;
    memcpy(font->data, bytes, sizeof(bytes));
    CHECK(td_font_open(&font->face, font->data, sizeof(bytes)));
    td_pyui_send_font(90, font, NULL);
    send(TD_PYUI_OPEN, 79, 0, 24, 6, 0, 0, 0, 0, "Fonts");
    td_pyui_msg_t add = {.op = TD_PYUI_ADD, .id = 80, .win = 79, .flags = TD_PYUI_W_CANVAS, .c = 20, .d = 4, .e = 3};
    CHECK(td_pyui_send(&add, ""));
    td_term_set_sixel(true);
    td_term_set_cell_px(10, 20);
    td_pyui_msg_t m = {.op = TD_PYUI_FONT_TEXT, .id = 80, .win = 90, .c = 20, .d = 4, .e = 231, .f = 17};
    char long_text[201];
    memset(long_text, '?', 200);
    long_text[200] = 0;
    m.len = 200;
    CHECK(td_pyui_send(&m, long_text));
    td_picture_set_sink(font_picture, NULL);
    font_white = 0;
    settle();
    CHECK_EQ(font_white, 300);
    send(TD_PYUI_PAGE_COPY, 80, 0, 0, 1, 0, 0, 0, 0, NULL);
    send(TD_PYUI_PAGE, 80, 0, 1, 1, 0, 0, 0, 0, NULL);
    settle();
    CHECK_EQ(font_white, 300);
    m.len = 1;
    CHECK(td_pyui_send(&m, "?"));
    settle();
    CHECK_EQ(font_white, 3);
    send(TD_PYUI_PAGE, 80, 0, 0, 0, 0, 0, 0, 0, NULL);
    settle();
    CHECK_EQ(font_white, 300);
    /* A transparent multiline fallback must not escape its text rectangle.
     * Native image delivery covers only that box; stray cells stay visible. */
    send(TD_PYUI_FILL, 80, 0, 0, 0, 20, 4, 231, 17, ".");
    m.a = 4;
    m.b = 1;
    m.c = 8;
    m.d = 2;
    m.f = TD_PYUI_CLEAR;
    m.flags = 16;
    const char *overflow = "????????????????????\n????????????????????";
    m.len = (uint16_t)strlen(overflow);
    CHECK(td_pyui_send(&m, overflow));
    settle();
    static td_buffer_t font_back;
    td_buffer_init(&font_back, 80, 25);
    td_wm_compose(&font_back);
    td_rect_t client = td_win_client(td_win_focused());
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 20; x++)
            if (x < 4 || x >= 12 || y < 1 || y >= 3)
                CHECK_EQ(td_buffer_cell(&font_back, client.x + x, client.y + y)->ch, '.');
    send(TD_PYUI_CLOSE, 90, 0, 0, 0, 0, 0, 0, 0, NULL);
    send(TD_PYUI_CLOSE, 79, 0, 0, 0, 0, 0, 0, 0, NULL);
    settle();
    td_picture_set_sink(NULL, NULL);
    td_term_set_sixel(false);
}

static void test_pictures(void)
{
    static const uint8_t n9[9] = {21, 21, 21, 21, 196, 21, 21, 21, 21};
    send_image(30, 3, 3, n9, NULL);
    send(TD_PYUI_FILL, CANVAS, 0, 0, 0, 12, 4, -1, -1, " ");

    /* A terminal without pictures: the canvas's own pixels show it. */
    td_term_set_sixel(false);
    send_picture(30, 2, 1, 8, 2, "OK");
    settle();
    CHECK_EQ(cell(2, 1).ch, 0x2580);   /* pixels: the picture, made smaller */
    CHECK_EQ(cell(3, 2).fg, 196);      /* its middle is red (the label is at 5..6) */

    /* One with Sixel and 10x20 cells: retain the coloured fallback while
     * sending the picture, never expose a black placeholder. The image is
     * Sixel at that size (8 x 2 cells: 80 x 40 pixels). */
    td_term_set_sixel(true);
    td_term_set_cell_px(10, 20);
    s_out_len = 0;
    td_wm_invalidate();
    settle();
    CHECK_EQ(cell(2, 1).ch, 0x2580);
    CHECK_EQ(cell(3, 2).fg, 196);
    CHECK(strstr(s_out, "\x1bP0;1;0q\"1;1;80;40") != NULL);
    CHECK(strstr(s_out, "#196;2;100;0;0") != NULL);   /* the middle's red */
    CHECK(strstr(s_out, "\xe2\x96\x80") == NULL); /* no coarse image on first appearance */

    /* CrowPanel font 16 (100x30): the same cells use 8x16 pixels. */
    td_term_set_cell_px(8, 16);
    s_out_len = 0;
    settle();
    CHECK(strstr(s_out, "\x1bP0;1;0q\"1;1;64;32") != NULL);

    /* Nothing changed: not sent again. */
    s_out_len = 0;
    td_wm_invalidate();
    settle();
    CHECK(strstr(s_out, "\x1bP") == NULL);

    /* A pressed/relabelled image replaces an image at the same rectangle.
     * Only Sixel may be sent: no block/text fallback before the new DCS. */
    s_out_len = 0;
    send_picture(30, 2, 1, 8, 2, "DOWN");
    settle();
    CHECK(strstr(s_out, "\x1bP") != NULL);
    CHECK(strstr(s_out, "\xe2\x96\x80") == NULL);
    CHECK(strstr(s_out, "DOWN") == NULL);
    CHECK(strncmp(s_out, "\x1b[?2026h\x1b"
                         "7",
                  10) == 0);
    CHECK(strstr(s_out, "\x1b[?2026l") != NULL);

    /* A local LCD consumes the exact rendered pixels without a Sixel
     * encode/decode round trip; changing sinks invalidates retained IDs. */
    td_picture_set_sink(local_picture, NULL);
    s_out_len = 0;
    td_wm_invalidate();
    settle();
    CHECK_EQ(s_local_pictures, 1);
    CHECK(strstr(s_out, "\x1bP") == NULL);
    td_picture_set_sink(NULL, NULL);
    s_out_len = 0;
    td_wm_invalidate();
    settle();
    CHECK(strstr(s_out, "\x1bP") != NULL);

    /* Text over it removes it; the cells show again. */
    send(TD_PYUI_PUT, CANVAS, 0, 3, 1, 15, -1, 0, 0, "X");
    settle();
    CHECK_EQ(cell(3, 1).ch, 'X');
    CHECK_EQ(cell(2, 1).ch, 0x2580);   /* the rest is the pixels again */
    td_term_set_sixel(false);
}

/* A QR-like picture (black and white modules) shrunk by an uneven amount
 * into a terminal without pictures stays black and white: no grey edges,
 * in half blocks and in quadrants, and again after the detail changes. */
static bool black_or_white(int colour)
{
    return colour == 16 || colour == 231;
}
static void check_sharp(int x0, int y0, int w, int h)
{
    int black = 0, white = 0;
    for (int y = y0; y < y0 + h; y++)
        for (int x = x0; x < x0 + w; x++)
        {
            td_cell_t c = cell(x, y);
            CHECK(black_or_white(c.fg));
            CHECK(black_or_white(c.bg));
            black += (c.fg == 16) + (c.bg == 16);
            white += (c.fg == 231) + (c.bg == 231);
        }
    CHECK(black > 0 && white > 0);
}
static void test_sharp_pictures(void)
{
    enum
    {
        N = 21   /* a version 1 QR code's modules */
    };
    static uint8_t qr[N * N];
    uint32_t seed = 12345;
    for (int i = 0; i < N * N; i++)
    {
        seed = seed * 1103515245u + 12345u;
        qr[i] = (seed >> 16) & 1 ? 16 : 231;
    }
    send_image(32, N, N, qr, NULL);
    send(TD_PYUI_FILL, CANVAS, 0, 0, 0, 12, 4, -1, -1, " ");
    td_term_set_sixel(false);
    td_canvas_set_detail(TD_CANVAS_HALF);

    /* 21 modules into 9 x 4 cells: 18 x 8 pixels, neither a whole number. */
    td_pyui_picture_t part = {0, 0, 0, 15, 0};
    td_pyui_msg_t m = {0};
    m.op = TD_PYUI_PICTURE;
    m.id = CANVAS;
    m.win = 32;
    m.a = 1;
    m.b = 0;
    m.c = 9;
    m.d = 4;
    m.len = sizeof(part);
    CHECK(td_pyui_send(&m, (const char *)&part));
    settle();
    CHECK(!*error_event());
    check_sharp(1, 0, 9, 4);

    /* Quadrants: drawn again for four pixels to a cell, still sharp. */
    td_canvas_set_detail(TD_CANVAS_QUAD);
    td_wm_invalidate();
    settle();
    check_sharp(1, 0, 9, 4);
    td_canvas_set_detail(TD_CANVAS_HALF);
    td_wm_invalidate();
    settle();
    check_sharp(1, 0, 9, 4);
    send(TD_PYUI_CLOSE, 32, 0, 0, 0, 0, 0, 0, 0, NULL);
    settle();
}

static void test_picture_background(void)
{
    uint8_t px[64], mask[64] = {0};
    memset(px, 196, sizeof(px));
    mask[3 * 8 + 3] = 1; /* a hole too small to survive the block fallback */
    send_image(31, 8, 8, px, mask);
    send(TD_PYUI_PAGE, CANVAS, 0, 0, 0, 0, 0, 0, 0, NULL);
    send(TD_PYUI_FILL, CANVAS, 0, 0, 0, 12, 4, 15, 46, " ");
    send_picture(31, 1, 1, 8, 2, "");
    td_term_set_sixel(true);
    td_term_set_cell_px(10, 20);
    s_out_len = 0;
    settle();
    CHECK(strstr(s_out, "#46;2;0;100;0") != NULL); /* exact green under the hole */
    /* Repainting and copying keep the original background, not red fallback pixels. */
    send_picture(31, 1, 1, 8, 2, "");
    send(TD_PYUI_PAGE_COPY, CANVAS, 0, 0, 1, 0, 0, 0, 0, NULL);
    send(TD_PYUI_PAGE, CANVAS, 0, 1, 1, 0, 0, 0, 0, NULL);
    s_out_len = 0;
    settle();
    CHECK(strstr(s_out, "#46;2;0;100;0") != NULL);
    send(TD_PYUI_FILL, CANVAS, 0, 0, 0, 12, 4, 15, 21, " ");
    send(TD_PYUI_PAGE, CANVAS, 0, 0, 0, 0, 0, 0, 0, NULL);
    s_out_len = 0;
    settle();
    CHECK(strstr(s_out, "#46;2;0;100;0") != NULL); /* freeing one copy keeps the other */
    td_term_set_sixel(false);
}

static void test_canvas_release(void)
{
    td_pyui_msg_t msg;
    char text[TD_PYUI_TEXT_MAX + 1];
    while (td_pyui_recv(&msg, text, sizeof(text)))
    {
    }
    td_event_t ev = {.type = TD_EV_MOUSE, .x = CX + 1, .y = CY + 1, .button = 0, .action = TD_MOUSE_PRESS, .time_ms = s_now};
    td_wm_dispatch(&ev);
    CHECK(td_pyui_recv(&msg, text, sizeof(text)));
    CHECK_EQ(msg.op, TD_PYUI_EV_MOUSE);
    CHECK_EQ(msg.id, CANVAS);
    CHECK_EQ(msg.d, TD_MOUSE_PRESS);
    ev.x = CX + 20; /* outside the canvas: the original canvas still gets release */
    ev.action = TD_MOUSE_RELEASE;
    td_wm_dispatch(&ev);
    CHECK(td_pyui_recv(&msg, text, sizeof(text)));
    CHECK_EQ(msg.id, CANVAS);
    CHECK_EQ(msg.a, 20);
    CHECK_EQ(msg.d, TD_MOUSE_RELEASE);
}

/* A producer paused between erase and redraw exposes the empty bar. This
 * reproduces the HMI update crossing desktop ticks without thread timing. */
static void test_unbatched_frames(void)
{
    send(TD_PYUI_RECT, CANVAS, 0, 0, 0, 20, 4, 40, 0, NULL);
    settle();
    CHECK_EQ(s_terminal.cells[CY * TD_VT_MAX_COLS + CX].bg, 40);
    send(TD_PYUI_FILL, CANVAS, 0, 0, 0, 10, 2, -1, TD_PYUI_CLEAR, " ");
    send(TD_PYUI_RECT, CANVAS, 0, 0, 0, 20, 4, 17, 0, NULL);
    settle();
    CHECK_EQ(s_terminal.cells[CY * TD_VT_MAX_COLS + CX].bg, 17);
    send(TD_PYUI_RECT, CANVAS, 0, 0, 0, 20, 4, 40, 0, NULL);
    settle();
    CHECK_EQ(s_terminal.cells[CY * TD_VT_MAX_COLS + CX].bg, 40);
}

static void test_batch_frames(void)
{
    /* The actual terminal, not just a manually composed buffer, keeps
     * the old bar through multiple ticks and queue refills. */
    send(TD_PYUI_BATCH_BEGIN, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    send(TD_PYUI_RECT, CANVAS, 0, 0, 0, 20, 4, 17, 0, NULL);
    settle();
    CHECK_EQ(cell(0, 0).bg, 17);   /* working state changed */
    CHECK_EQ(s_terminal.cells[CY * TD_VT_MAX_COLS + CX].bg, 40);
    uint32_t frames = td_stats()->frames;
    send(TD_PYUI_BATCH_BEGIN, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    for (int chunk = 0; chunk < 3; chunk++)
    {
        for (int i = 0; i < 100; i++)
            send(TD_PYUI_RECT, CANVAS, 0, 0, 0, 20, 4, 17, 0, NULL);
        settle();
        CHECK_EQ(td_stats()->frames, frames);
    }
    send(TD_PYUI_BATCH_END, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    settle();
    CHECK_EQ(td_stats()->frames, frames);
    /* Force a full repaint while held too. */
    td_full_redraw();
    settle();
    CHECK_EQ(td_stats()->frames, frames);
    send(TD_PYUI_RECT, CANVAS, 0, 0, 0, 20, 4, 46, 0, NULL);
    send(TD_PYUI_BATCH_END, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    settle();
    CHECK(td_stats()->frames > frames);
    CHECK_EQ(s_terminal.cells[CY * TD_VT_MAX_COLS + CX].bg, 46);

    /* All messages are already queued, but the tick budget ends between
     * erase and redraw. Exactly the same hold must apply. */
    send(TD_PYUI_BATCH_BEGIN, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    for (int i = 0; i < 198; i++)
        send(TD_PYUI_CLOSE, 99, 0, 0, 0, 0, 0, 0, 0, NULL);
    send(TD_PYUI_RECT, CANVAS, 0, 0, 0, 20, 4, 17, 0, NULL);
    send(TD_PYUI_RECT, CANVAS, 0, 0, 0, 20, 4, 46, 0, NULL);
    send(TD_PYUI_BATCH_END, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    frames = td_stats()->frames;
    s_now += 20;
    td_step();
    CHECK_EQ(cell(0, 0).bg, 17);
    CHECK_EQ(td_stats()->frames, frames);
    CHECK_EQ(s_terminal.cells[CY * TD_VT_MAX_COLS + CX].bg, 46);
    settle();
    CHECK_EQ(s_terminal.cells[CY * TD_VT_MAX_COLS + CX].bg, 46);

    /* A missing end releases after the absolute timeout, not a timeout
     * extended by each nested begin. Late ends are harmless. */
    send(TD_PYUI_BATCH_BEGIN, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    send(TD_PYUI_RECT, CANVAS, 0, 0, 0, 20, 4, 21, 0, NULL);
    settle();
    s_now += 1100;
    settle();
    CHECK_EQ(s_terminal.cells[CY * TD_VT_MAX_COLS + CX].bg, 21);
    CHECK(strstr(error_event(), "batch exceeded") != NULL);
    send(TD_PYUI_BATCH_END, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    send(TD_PYUI_BATCH_END, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    settle();
}

static void test_bar(void)
{
    /* Full, shrinking and empty bars each reach the wire complete. The
     * top edge is optional and black (zero bytes) is a valid colour. */
    td_pyui_msg_t m = {.op = TD_PYUI_BAR, .id = CANVAS, .a = 0, .b = 0, .c = 20, .d = 4, .e = 1000, .f = 1, .len = 3, .flags = 1};
    const char colors[3] = {40, 0, 120};
    CHECK(td_pyui_send(&m, colors));
    settle();
    CHECK_EQ(cell(4, 0).fg, 120);
    CHECK_EQ(cell(4, 0).bg, 40);
    m.e = 500;
    CHECK(td_pyui_send(&m, colors));
    settle();
    CHECK_EQ(s_terminal.cells[(CY + 1) * TD_VT_MAX_COLS + CX + 4].bg, 40);
    CHECK_EQ(s_terminal.cells[(CY + 1) * TD_VT_MAX_COLS + CX + 5].bg, 0);
    m.e = -100;
    CHECK(td_pyui_send(&m, colors));
    settle();
    CHECK_EQ(cell(4, 0).bg, 0);
    CHECK_EQ(cell(4, 0).fg, 0);
    /* Normal pictures: identical left/right column pairs at rounded ends. */
    m.e = 1200;
    m.d = 8;
    m.f = 4;
    m.flags = 0;
    send(TD_PYUI_RECT, CANVAS, 0, 0, 0, 24, 8, 17, 0, NULL);
    CHECK(td_pyui_send(&m, colors));
    settle();
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 5; x++)
        {
            CHECK_EQ(cell(x, y).fg, cell(9 - x, y).fg);
            CHECK_EQ(cell(x, y).bg, cell(9 - x, y).bg);
        }
    /* Odd coordinates and off-screen rectangles remain clipped. */
    m.a = -3;
    m.b = -1;
    CHECK(td_pyui_send(&m, colors));
    settle();
    CHECK(!*error_event());
}

static void test_gradient_fills(void)
{
    const uint8_t vertical[7] = {255, 0, 0, 0, 0, 255, 0};
    td_pyui_msg_t m = {.op = TD_PYUI_RECT, .id = CANVAS, .flags = 2, .a = 0, .b = 0, .c = 20, .d = 4, .len = 7};
    CHECK(td_pyui_send(&m, (const char *)vertical));
    settle();
    CHECK_EQ(cell(3, 0).fg, td_rgb256(255, 0, 0));
    CHECK_EQ(cell(3, 1).bg, td_rgb256(0, 0, 255));
    CHECK(cell(3, 0).fg != cell(3, 1).bg);
    /* Rounded fills retain their underlying corners and symmetric ends. */
    send(TD_PYUI_RECT, CANVAS, 0, 0, 0, 20, 8, 17, 0, NULL);
    m.d = 8;
    m.f = 4;
    CHECK(td_pyui_send(&m, (const char *)vertical));
    settle();
    CHECK_EQ(cell(0, 0).fg, 17);
    for (int y = 0; y < 4; y++)
        for (int x = 0; x < 5; x++)
        {
            CHECK_EQ(cell(x, y).fg, cell(9 - x, y).fg);
            CHECK_EQ(cell(x, y).bg, cell(9 - x, y).bg);
        }
    /* A shrinking gradient bar clears old fill with its solid track. */
    const uint8_t bar[10] = {40, 17, 120, 255, 0, 0, 0, 0, 255, 0};
    m.op = TD_PYUI_BAR;
    m.flags = 0;
    m.e = 1000;
    m.f = 0;
    m.len = 10;
    CHECK(td_pyui_send(&m, (const char *)bar));
    settle();
    CHECK_EQ(cell(3, 0).fg, td_rgb256(255, 0, 0));
    m.e = 0;
    CHECK(td_pyui_send(&m, (const char *)bar));
    settle();
    CHECK_EQ(cell(3, 0).fg, 17);
    m.op = TD_PYUI_CIRCLE;
    m.flags = 2;
    m.a = 6;
    m.b = 4;
    m.c = 3;
    m.len = 7;
    CHECK(td_pyui_send(&m, (const char *)vertical));
    settle();
    CHECK(!*error_event());
}

static uint16_t rgb_frame[800 * 500];
static int rgb_max_height;
static bool rgb_capture(void *user, int x, int y, const uint16_t *px, int w, int h)
{
    (void)user;
    if (h > rgb_max_height)
        rgb_max_height = h;
    for (int row = 0; row < h; row++)
        memcpy(rgb_frame + (y * 20 + row) * 800 + x * 10, px + (size_t)row * w, (size_t)w * sizeof(*px));
    return true;
}
static int rejected_run_calls;
static bool reject_runs(void *user, int x, int y, const uint16_t *runs, size_t words, int w, int h)
{
    (void)user;
    (void)x;
    (void)y;
    (void)runs;
    (void)words;
    (void)w;
    (void)h;
    rejected_run_calls++;
    return false;
}

static void test_rgb_pictures(void)
{
    CHECK(td_pyui_attach());
    CHECK(!td_pyui_rgb_enabled());
    if (TD_PYUI_MAX_CELLS <= 2000)
    {
        CHECK(!td_pyui_enable_rgb());
        td_pyui_detach_begin();
        for (int i = 0; i < 20 && !td_pyui_detach_done(); i++)
            settle();
        return;
    }
    CHECK(td_pyui_enable_rgb());
    s_terminal.sixel = true;
    s_terminal.cell_w = 10;
    s_terminal.cell_h = 20;
    send(TD_PYUI_OPEN, WIN, 0, 30, 10, WIN_X, WIN_Y, 0, 0, "RGB");
    send(TD_PYUI_ADD, CANVAS, TD_PYUI_W_CANVAS, 0, 0, 12, 4, 2, 0, NULL);
    uint8_t colors[7] = {13, 45, 77, 17, 60, 98, 0};
    td_pyui_msg_t m = {.op = TD_PYUI_RECT, .id = CANVAS, .flags = 2, .c = 24, .d = 8, .len = 7};
    CHECK(td_pyui_send(&m, (const char *)colors));
    td_term_set_sixel(true);
    td_term_set_cell_px(10, 20);
    td_picture_set_rgb_sink(rgb_capture, NULL);
    settle();
    CHECK(rgb_max_height > 0 && rgb_max_height <= 40); /* bounded regions, never an 80-pixel canvas allocation */
    CHECK_EQ(rgb_frame[CY * 20 * 800 + CX * 10], td_rgb565(13, 45, 77));
    /* A dark-blue image must not turn gray; its keyed corner must keep the
     * original RGB gradient beneath, including after text/pressed redraws. */
    td_pyui_image_t *img = td_pyui_image_new_rgb(6, 6, true, true);
    CHECK(img != NULL);
    for (int i = 0; i < 36; i++)
    {
        img->px[i] = td_rgb256(13, 45, 77);
        img->rgb[i] = td_rgb565(13, 45, 77);
        img->mask[i] = i == 0;
    }
    img->rgb[13] = td_rgb565(0, 100, 170);
    CHECK(td_pyui_pixels_take(td_pyui_image_bytes(img)));
    td_pyui_send_image(TD_PYUI_IMAGE, 30, 0, 0, img, NULL);
    send_picture(30, 2, 1, 4, 2, "");
    settle();
    uint16_t corner = rgb_frame[(CY + 1) * 20 * 800 + (CX + 2) * 10];
    CHECK_EQ(rgb_frame[((CY + 2) * 20 + 19) * 800 + (CX + 5) * 10 + 9], td_rgb565(13, 45, 77));
    send_picture(30, 2, 1, 4, 2, "ON");
    settle();
    CHECK_EQ(rgb_frame[(CY + 1) * 20 * 800 + (CX + 2) * 10], corner);
    send_picture(30, 2, 1, 4, 2, "");
    settle();
    uint16_t detail = rgb_frame[((CY + 1) * 20 + 12) * 800 + (CX + 2) * 10 + 3];
    CHECK_EQ(detail, td_rgb565(0, 100, 170));
    send_picture(30, 5, 1, 2, 1, "");
    settle();
    CHECK_EQ(rgb_frame[((CY + 1) * 20 + 12) * 800 + (CX + 2) * 10 + 3], detail);
    /* Showing an unrelated page must erase the earlier image everywhere,
     * without a capture forcing a full redraw. */
    send(TD_PYUI_PAGE, CANVAS, 0, 1, -1, 0, 0, 0, 0, NULL);
    send(TD_PYUI_FILL, CANVAS, 0, 0, 0, 12, 4, 231, 18, " ");
    send(TD_PYUI_PAGE, CANVAS, 0, 1, 1, 0, 0, 0, 0, NULL);
    settle();
    CHECK_EQ(rgb_frame[((CY + 1) * 20 + 12) * 800 + (CX + 2) * 10 + 3], td_palette_rgb565(18));
    send(TD_PYUI_PAGE, CANVAS, 0, 0, 0, 0, 0, 0, 0, NULL);
    settle();
    CHECK_EQ(rgb_frame[((CY + 1) * 20 + 12) * 800 + (CX + 2) * 10 + 3], detail);
    send(TD_PYUI_PAGE_COPY, CANVAS, 0, 0, 1, 0, 0, 0, 0, NULL);
    send(TD_PYUI_PAGE, CANVAS, 0, 1, 1, 0, 0, 0, 0, NULL);
    settle();
    CHECK_EQ(rgb_frame[(CY + 1) * 20 * 800 + (CX + 2) * 10], corner);
    send(TD_PYUI_RESIZE, CANVAS, 0, 14, 5, 0, 0, 0, 0, NULL);
    settle();
    CHECK(!*error_event());
    td_picture_set_rgb_sink(NULL, NULL);
    s_out_len = 0;
    td_pictures_forget();
    td_win_invalidate(NULL);
    settle();
    CHECK(strstr(s_out, ";2;3;17;29") != NULL); /* 13/45/77 preserved as RGB565 then Sixel percentages */
    /* A changed source image must replace its already-cached appearance,
     * including pictures on copied/hidden pages. */
    /* Resizing deliberately removed picture objects; recreate a cached
     * reference before changing the source image. */
    td_picture_set_rgb_sink(rgb_capture, NULL);
    send_picture(30, 2, 1, 4, 2, "");
    settle();
    td_pictures_forget();
    td_win_invalidate(NULL);
    settle();
    uint32_t cache[12];
    td_pyui_cache_stats(cache);
    CHECK(cache[2] > 0);
    const uint8_t changed[7] = {27, 147, 86, 27, 147, 86, 0};
    m = (td_pyui_msg_t){.op = TD_PYUI_RECT, .id = 30, .flags = 2, .c = 6, .d = 6, .len = 7};
    CHECK(td_pyui_send(&m, (const char *)changed));
    settle();
    CHECK_EQ(img->rgb[13], td_rgb565(27, 147, 86));
    CHECK(!*error_event());
    CHECK_EQ(rgb_frame[((CY + 1) * 20 + 12) * 800 + (CX + 2) * 10 + 3], td_rgb565(27, 147, 86));
    td_picture_set_runs_sink(reject_runs, NULL);
    td_win_invalidate(NULL);
    settle();
    CHECK(rejected_run_calls > 0);
    CHECK_EQ(rgb_frame[((CY + 1) * 20 + 12) * 800 + (CX + 2) * 10 + 3], td_rgb565(27, 147, 86));
    td_picture_set_runs_sink(NULL, NULL);
    td_pyui_detach_begin();
    for (int i = 0; i < 20 && !td_pyui_detach_done(); i++)
        settle();
    CHECK(td_pyui_pixels_take(TD_PYUI_MAX_PIXELS));
    td_pyui_pixels_give(TD_PYUI_MAX_PIXELS);
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
    td_vterm_init(&s_terminal, 80, 25);
    s_terminal.reply = terminal_reply;
    td_init(&s_hal);
    td_pyui_start();
    CHECK(td_pyui_attach());
    send(TD_PYUI_OPEN, WIN, 0, 30, 10, WIN_X, WIN_Y, 0, 0, "Pages");
    settle();

    test_pages();
    test_big();
    test_pixels();
    test_pictures();
    test_sharp_pictures();
    test_picture_background();
    test_generated_fonts();
    test_canvas_release();
    test_unbatched_frames();
    test_batch_frames();
    test_bar();
    test_gradient_fills();
    test_limits();

    send(TD_PYUI_BATCH_BEGIN, 0, 0, 0, 0, 0, 0, 0, 0, NULL);
    settle();
    uint32_t frames = td_stats()->frames;
    td_pyui_detach_begin();
    for (int i = 0; i < 20 && !td_pyui_detach_done(); i++)
        settle();
    CHECK(td_stats()->frames > frames);   /* script death releases its hold */
    /* Every image and pixel layer gave its bytes back. */
    CHECK(td_pyui_pixels_take(TD_PYUI_MAX_PIXELS));
    td_pyui_pixels_give(TD_PYUI_MAX_PIXELS);
    test_rgb_pictures();
    return TD_TEST_RESULT();
}
