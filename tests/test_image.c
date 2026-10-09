/*
 * test_image.c - BMP and PPM/PGM files read row by row (td_image_* in
 * td_apps.h): every supported kind, the row order, padding, colour tables,
 * bit-field masks, 16-bit samples, plain (text) PPM/PGM, and the errors for what is not supported.
 * The files are written to a temporary folder and read through td_fs_stdio.
 */
#include <stdio.h>
#include <stdlib.h>

#include "td_apps.h"
#include "td_fs_stdio.h"
#include "td_test.h"

static const td_fs_ops_t *s_fs;
static uint8_t s_file[8192];
static int s_len;

static void put8(int v)
{
    s_file[s_len++] = (uint8_t)v;
}
static void put16(int v)
{
    put8(v & 0xFF);
    put8((v >> 8) & 0xFF);
}
static void put32(uint32_t v)
{
    put16((int)(v & 0xFFFF));
    put16((int)(v >> 16));
}

/* A BMP header with a 40-byte info header; colours entries after it, then
 * (masks) 12 bytes of bit-field masks if masks is not NULL. */
static void bmp_header(int w, int h, int bpp, int comp, int colours, const uint32_t *masks)
{
    s_len = 0;
    int table = colours * 4 + (masks ? 12 : 0);
    put8('B');
    put8('M');
    put32(0); /* file size: not checked */
    put32(0);
    put32((uint32_t)(54 + table)); /* pixel data offset */
    put32(40);
    put32((uint32_t)w);
    put32((uint32_t)h);
    put16(1);
    put16(bpp);
    put32((uint32_t)comp);
    put32(0);
    put32(2835);
    put32(2835);
    put32((uint32_t)colours);
    put32(0);
    if (masks)
    {
        put32(masks[0]);
        put32(masks[1]);
        put32(masks[2]);
    }
}

static void save(const char *name)
{
    char path[64];
    snprintf(path, sizeof(path), "image_test/%s", name);
    FILE *f = fopen(path, "wb");
    fwrite(s_file, 1, (size_t)s_len, f);
    fclose(f);
}

static td_image_t s_img;
static uint8_t s_rgb[3 * 64];
static char s_err[96];

static bool open_image(const char *name)
{
    char path[64];
    snprintf(path, sizeof(path), "image_test/%s", name);
    s_err[0] = '\0';
    return td_image_load(&s_img, s_fs, path, s_err, (int)sizeof(s_err));
}

static bool pixel_is(int x, int r, int g, int b)
{
    return s_rgb[x * 3] == r && s_rgb[x * 3 + 1] == g && s_rgb[x * 3 + 2] == b;
}

static void test_bmp24(void)
{
    /* 3 x 2, bottom-up: the file holds the bottom row first. Rows are padded
     * to 12 bytes. */
    bmp_header(3, 2, 24, 0, 0, NULL);
    const uint8_t bottom[12] = {255, 0, 0, 0, 255, 0, 0, 0, 255, 0xEE, 0xEE, 0xEE}; /* B, G, R + padding */
    const uint8_t top[12] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 0xEE, 0xEE, 0xEE};
    memcpy(s_file + s_len, bottom, 12);
    s_len += 12;
    memcpy(s_file + s_len, top, 12);
    s_len += 12;
    save("a.bmp");
    CHECK(open_image("a.bmp"));
    CHECK_EQ(s_img.w, 3);
    CHECK_EQ(s_img.h, 2);
    CHECK(strcmp(s_img.format, "BMP 24-bit") == 0);
    int y;
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK_EQ(y, 1);                   /* the bottom row first */
    CHECK(pixel_is(0, 0, 0, 255));    /* stored blue, green, red */
    CHECK(pixel_is(1, 0, 255, 0));
    CHECK(pixel_is(2, 255, 0, 0));
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK_EQ(y, 0);
    CHECK(pixel_is(0, 30, 20, 10));
    CHECK(pixel_is(2, 90, 80, 70));
    CHECK(!td_image_row(&s_img, &y, s_rgb)); /* no more rows */
    td_image_rewind(&s_img);
    CHECK(td_image_row(&s_img, &y, s_rgb) && y == 1); /* from the start again */
    td_image_close(&s_img);
}

static void test_bmp_palette(void)
{
    /* 8-bit, top-down (negative height), 2 x 2, three colours. */
    bmp_header(2, -2, 8, 0, 3, NULL);
    const uint8_t table[12] = {0, 0, 0, 0, 255, 255, 255, 0, 0, 128, 255, 0}; /* BGRx */
    memcpy(s_file + s_len, table, 12);
    s_len += 12;
    const uint8_t rows[8] = {1, 2, 0, 0, 2, 0, 0, 0}; /* each row padded to 4 bytes */
    memcpy(s_file + s_len, rows, 8);
    s_len += 8;
    save("p8.bmp");
    CHECK(open_image("p8.bmp"));
    int y;
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK_EQ(y, 0); /* top-down */
    CHECK(pixel_is(0, 255, 255, 255));
    CHECK(pixel_is(1, 255, 128, 0));
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK(pixel_is(0, 255, 128, 0));
    CHECK(pixel_is(1, 0, 0, 0));
    td_image_close(&s_img);

    /* 1-bit, 10 pixels: the bits run from the high one. */
    bmp_header(10, 1, 1, 0, 2, NULL);
    const uint8_t mono[8] = {0, 0, 0, 0, 255, 255, 255, 0};
    memcpy(s_file + s_len, mono, 8);
    s_len += 8;
    put8(0xA0); /* 1010 0000 */
    put8(0xC0); /* 11 */
    put8(0);
    put8(0);
    save("p1.bmp");
    CHECK(open_image("p1.bmp"));
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK(pixel_is(0, 255, 255, 255));
    CHECK(pixel_is(1, 0, 0, 0));
    CHECK(pixel_is(2, 255, 255, 255));
    CHECK(pixel_is(8, 255, 255, 255));
    CHECK(pixel_is(9, 255, 255, 255));
    td_image_close(&s_img);

    /* 4-bit: the high nibble first. */
    bmp_header(2, 1, 4, 0, 2, NULL);
    memcpy(s_file + s_len, mono, 8);
    s_len += 8;
    put8(0x10);
    put8(0);
    put8(0);
    put8(0);
    save("p4.bmp");
    CHECK(open_image("p4.bmp"));
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK(pixel_is(0, 255, 255, 255));
    CHECK(pixel_is(1, 0, 0, 0));
    td_image_close(&s_img);
}

static void test_bmp_masks(void)
{
    /* 32-bit with bit-field masks (RGBA, red in the low byte). */
    const uint32_t masks[3] = {0x000000FFu, 0x0000FF00u, 0x00FF0000u};
    bmp_header(1, 1, 32, 3, 0, masks);
    put32(0x80332211u);
    save("m32.bmp");
    CHECK(open_image("m32.bmp"));
    int y;
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK(pixel_is(0, 0x11, 0x22, 0x33));
    td_image_close(&s_img);

    /* 16-bit without masks is 5-5-5: full red. */
    bmp_header(1, 1, 16, 0, 0, NULL);
    put16(0x7C00);
    put16(0);
    save("r555.bmp");
    CHECK(open_image("r555.bmp"));
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK(pixel_is(0, 255, 0, 0));
    td_image_close(&s_img);
}

static void test_pnm(void)
{
    /* PPM with a comment, maxval 255. */
    const char hdr[] = "P6\n# made by a test\n2 1\n255\n";
    s_len = 0;
    memcpy(s_file, hdr, sizeof(hdr) - 1);
    s_len = (int)sizeof(hdr) - 1;
    const uint8_t px[6] = {1, 2, 3, 250, 251, 252};
    memcpy(s_file + s_len, px, 6);
    s_len += 6;
    save("a.ppm");
    CHECK(open_image("a.ppm"));
    CHECK(strcmp(s_img.format, "PPM") == 0);
    int y;
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK_EQ(y, 0);
    CHECK(pixel_is(0, 1, 2, 3));
    CHECK(pixel_is(1, 250, 251, 252));
    td_image_close(&s_img);

    /* PGM, maxval 15: scaled to 0..255. */
    const char pgm[] = "P5 2 1 15 ";
    memcpy(s_file, pgm, sizeof(pgm) - 1);
    s_len = (int)sizeof(pgm) - 1;
    put8(15);
    put8(0);
    save("a.pgm");
    CHECK(open_image("a.pgm"));
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK(pixel_is(0, 255, 255, 255));
    CHECK(pixel_is(1, 0, 0, 0));
    td_image_close(&s_img);

    /* 16-bit samples, big-endian. */
    const char wide[] = "P6 1 1 65535\n";
    memcpy(s_file, wide, sizeof(wide) - 1);
    s_len = (int)sizeof(wide) - 1;
    put8(0xFF);
    put8(0xFF);
    put8(0x80);
    put8(0x00);
    put8(0x00);
    put8(0x00);
    save("w.ppm");
    CHECK(open_image("w.ppm"));
    CHECK(strcmp(s_img.format, "PPM 16-bit") == 0);
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK(pixel_is(0, 255, 127, 0));
    td_image_close(&s_img);
}

static void test_plain(void)
{
    /* P3: decimal samples, any white space, comments; maxval 7. A row may
     * span lines. */
    const char p3[] = "P3\n# typed in a terminal\n2 2 7\n7 0 0  0 7 0\n0 0\n7 # blue\n  7 7 7\n";
    memcpy(s_file, p3, sizeof(p3) - 1);
    s_len = (int)sizeof(p3) - 1;
    save("t.ppm");
    CHECK(open_image("t.ppm"));
    CHECK(strcmp(s_img.format, "PPM text") == 0);
    CHECK_EQ(s_img.w, 2);
    int y;
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK_EQ(y, 0);
    CHECK(pixel_is(0, 255, 0, 0));
    CHECK(pixel_is(1, 0, 255, 0));
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK_EQ(y, 1);
    CHECK(pixel_is(0, 0, 0, 255));
    CHECK(pixel_is(1, 255, 255, 255));
    CHECK(!td_image_row(&s_img, &y, s_rgb));
    td_image_rewind(&s_img);
    CHECK(td_image_row(&s_img, &y, s_rgb) && y == 0 && pixel_is(0, 255, 0, 0));
    td_image_close(&s_img);

    /* P2: a sample above maxval is clamped; a short file stops. */
    const char p2[] = "P2 3 2 255 0 128 999 10";
    memcpy(s_file, p2, sizeof(p2) - 1);
    s_len = (int)sizeof(p2) - 1;
    save("t.pgm");
    CHECK(open_image("t.pgm"));
    CHECK(strcmp(s_img.format, "PGM text") == 0);
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK(pixel_is(0, 0, 0, 0));
    CHECK(pixel_is(1, 128, 128, 128));
    CHECK(pixel_is(2, 255, 255, 255));
    CHECK(!td_image_row(&s_img, &y, s_rgb));
    td_image_close(&s_img);

    /* A plain file longer than the 4 KB window: 600 x 2 grey. */
    s_len = snprintf((char *)s_file, sizeof(s_file), "P2 600 2 255\n");
    for (int i = 0; i < 1200; ++i)
        s_len += snprintf((char *)s_file + s_len, sizeof(s_file) - (size_t)s_len, "%d%c", i % 200,
                          i % 20 == 19 ? '\n' : ' ');
    save("long.pgm");
    CHECK(s_len > 4096);
    CHECK(open_image("long.pgm"));
    static uint8_t wide[3 * 600];
    CHECK(td_image_row(&s_img, &y, wide));
    CHECK(td_image_row(&s_img, &y, wide));
    CHECK_EQ(y, 1);
    CHECK_EQ(wide[0], 600 % 200);
    CHECK_EQ(wide[3 * 599], 1199 % 200);
    td_image_close(&s_img);
}

static void test_errors(void)
{
    s_len = 0;
    memcpy(s_file, "hello", 5);
    s_len = 5;
    save("text.bmp");
    CHECK(!open_image("text.bmp"));
    CHECK(strstr(s_err, "Not a BMP") != NULL);

    bmp_header(2, 2, 8, 1, 0, NULL); /* RLE8 */
    save("rle.bmp");
    CHECK(!open_image("rle.bmp"));
    CHECK(strstr(s_err, "RLE") != NULL);

    bmp_header(5000, 1, 24, 0, 0, NULL);
    save("wide.bmp");
    CHECK(!open_image("wide.bmp"));
    CHECK(strstr(s_err, "too large") != NULL);

    CHECK(!open_image("missing.bmp"));
    CHECK(strstr(s_err, "cannot be read") != NULL);

    /* A file that ends early: the rows that are there, then false. */
    bmp_header(1, 3, 24, 0, 0, NULL);
    put32(0x00FFFFFFu); /* one row (3 bytes + padding) of three */
    save("short.bmp");
    CHECK(open_image("short.bmp"));
    int y;
    CHECK(td_image_row(&s_img, &y, s_rgb));
    CHECK(!td_image_row(&s_img, &y, s_rgb));
    td_image_close(&s_img);

    /* Without read_at the viewer says so. */
    td_fs_ops_t no_parts = *s_fs;
    no_parts.read_at = NULL;
    CHECK(!td_image_load(&s_img, &no_parts, "image_test/a.ppm", s_err, (int)sizeof(s_err)));
    CHECK(strstr(s_err, "in parts") != NULL);
}

static void test_names(void)
{
    CHECK(td_is_image("photo.bmp"));
    CHECK(td_is_image("PHOTO.BMP"));
    CHECK(td_is_image("a.ppm"));
    CHECK(td_is_image("a.Pgm"));
    CHECK(!td_is_image("a.png"));
    CHECK(!td_is_image("bmp"));
    CHECK(!td_is_image("a.bmpx"));
    CHECK(!td_is_image(NULL));
}

int main(void)
{
    s_fs = td_fs_stdio(".");
    s_fs->mkdir("image_test");
    test_names();
    test_bmp24();
    test_bmp_palette();
    test_bmp_masks();
    test_pnm();
    test_plain();
    test_errors();
    return TD_TEST_RESULT();
}
