/* Exercise the real port painter with explicit RGB scanout boundaries. */
#define TD_DISPLAY_TEST
#define TD_CROWPANEL_RENDER_CORE 1
#include "../ports/crowpanel7/main/display.c"
#include "td_test.h"

const uint8_t *font8x16_glyph(uint16_t cp)
{
    (void)cp;
    return NULL;
}
const uint8_t *font10x20_glyph(uint16_t cp)
{
    (void)cp;
    return NULL;
}
const uint8_t *font12x24_glyph(uint16_t cp)
{
    (void)cp;
    return NULL;
}
const uint8_t *font16x32_glyph(uint16_t cp)
{
    (void)cp;
    return NULL;
}

static uint16_t visible_pixel(int x, int y)
{
    return test_visible[(size_t)y * DISPLAY_W + x];
}
static void complete_frame(void)
{
    CHECK(test_requested != NULL);
    test_visible = test_requested;
    test_requested = NULL;
    CHECK(test_frame_callback != NULL);
    test_frame_callback(s_panel, NULL, NULL);
    paint(); /* consume acknowledgement before changing the next buffer */
}
static void background(int col, int row, int color)
{
    s_vt->cells[row * TD_VT_MAX_COLS + col] = (td_vcell_t){' ', 231, (uint8_t)color};
    s_vt->dirty = true;
}

int main(void)
{
    CHECK(display_init(NULL));
    CHECK(test_painter_core == TD_CROWPANEL_RENDER_CORE);
     /* A missed scanout preparation must be observable independently of
     * application drawing. Startup alignment is deliberately ignored. */
    uint32_t telemetry[8];
    for (int i = 0; i < 4; i++)
        scanout_vsync(s_panel, NULL, NULL);
    display_scanout_info(telemetry);
    CHECK(telemetry[0] == 4 && telemetry[2] == 1);
    CHECK(telemetry[5] == 24000000 && telemetry[7] == 20);
    test_ticks = 100;
    frame_complete(s_panel, NULL, NULL);
    test_ticks = 122;
    frame_complete(s_panel, NULL, NULL);
    scanout_vsync(s_panel, NULL, NULL);
    display_scanout_info(telemetry);
    CHECK(telemetry[1] == 2 && telemetry[2] == 1 && telemetry[3] == 22000);
    s_vt->cursor_visible = false;
    paint();
    CHECK(test_present_count == 1);
    CHECK(test_visible == test_buffers[0]);
    background(0, 0, 18);
    unsigned before = test_present_count;
    paint();
    paint();
    paint(); /* missing boundary must not release a buffer */
    CHECK(test_present_count == before);
    CHECK(visible_pixel(0, 0) == 0);
    complete_frame(); /* old black frame visible; blue submitted off-screen */
    CHECK(visible_pixel(0, 0) == 0);
    CHECK(test_present_count == before + 1);
    complete_frame();
    CHECK(visible_pixel(0, 0) == s_palette[18]);

      /* Each buffer must catch up with all cells, including unchanged values
     * last drawn into the other buffer. */
    for (int color = 20; color < 40; color++)
    {
        background(1, 0, color);
        uint16_t old = visible_pixel(s_cw, 0);
        paint();
        CHECK(visible_pixel(s_cw, 0) == old);
        complete_frame();
        CHECK(visible_pixel(0, 0) == s_palette[18]);
        CHECK(visible_pixel(s_cw, 0) == s_palette[color]);
    }

      /* Slot reuse can leave identical cell codes with different picture
     * pixels. Both framebuffer histories must track that generation. */
    uint8_t pixels[8 * 16];
    for (int color = 9; color <= 13; color++)
    {
        memset(pixels, color, sizeof(pixels));
        s_next_slot = 0; /* force a wrap onto exactly the same tile identity */
        CHECK(display_picture(NULL, 2, 1, pixels, 8, 16));
        uint16_t old = visible_pixel(2 * s_cw, s_ch);
        paint();
        CHECK(visible_pixel(2 * s_cw, s_ch) == old);
        complete_frame();
        CHECK(visible_pixel(2 * s_cw, s_ch) == s_palette[color]);
        CHECK(visible_pixel(0, 0) == s_palette[18]);
    }

      /* A fast value must not recycle a slot still used by another visible
     * field. Power pages mix frequently updated readings and static labels.
     * Exercise several allocator wraps through the actual LCD painter. */
    memset(pixels, 46, sizeof(pixels));
    CHECK(display_picture(NULL, 5, 1, pixels, 8, 16));
    paint();
    complete_frame();
    CHECK(visible_pixel(5 * s_cw, s_ch) == s_palette[46]);
    for (int update = 0; update < IMG_SLOTS * 3; update++)
    {
        int color = 9 + update % 5;
        memset(pixels, color, sizeof(pixels));
        CHECK(display_picture(NULL, 2, 1, pixels, 8, 16));
        paint();
        complete_frame();
        CHECK(visible_pixel(2 * s_cw, s_ch) == s_palette[color]);
        CHECK(visible_pixel(5 * s_cw, s_ch) == s_palette[46]);
    }
    memset(pixels, 13, sizeof(pixels));
    CHECK(display_picture(NULL, 2, 1, pixels, 8, 16));
    paint();
    complete_frame();

      /* Replacing part of an image must preserve the tiles outside it. */
    uint8_t wide[16 * 16];
    memset(wide, 14, sizeof(wide));
    CHECK(display_picture(NULL, 8, 2, wide, 16, 16));
    paint();
    complete_frame();
    int shared = s_vt->cells[2 * TD_VT_MAX_COLS + 8].fg;
    s_next_slot = shared;
    memset(pixels, 21, sizeof(pixels));
    CHECK(display_picture(NULL, 8, 2, pixels, 8, 16));
    CHECK(s_vt->cells[2 * TD_VT_MAX_COLS + 8].fg != shared);
    paint();
    complete_frame();
    CHECK(visible_pixel(8 * s_cw, 2 * s_ch) == s_palette[21]);
    CHECK(visible_pixel(9 * s_cw, 2 * s_ch) == s_palette[14]);

      /* A logical frame held by DEC 2026 must not clear part of an image. */
    s_vt->synchronized_output = true;
    background(2, 1, 25);
    before = test_present_count;
    paint();
    CHECK(test_present_count == before);
    CHECK(visible_pixel(2 * s_cw, s_ch) == s_palette[13]);
    s_vt->synchronized_output = false;
    paint();
    CHECK(visible_pixel(2 * s_cw, s_ch) == s_palette[13]);
    complete_frame();
    CHECK(visible_pixel(2 * s_cw, s_ch) == s_palette[25]);

      /* A frame boundary inside draw_bitmap precedes the acknowledgement
     * arm. It must not authorize recycling the just-submitted buffer. */
    test_early_boundary = true;
    background(0, 1, 30);
    paint();
    CHECK(visible_pixel(0, s_ch) == s_palette[30]);
    background(0, 1, 31);
    before = test_present_count;
    paint();
    CHECK(test_present_count == before);
    CHECK(visible_pixel(0, s_ch) == s_palette[30]);
    complete_frame();
    CHECK(visible_pixel(0, s_ch) == s_palette[30]);
    complete_frame();
    CHECK(visible_pixel(0, s_ch) == s_palette[31]);

      /* A return to a scene already held in the backbuffer must present it;
     * terminal status traffic must not present a duplicate visible frame. */
    while (test_requested)
        complete_frame();
    background(2, 0, 29);
    paint();
    complete_frame();
    background(2, 0, 30);
    paint();
    complete_frame();
    before = test_present_count;
    background(2, 0, 29);
    paint();
    CHECK_EQ(test_present_count, before + 1);
    complete_frame();
    CHECK_EQ(visible_pixel(2 * s_cw, 0), s_palette[29]);
    before = test_present_count;
    s_vt->dirty = true;
    paint();
    CHECK_EQ(test_present_count, before);

      /* Native RGB colors survive the local sink and frame-boundary handoff. */
    uint16_t exact[8 * 16];
    for (int i = 0; i < 8 * 16; i++)
        exact[i] = 0x0969;
    CHECK(display_picture_rgb(NULL, 3, 2, exact, 8, 16));
    paint();
    complete_frame();
    if (test_requested)
        complete_frame();
    CHECK_EQ(visible_pixel(3 * s_cw, 2 * s_ch), 0x0969);
    uint16_t *plane = s_rgb_plane;
    int next_slot = s_next_slot;
    for (int update = 0; update < 300; update++)
    {
        uint16_t color = (uint16_t)(0x0969 + update);
        for (int i = 0; i < 8 * 16; i++)
            exact[i] = color;
        CHECK(display_picture_rgb(NULL, 3, 2, exact, 8, 16));
        paint();
        complete_frame();
        if (test_requested)
            complete_frame();
        CHECK_EQ(visible_pixel(3 * s_cw, 2 * s_ch), color);
        CHECK(plane == s_rgb_plane);
        CHECK_EQ(s_next_slot, next_slot);
    }
     /* Cached runs cross rows and expand directly into the same RGB plane.
     * Invalid streams must fail before altering any destination pixels. */
    uint16_t encoded[] = {9, 0x0969, 25, 0x5555, 57, 0x17bf};
    CHECK(display_picture_runs(NULL, 3, 2, encoded, 6, 13, 7));
    paint();
    complete_frame();
    if (test_requested)
        complete_frame();
    for (int i = 0; i < 91; i++)
        CHECK_EQ(visible_pixel(3 * s_cw + i % 13, 2 * s_ch + i / 13), i < 9 ? 0x0969 : i < 34 ? 0x5555
                                                                                              : 0x17bf);
    CHECK_EQ(visible_pixel(3 * s_cw + 13, 2 * s_ch), 0);
    uint32_t sequence = s_rgb_sequence;
    uint16_t bad_runs[] = {0, 0xFFFF, 92, 0};
    CHECK(!display_picture_runs(NULL, 3, 2, bad_runs, 4, 13, 7));
    CHECK(!display_picture_runs(NULL, 3, 2, encoded, 5, 13, 7));
    CHECK(!display_picture_runs(NULL, 3, 2, encoded, 4, 13, 7));
    CHECK(!display_picture_runs(NULL, 3, 2, NULL, 6, 13, 7));
    CHECK_EQ(s_rgb_sequence, sequence);
    CHECK(plane == s_rgb_plane);
     /* Partial cells clear their padding rather than revealing an old page. */
    exact[0] = 0x0969;
    CHECK(display_picture_rgb(NULL, 3, 2, exact, 1, 1));
    paint();
    complete_frame();
    if (test_requested)
        complete_frame();
    CHECK_EQ(visible_pixel(3 * s_cw, 2 * s_ch), 0x0969);
    CHECK_EQ(visible_pixel(3 * s_cw + 1, 2 * s_ch), 0);
    for (int i = 0; i < 8 * 16; i++)
        exact[i] = 0x0969;

      /* Large RGB page backgrounds replace old logo tiles through the same
     * cache used on hardware. Keep updates inside one synchronized frame. */
    uint16_t *strip = malloc((size_t)DISPLAY_W * 2 * s_ch * sizeof(*strip));
    CHECK(strip != NULL);
    for (int page = 0; page < 3; page++)
    {
        uint16_t color = (uint16_t)(0x0969 + page * 64);
        for (int i = 0; i < DISPLAY_W * 2 * s_ch; i++)
            strip[i] = color;
        s_vt->synchronized_output = true;
        for (int row = 0; row < s_rows; row += 2)
            CHECK(display_picture_rgb(NULL, 0, row, strip, DISPLAY_W, (row + 1 < s_rows ? 2 : 1) * s_ch));
        if (page == 0)
            CHECK(display_picture_rgb(NULL, 3, 2, exact, 8, 16));
        before = test_present_count;
        paint();
        CHECK_EQ(test_present_count, before);
        s_vt->synchronized_output = false;
        paint();
        complete_frame();
        if (test_requested)
            complete_frame();
        CHECK_EQ(visible_pixel(DISPLAY_W - 1, DISPLAY_H - 1), color);
        CHECK_EQ(visible_pixel(3 * s_cw, 2 * s_ch), page == 0 ? 0x0969 : color);
    }
    free(strip);

      /* A VT move can put native source tiles at different destinations.
     * Adjacent destinations must not merge across a source-row wrap. */
    for (int i = 0; i < 8 * 16; i++)
        exact[i] = 0x0969;
    CHECK(display_picture_rgb(NULL, s_cols - 1, 5, exact, 8, 16));
    for (int i = 0; i < 8 * 16; i++)
        exact[i] = 0x17BF;
    CHECK(display_picture_rgb(NULL, 0, 6, exact, 8, 16));
    s_vt->cells[8 * TD_VT_MAX_COLS + 10] = s_vt->cells[5 * TD_VT_MAX_COLS + s_cols - 1];
    s_vt->cells[8 * TD_VT_MAX_COLS + 11] = s_vt->cells[6 * TD_VT_MAX_COLS];
    s_vt->dirty = true;
    paint();
    complete_frame();
    if (test_requested)
        complete_frame();
    CHECK_EQ(visible_pixel(10 * s_cw, 8 * s_ch), 0x0969);
    CHECK_EQ(visible_pixel(11 * s_cw, 8 * s_ch), 0x17BF);
    for (int i = 0; i < 8 * 16; i++)
        exact[i] = 0x0969;

      /* Capacity exhaustion is reported without stealing a live tile. */
    for (int r = 0; r < s_rows; r++)
        for (int c = 0; c < s_cols; c++)
            background(c, r, 17);
    memset(pixels, 14, sizeof(pixels));
    for (int i = 0; i < IMG_SLOTS; i++)
        CHECK(display_picture(NULL, i % s_cols, 4 + i / s_cols, pixels, 8, 16));
    CHECK(!display_picture(NULL, 0, 7, pixels, 8, 16));
    CHECK(s_six.buf == NULL);
     /* RGB composition has no per-picture slot allocation. An unrelated
     * native field must work even when every legacy Sixel slot is live. */
    CHECK(display_picture_rgb(NULL, 0, 7, exact, 8, 16));
    paint();
    complete_frame();
    CHECK_EQ(visible_pixel(0, 7 * s_ch), 0x0969);
    for (int i = 0; i < IMG_SLOTS; i++)
        CHECK(visible_pixel((i % s_cols) * s_cw, (4 + i / s_cols) * s_ch) == s_palette[14]);
    for (int i = 0; i < IMG_SLOTS; i++)
        free(s_slots[i].px);
    free(s_rgb_plane);
    free(s_vt);
    free(test_buffers[0]);
    free(test_buffers[1]);
    free(s_frame_done);
    free(s_lock);
    free(s_img_lock);
     /* Orientation: a touch at the panel's pixel lands on the turned
     * picture's pixel (portrait 480 x 800 at 90 and 270 degrees). */
    int tx, ty, tw, th;
    display_set_rotation(90);
    display_touch_point(799, 0, &tx, &ty, &tw, &th);
    CHECK(tx == 0 && ty == 0 && tw == 480 && th == 800);
    display_touch_point(0, 479, &tx, &ty, &tw, &th);
    CHECK(tx == 479 && ty == 799);
    display_set_rotation(270);
    display_touch_point(0, 479, &tx, &ty, &tw, &th);
    CHECK(tx == 0 && ty == 0);
    display_set_rotation(180);
    display_touch_point(799, 479, &tx, &ty, &tw, &th);
    CHECK(tx == 0 && ty == 0 && tw == 800 && th == 480);
    display_set_rotation(0);
    return TD_TEST_RESULT();
}
