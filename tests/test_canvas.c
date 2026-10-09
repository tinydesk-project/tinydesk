/*
 * test_canvas.c - pixel canvases (td_canvas.h): drawing, clipping, the
 * half-block cells they become, and colour matching.
 */
#include <stdlib.h>

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

int main(void)
{
    test_new_resize();
    test_allocator();
    test_drawing();
    test_cells();
    test_colours();
    return TD_TEST_RESULT();
}
