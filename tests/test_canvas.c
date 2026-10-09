/*
 * test_canvas.c - pixel canvases (td_canvas.h): drawing, clipping, the
 * half-block cells they become, and colour matching.
 */
#include <stdlib.h>
#include <string.h>

#include "td_test.h"
#include "tinydesk/td.h"

static td_buffer_t s_buf;

static void test_new_resize(void)
{
    CHECK(td_canvas_new(0, 4) == NULL);
    CHECK(td_canvas_new(4, -1) == NULL);
    td_canvas_t *c = td_canvas_new(5, 3);
    CHECK(c != NULL);
    CHECK_EQ(c->w, 5);
    CHECK_EQ(c->h, 3);
    CHECK_EQ(td_canvas_get(c, 4, 2), 0); /* starts black */
    td_canvas_pixel(c, 1, 1, 9);
    CHECK(td_canvas_resize(c, 7, 4));
    CHECK_EQ(c->w, 7);
    CHECK_EQ(c->h, 4);
    CHECK_EQ(td_canvas_get(c, 1, 1), 0); /* cleared */
    CHECK(!td_canvas_resize(c, 0, 4));
    CHECK_EQ(c->w, 7); /* kept */
    td_canvas_free(c);
    td_canvas_free(NULL);
}

/* A port's allocator (PSRAM on ESP32 boards) gets every pixel buffer. */
static int s_allocs, s_frees;
static size_t s_last_bytes;
static void *count_alloc(size_t bytes)
{
    s_allocs++;
    s_last_bytes = bytes;
    void *p = malloc(bytes);
    if (p)
        memset(p, 0xAA, bytes); /* not zero: the canvas must clear it */
    return p;
}
static void count_free(void *p)
{
    if (p)
        s_frees++;
    free(p);
}

static void test_allocator(void)
{
    td_canvas_set_allocator(count_alloc, count_free);
    td_canvas_t *c = td_canvas_new(6, 4);
    CHECK_EQ(s_allocs, 1);
    CHECK_EQ(s_last_bytes, 24);
    CHECK_EQ(td_canvas_get(c, 5, 3), 0); /* cleared */
    CHECK(td_canvas_resize(c, 10, 2));
    CHECK_EQ(s_allocs, 2);
    CHECK_EQ(s_frees, 1);
    CHECK_EQ(s_last_bytes, 20);
    CHECK_EQ(td_canvas_get(c, 9, 1), 0);
    td_canvas_free(c);
    CHECK_EQ(s_frees, 2);
    td_canvas_set_allocator(NULL, NULL); /* back to malloc/free */
    c = td_canvas_new(2, 2);
    CHECK_EQ(s_allocs, 2);
    td_canvas_free(c);
    CHECK_EQ(s_frees, 2);
}

static void test_drawing(void)
{
    td_canvas_t *c = td_canvas_new(8, 6);
    td_canvas_pixel(c, -1, 0, 5); /* outside: ignored */
    td_canvas_pixel(c, 8, 0, 5);
    td_canvas_pixel(c, 7, 5, 5);
    CHECK_EQ(td_canvas_get(c, 7, 5), 5);
    CHECK_EQ(td_canvas_get(c, 8, 5), 0);

    td_canvas_clear(c, 1);
    td_canvas_fill_rect(c, 6, 4, 10, 10, 2); /* clipped at the corner */
    CHECK_EQ(td_canvas_get(c, 5, 4), 1);
    CHECK_EQ(td_canvas_get(c, 6, 4), 2);
    CHECK_EQ(td_canvas_get(c, 7, 5), 2);
    td_canvas_fill_rect(c, -3, -3, 4, 4, 3);
    CHECK_EQ(td_canvas_get(c, 0, 0), 3);
    CHECK_EQ(td_canvas_get(c, 1, 0), 1);

    td_canvas_clear(c, 0);
    td_canvas_line(c, 0, 0, 5, 5, 4); /* diagonal, both ends */
    for (int i = 0; i <= 5; ++i)
        CHECK_EQ(td_canvas_get(c, i, i), 4);
    CHECK_EQ(td_canvas_get(c, 1, 0), 0);
    td_canvas_line(c, 7, 2, -20, 2, 6); /* runs off the left edge */
    CHECK_EQ(td_canvas_get(c, 0, 2), 6);
    CHECK_EQ(td_canvas_get(c, 7, 2), 6);

    static const uint8_t src[2][3] = {{10, 11, 12}, {13, 14, 15}};
    td_canvas_clear(c, 0);
    td_canvas_blit(c, -1, 5, &src[0][0], 3, 2, 3); /* clipped left and below */
    CHECK_EQ(td_canvas_get(c, 0, 5), 11);
    CHECK_EQ(td_canvas_get(c, 1, 5), 12);
    CHECK_EQ(td_canvas_get(c, 2, 5), 0);
    td_canvas_blit(c, 6, 0, &src[0][0], 3, 2, 3); /* clipped right */
    CHECK_EQ(td_canvas_get(c, 7, 0), 11);
    CHECK_EQ(td_canvas_get(c, 7, 1), 14);
    td_canvas_free(c);
}

static void test_cells(void)
{
    /* 3 x 3 pixels: two cell rows, the second with nothing below. */
    td_canvas_t *c = td_canvas_new(3, 3);
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x)
            td_canvas_pixel(c, x, y, (uint8_t)(100 + y * 10 + x));
    td_buffer_init(&s_buf, 10, 5);
    td_draw_target(&s_buf);
    td_canvas_draw(c, 2, 1);
    td_cell_t *cell = td_buffer_cell(&s_buf, 2, 1);
    CHECK_EQ(cell->ch, 0x2580);
    CHECK_EQ(cell->fg, 100); /* upper pixel */
    CHECK_EQ(cell->bg, 110); /* lower pixel */
    cell = td_buffer_cell(&s_buf, 4, 1);
    CHECK_EQ(cell->fg, 102);
    CHECK_EQ(cell->bg, 112);
    cell = td_buffer_cell(&s_buf, 3, 2);
    CHECK_EQ(cell->fg, 121);
    CHECK_EQ(cell->bg, 0); /* odd height: black below */
    CHECK_EQ(td_buffer_cell(&s_buf, 2, 3)->ch, ' '); /* only two rows */
    CHECK_EQ(td_buffer_cell(&s_buf, 5, 1)->ch, ' '); /* only three columns */

    /* Drawn through the origin and clipped like any other drawing. */
    td_buffer_init(&s_buf, 10, 5);
    td_draw_target(&s_buf);
    td_draw_origin(5, 0);
    td_draw_clip(td_rect(5, 0, 2, 1));
    td_canvas_draw(c, 0, 0);
    CHECK_EQ(td_buffer_cell(&s_buf, 5, 0)->ch, 0x2580);
    CHECK_EQ(td_buffer_cell(&s_buf, 6, 0)->fg, 101);
    CHECK_EQ(td_buffer_cell(&s_buf, 7, 0)->ch, ' '); /* clipped */
    CHECK_EQ(td_buffer_cell(&s_buf, 5, 1)->ch, ' ');
    td_canvas_free(c);
}

/* The four pixel colours a quadrant cell shows. */
static void quad_pixels(uint32_t ch, uint8_t fg, uint8_t bg, uint8_t out[4])
{
    static const uint32_t glyph[16] = {' ', 0x2597, 0x2596, 0x2584, 0x259D, 0x2590, 0x259E, 0x259F,
                                       0x2598, 0x259A, 0x258C, 0x2599, 0x2580, 0x259C, 0x259B, 0x2588};
    int mask = -1;
    for (int m = 0; m < 16; ++m)
        if (glyph[m] == ch)
            mask = m;
    for (int p = 0; p < 4; ++p)
        out[p] = mask >= 0 && (mask & (8 >> p)) ? fg : bg;
}

static bool quad_shows(const uint8_t px[4], const uint8_t want[4])
{
    uint32_t ch;
    uint8_t fg, bg, got[4];
    td_canvas_quad_cell(px, &ch, &fg, &bg);
    quad_pixels(ch, fg, bg, got);
    return memcmp(got, want, 4) == 0;
}

static void test_quad(void)
{
    /* One colour: a blank cell in it. */
    const uint8_t one[4] = {5, 5, 5, 5};
    uint32_t ch;
    uint8_t fg, bg;
    td_canvas_quad_cell(one, &ch, &fg, &bg);
    CHECK_EQ(ch, ' ');
    CHECK_EQ(bg, 5);

    /* Two colours: every one of the 16 patterns exactly, and only the
     * eight characters with the upper left quarter empty. */
    bool exact = true, eight = true;
    for (int m = 0; m < 16; ++m)
    {
        uint8_t px[4];
        for (int p = 0; p < 4; ++p)
            px[p] = (m & (8 >> p)) ? 196 : 21;
        exact = exact && quad_shows(px, px);
        td_canvas_quad_cell(px, &ch, &fg, &bg);
        eight = eight && (ch == ' ' || ch == 0x2597 || ch == 0x2596 || ch == 0x2584 || ch == 0x259D ||
                          ch == 0x2590 || ch == 0x259E || ch == 0x259F);
    }
    CHECK(exact);
    CHECK(eight);

    /* Three colours: the two reds are close, so the cell keeps red and
     * blue; the darker red shows as red. */
    const uint8_t three[4] = {196, 160, 21, 21};
    const uint8_t shown[4] = {196, 196, 21, 21};
    const uint8_t shown2[4] = {160, 160, 21, 21};
    CHECK(quad_shows(three, shown) || quad_shows(three, shown2));
    /* Four colours: black, white and two near-whites: black stays. */
    const uint8_t four[4] = {16, 231, 255, 254};
    td_canvas_quad_cell(four, &ch, &fg, &bg);
    CHECK(fg == 16 || bg == 16);

    /* The size that fills cells, and the drawing. */
    int w, h;
    td_canvas_fit(10, 5, &w, &h);
    CHECK(w == 10 && h == 10);
    td_canvas_set_detail(TD_CANVAS_QUAD);
    CHECK_EQ(td_canvas_detail(), TD_CANVAS_QUAD);
    td_canvas_fit(10, 5, &w, &h);
    CHECK(w == 20 && h == 10);
    td_set_ascii_mode(true); /* quadrants need UTF-8 */
    td_canvas_fit(10, 5, &w, &h);
    CHECK(w == 10 && h == 10);
    td_set_ascii_mode(false);

    /* 3 x 3 pixels: 2 x 2 cells; the missing right and lower pixels are 0. */
    td_canvas_t *c = td_canvas_new(3, 3);
    td_canvas_clear(c, 21);
    td_canvas_pixel(c, 1, 0, 196);
    td_canvas_pixel(c, 2, 2, 196);
    td_buffer_init(&s_buf, 10, 5);
    td_draw_target(&s_buf);
    td_draw_origin(0, 0);
    td_draw_clip(td_rect(0, 0, 10, 5));
    td_canvas_draw(c, 1, 1);
    uint8_t got[4];
    td_cell_t *cell = td_buffer_cell(&s_buf, 1, 1);
    quad_pixels(cell->ch, (uint8_t)cell->fg, (uint8_t)cell->bg, got);
    CHECK(got[0] == 21 && got[1] == 196 && got[2] == 21 && got[3] == 21);
    cell = td_buffer_cell(&s_buf, 2, 2);
    quad_pixels(cell->ch, (uint8_t)cell->fg, (uint8_t)cell->bg, got);
    CHECK(got[0] == 196 && got[1] == 0 && got[2] == 0 && got[3] == 0);
    CHECK_EQ(td_buffer_cell(&s_buf, 3, 1)->ch, ' '); /* only two columns */
    td_canvas_free(c);
    td_canvas_set_detail(TD_CANVAS_HALF);
}

static void test_colours(void)
{
    CHECK_EQ(td_rgb256(0, 0, 0), 16);
    CHECK_EQ(td_rgb256(255, 255, 255), 231);
    CHECK_EQ(td_rgb256(255, 0, 0), 196);
    CHECK_EQ(td_rgb256(0, 255, 0), 46);
    CHECK_EQ(td_rgb256(0, 0, 255), 21);
    CHECK_EQ(td_rgb256(95, 135, 175), 67);     /* exact cube colour */
    CHECK_EQ(td_rgb256(128, 128, 128), 244);   /* grey ramp: 8 + 10 * 12 */
    CHECK_EQ(td_rgb256(100, 100, 100), 241);   /* grey ramp 98 is closer than the cube's 95 */
    CHECK_EQ(td_rgb256(90, 140, 170), 67);     /* near a cube colour */
}

/* The red, green and blue of a palette index 16-255. */
static void palette_rgb(int i, int rgb[3])
{
    static const int level[6] = {0, 95, 135, 175, 215, 255};
    if (i >= 232)
    {
        rgb[0] = rgb[1] = rgb[2] = 8 + 10 * (i - 232);
        return;
    }
    i -= 16;
    rgb[0] = level[i / 36];
    rgb[1] = level[i / 6 % 6];
    rgb[2] = level[i % 6];
}

/* The mean of one channel over a 4 x 4 patch of a dithered colour. */
static int patch_mean(int r, int g, int b, int c)
{
    int sum = 0;
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
        {
            int rgb[3];
            palette_rgb(td_rgb256_dither((uint8_t)r, (uint8_t)g, (uint8_t)b, x, y), rgb);
            sum += rgb[c];
        }
    return sum / 16;
}

static bool same_everywhere(int r, int g, int b, int want)
{
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            if (td_rgb256_dither((uint8_t)r, (uint8_t)g, (uint8_t)b, x, y) != want)
                return false;
    return true;
}

static void test_dither(void)
{
    /* Palette colours are never mixed: no speckles on white, black or a
     * pure cube colour or grey. */
    CHECK(same_everywhere(255, 255, 255, 231));
    CHECK(same_everywhere(0, 0, 0, 16));
    CHECK(same_everywhere(255, 0, 0, 196));
    CHECK(same_everywhere(0, 0, 255, 21));
    CHECK(same_everywhere(95, 135, 175, 67));
    CHECK(same_everywhere(128, 128, 128, 244)); /* a ramp grey */
    CHECK(same_everywhere(135, 135, 135, 102)); /* a cube grey */

    /* A patch averages to the colour: within a few levels for every value
     * of one channel, in colour and in grey. */
    int worst_colour = 0, worst_grey = 0;
    for (int v = 0; v < 256; ++v)
    {
        int e = abs(patch_mean(v, 0, 255, 0) - v);
        worst_colour = e > worst_colour ? e : worst_colour;
        e = abs(patch_mean(v, v, v, 1) - v);
        worst_grey = e > worst_grey ? e : worst_grey;
    }
    CHECK(worst_colour <= 6); /* the cube's gaps are 40-95 */
    CHECK(worst_grey <= 2);   /* the greys are about 10 apart */

    /* Halfway between two levels: half the pixels each. */
    int upper = 0;
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            upper += td_rgb256_dither(235, 0, 0, x, y) == 196;
    CHECK_EQ(upper, 8);
    CHECK(td_rgb256_dither(235, 0, 0, -1, -7) == td_rgb256_dither(235, 0, 0, 3, 1)); /* negative x, y */
}

int main(void)
{
    test_new_resize();
    test_allocator();
    test_drawing();
    test_cells();
    test_colours();
    test_dither();
    test_quad();
    return TD_TEST_RESULT();
}
