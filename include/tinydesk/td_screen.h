/*
 * td_screen.h - cell buffers, drawing primitives, UTF-8 helpers and the
 * diff renderer.
 */
#ifndef TD_SCREEN_H
#define TD_SCREEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "td_config.h"
#include "td_hal.h"

/* ---------------------------------------------------------------- cells */

/* Cell attributes. */
#define TD_BOLD      0x01u
#define TD_UNDERLINE 0x02u
#define TD_REVERSE   0x04u

/* One character cell: 8 bytes, so 80x25 is 16 KB per buffer. */
typedef struct
{
    uint32_t ch;     /* Unicode code point (ASCII + box drawing + blocks) */
    uint8_t fg, bg;  /* 256-colour palette indexes */
    uint8_t attr;    /* TD_BOLD | TD_UNDERLINE | TD_REVERSE */
    uint8_t _pad;
} td_cell_t;

/* A value no real cell ever holds; used to force a full redraw. */
#define TD_CH_INVALID 0xFFFFFFFFu

/* A screen-sized grid of cells. Storage is static and sized for the
 * maximum terminal; cols/rows give the part in use. */
typedef struct
{
    int cols, rows;
    td_cell_t cells[TD_MAX_COLS * TD_MAX_ROWS];
} td_buffer_t;

typedef struct
{
    int x, y, w, h;
} td_rect_t;

/* Build a rectangle. */
static inline td_rect_t td_rect(int x, int y, int w, int h)
{
    td_rect_t r = {x, y, w, h};
    return r;
}

/* Intersection of two rectangles (w or h is 0 when they do not overlap). */
td_rect_t td_rect_intersect(td_rect_t a, td_rect_t b);

/* True if the point lies inside the rectangle. */
bool td_rect_contains(td_rect_t r, int x, int y);

/* Set the used size (clamped to the maximum) and clear to spaces. */
void td_buffer_init(td_buffer_t *buf, int cols, int rows);

/* Fill every used cell with ch = TD_CH_INVALID so the next diff resends
 * everything. */
void td_buffer_invalidate(td_buffer_t *buf);

/* Cell at (x, y) or NULL when outside the used area. */
td_cell_t *td_buffer_cell(td_buffer_t *buf, int x, int y);

/* ------------------------------------------------------------ drawing */

/* Box styles for td_box(). */
typedef enum
{
    TD_BOX_SINGLE = 0,
    TD_BOX_DOUBLE,
    TD_BOX_ASCII,
} td_box_style_t;

/* Select the buffer the primitives draw into; resets origin and clip to
 * the whole buffer. */
void td_draw_target(td_buffer_t *buf);

/* Current drawing target (NULL before td_draw_target()). */
td_buffer_t *td_draw_get_target(void);

/* Coordinates given to primitives are relative to this origin. */
void td_draw_origin(int x, int y);

/* Clip rectangle in absolute screen coordinates; nothing is drawn
 * outside it. The clip is always intersected with the buffer. */
void td_draw_clip(td_rect_t clip);

/* Current clip rectangle (absolute). */
td_rect_t td_draw_get_clip(void);

/* Put one cell. */
void td_putc(int x, int y, uint32_t ch, uint8_t fg, uint8_t bg, uint8_t attr);

/* Fill a rectangle. */
void td_fill(td_rect_t r, uint32_t ch, uint8_t fg, uint8_t bg);

/* Draw UTF-8 text on one line; returns the number of columns used. */
int td_text(int x, int y, const char *str, uint8_t fg, uint8_t bg, uint8_t attr);

/* Like td_text() but writes at most max_cols columns. */
int td_textn(int x, int y, const char *str, int max_cols,
             uint8_t fg, uint8_t bg, uint8_t attr);

/* Draw a frame around the edge of the rectangle (interior untouched). */
void td_box(td_rect_t r, td_box_style_t style, uint8_t fg, uint8_t bg);

/* Darken the one-cell drop shadow to the right of and below r. */
void td_shadow(td_rect_t r);

/* Draw a vertical scrollbar of height h at (x, y) for a view showing
 * `page` of `total` lines starting at `pos`. */
void td_draw_vscroll(int x, int y, int h, int pos, int total, int page,
                     uint8_t fg, uint8_t bg);

/* ASCII-only mode: box drawing and block characters are replaced by
 * + - | # . at render time. */
void td_set_ascii_mode(bool on);
bool td_get_ascii_mode(void);

/* ASCII replacement for a code point (returns ch itself if < 128). */
uint32_t td_ascii_fallback(uint32_t ch);

/* -------------------------------------------------------------- UTF-8 */

/* Encode a code point; returns the byte count (1..4). Invalid code points
 * are encoded as U+FFFD. */
int td_utf8_encode(uint32_t cp, uint8_t out[4]);

/* Decode one code point from *s and advance *s. Returns 0 at the end of the
 * string. Malformed input yields U+FFFD and skips one byte. */
uint32_t td_utf8_next(const char **s);

/* Number of code points in a NUL-terminated UTF-8 string. */
int td_utf8_len(const char *s);

/* A column of exactly `cols` cells for a list or table: copies `s` (NULL
 * counts as "") into `out`, at most `cols` code points and never part of
 * one, then pads with spaces to `cols` code points. Code points are counted
 * as the screen draws them (td_utf8_next(): a malformed byte is one cell),
 * so strings that are not valid UTF-8 line up too. Stops early when `out`
 * is full; always NUL-terminates when cap > 0. Returns the bytes written,
 * without the NUL. Double-width characters (CJK, emoji) count as one cell,
 * as everywhere in TinyDesk. */
int td_utf8_pad(char *out, size_t cap, const char *s, int cols);

/* Like td_utf8_pad() without the padding: copies at most `cols` code points
 * of `s` (NULL counts as ""), never part of one, and stops early when `out`
 * is full. To shorten text to fit a buffer, pass INT_MAX for `cols`. Always
 * NUL-terminates when cap > 0; returns the bytes written. */
int td_utf8_copy(char *out, size_t cap, const char *s, int cols);

/* The part of `s` after its first `cols` code points (the end of the string
 * if it is shorter), counted as td_utf8_next() does. */
const char *td_utf8_skip(const char *s, int cols);

/* Streaming decoder for byte-at-a-time input. */
typedef struct
{
    uint32_t cp;
    uint8_t need;   /* continuation bytes still expected */
    uint8_t len;    /* continuation bytes of the current sequence */
} td_utf8_decoder_t;

/* Feed one byte. Returns how many code points were completed (0, 1 or 2)
 * and stores them in out. Malformed input produces U+FFFD; a sequence cut
 * short by a new lead byte yields U+FFFD followed by that character. */
int td_utf8_feed(td_utf8_decoder_t *d, uint8_t byte, uint32_t out[2]);

/* ----------------------------------------------------------- renderer */

typedef struct
{
    const td_hal_t *hal;
    int cur_x, cur_y;          /* terminal cursor, -1 = unknown */
    int cur_fg, cur_bg, cur_attr; /* terminal colour state, -1 = unknown */
    uint8_t out[TD_OUT_BUF_SIZE];
    int out_len;
    bool write_failed;         /* a write in this frame came up short */
    uint32_t bytes_sent;       /* running total, for statistics */
} td_renderer_t;

/* Prepare a renderer that writes through hal. */
void td_render_init(td_renderer_t *r, const td_hal_t *hal);

/* Forget the terminal cursor and colour state (after a reconnect). */
void td_render_reset_state(td_renderer_t *r);

/* Queue raw bytes (escape sequences) into the output buffer. */
void td_render_raw(td_renderer_t *r, const char *s);
void td_render_write(td_renderer_t *r, const void *data, int len);

/* Send everything queued. Returns false if the terminal did not accept
 * all bytes (the rest of the frame is dropped). */
bool td_render_flush(td_renderer_t *r);

/* Send the cells of back that differ from front, then copy back to front.
 * Returns false if bytes were dropped; the caller should invalidate front
 * and redraw later. */
bool td_render_diff(td_renderer_t *r, td_buffer_t *front, const td_buffer_t *back);

/* Terminal setup / teardown sequences (alternate screen, hidden cursor,
 * mouse reporting, no auto-wrap). */
void td_render_setup_terminal(td_renderer_t *r);
void td_render_restore_terminal(td_renderer_t *r);

/* Ask the terminal for its size; the answer arrives as TD_EV_RESIZE. */
void td_render_query_size(td_renderer_t *r);

#endif /* TD_SCREEN_H */
