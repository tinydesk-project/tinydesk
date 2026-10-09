/*
 * td_canvas.h - pixel surfaces, drawn with block characters.
 *
 * A canvas is w x h pixels of one byte each: an index into the terminal's
 * 256-colour palette (td_rgb256() finds one for an RGB colour). Apps draw
 * into it and show it from on_draw with td_canvas_draw(). How many pixels a
 * cell shows is the user's picture detail (Settings, td_canvas_set_detail()):
 *
 *   TD_CANVAS_HALF  two, one above the other: U+2580 (upper half block)
 *                   with the upper pixel as the foreground and the lower one
 *                   as the background. A canvas takes w columns and
 *                   (h + 1) / 2 rows. Every terminal TinyDesk runs in can
 *                   show it. The default.
 *   TD_CANVAS_QUAD  four, two by two, with quadrant characters (U+2596 -
 *                   U+259F): twice the columns' detail, but still two
 *                   colours per cell, so where four pixels differ the cell
 *                   shows the two that fit best. Needs a font that has the
 *                   quadrants (Cascadia Mono, DejaVu Sans Mono). A canvas
 *                   takes (w + 1) / 2 columns. ASCII mode draws half blocks.
 *
 * Size a canvas with td_canvas_fit() from the cells it should cover, and
 * check in on_draw that it still fits: the detail can change at any time
 * (every window is then drawn again). The renderer sends only the cells
 * that changed.
 *
 * Memory: w x h bytes plus a small header, allocated by td_canvas_new()
 * while an app needs it (a 78 x 44 canvas is 3.4 KB). The pixels come from
 * td_canvas_set_allocator()'s allocator, malloc() by default.
 */
#ifndef TD_CANVAS_H
#define TD_CANVAS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    int w, h;
    uint8_t *px; /* w * h palette indexes, row by row */
} td_canvas_t;

/* Where the pixels are allocated. A port with external RAM sets this once
 * at start-up (the ESP-IDF port: PSRAM when the board has it), so canvases
 * do not use the internal RAM other code needs. alloc returns `bytes` of
 * memory or NULL; the canvas clears them itself. NULL for both restores
 * malloc() and free(). Change it only while no canvas exists. */
typedef void *(*td_canvas_alloc_fn)(size_t bytes);
typedef void (*td_canvas_free_fn)(void *ptr);
void td_canvas_set_allocator(td_canvas_alloc_fn alloc_fn, td_canvas_free_fn free_fn);

/* A canvas of w x h pixels, all colour 0 (black); NULL without memory or
 * for a size below 1 x 1. */
td_canvas_t *td_canvas_new(int w, int h);

/* Free a canvas (NULL is ignored). */
void td_canvas_free(td_canvas_t *c);

/* Change the size; every pixel becomes colour 0. Returns false and keeps
 * the canvas as it was when there is not enough memory. */
bool td_canvas_resize(td_canvas_t *c, int w, int h);

/* Set every pixel. */
void td_canvas_clear(td_canvas_t *c, uint8_t color);

/* Set one pixel; nothing happens outside the canvas. */
void td_canvas_pixel(td_canvas_t *c, int x, int y, uint8_t color);

/* A pixel's colour, or 0 outside the canvas. */
uint8_t td_canvas_get(const td_canvas_t *c, int x, int y);

/* Fill a rectangle, clipped to the canvas. */
void td_canvas_fill_rect(td_canvas_t *c, int x, int y, int w, int h, uint8_t color);

/* A straight line from (x0, y0) to (x1, y1), both ends included, clipped. */
void td_canvas_line(td_canvas_t *c, int x0, int y0, int x1, int y1, uint8_t color);

/* Copy w x h palette indexes (rows `stride` bytes apart) to (x, y), clipped. */
void td_canvas_blit(td_canvas_t *c, int x, int y, const uint8_t *src, int w, int h, int stride);

/* Draw the canvas into the current drawing target with its top-left cell
 * at (x, y), relative to the drawing origin and clipped like td_putc(),
 * in the current detail. Pixels a cell has no canvas pixel for (an odd
 * height, or an odd width in TD_CANVAS_QUAD) are colour 0. */
void td_canvas_draw(const td_canvas_t *c, int x, int y);

/* Picture detail: pixels per cell. */
typedef enum
{
    TD_CANVAS_HALF, /* 1 x 2 */
    TD_CANVAS_QUAD  /* 2 x 2 */
} td_canvas_detail_t;

/* Set by the Settings app from the user's settings; apps only read it. */
void td_canvas_set_detail(td_canvas_detail_t detail);
td_canvas_detail_t td_canvas_detail(void);

/* The canvas size in pixels that fills cols x rows cells in the detail
 * drawn now (TD_CANVAS_QUAD in ASCII mode draws as TD_CANVAS_HALF). */
void td_canvas_fit(int cols, int rows, int *w, int *h);

/* The character and colours td_canvas_draw() uses in TD_CANVAS_QUAD for
 * four pixels (upper left, upper right, lower left, lower right). */
void td_canvas_quad_cell(const uint8_t px[4], uint32_t *ch, uint8_t *fg, uint8_t *bg);

/* The nearest colour of the xterm 256-colour palette: the 6 x 6 x 6 cube
 * (16-231) or the grey ramp (232-255), whichever is closer. */
uint8_t td_rgb256(uint8_t r, uint8_t g, uint8_t b);

/* td_rgb256() with ordered (4 x 4 Bayer) dithering for the pixel at (x, y):
 * picks one of the two palette levels around each channel so that a patch
 * of pixels averages to the RGB colour. Palette colours stay exact; (nearly)
 * grey colours use all 30 greys of the palette. For pictures and gradients. */
uint8_t td_rgb256_dither(uint8_t r, uint8_t g, uint8_t b, int x, int y);

#endif
