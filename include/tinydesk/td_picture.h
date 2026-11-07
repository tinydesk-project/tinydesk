/*
 * td_picture.h - pictures at the terminal's own resolution.
 *
 * A picture covers a rectangle of cells and is drawn by the terminal in
 * real pixels, not with block characters: smooth corners, gradients and
 * text in any size, like an LCD panel's UI. It needs a terminal that shows
 * Sixel graphics and reports the size of its cells in pixels (the
 * CrowPanel's LCD, the web terminal, Windows Terminal, xterm with -ti
 * vt340); TinyDesk asks at every terminal setup. Elsewhere (PuTTY) there
 * are no pictures, and what is in the cells under them shows instead: an
 * app draws a block-character version there anyway.
 *
 * Pictures are added while the screen is composed (from a window's
 * on_draw), each frame anew. After the cells of a frame have been sent, a
 * picture is sent when it is new or changed (another id), or when any cell
 * under it was sent in this frame (something drew over it). Keep a useful
 * fallback in those cells: it may appear while the image is transmitted.
 */
#ifndef TD_PICTURE_H
#define TD_PICTURE_H

#include <stdbool.h>
#include <stdint.h>

#include "td_screen.h"
#include <stddef.h>

  /* What the terminal can show (set from its answers). */
bool td_term_images(void);             /* Sixel and the cell size are known */
void td_term_cell_px(int *w, int *h);  /* a cell in pixels (0 x 0 if unknown) */
void td_term_set_sixel(bool on);
void td_term_set_cell_px(int w, int h);

  /* Fill w x h pixels (xterm 256-colour palette indexes, row by row) with the
 * picture, for the terminal's cell size. */
typedef void (*td_picture_render_fn)(void *user, uint8_t *px, int w, int h);
typedef void (*td_picture_render_rgb_fn)(void *user, uint16_t *px, int w, int h);
typedef bool (*td_picture_rgb_sink_fn)(void *user, int x, int y, const uint16_t *px, int w, int h);
void td_picture_set_rgb_sink(td_picture_rgb_sink_fn sink, void *user);
bool td_picture_local_rgb(void);
 /* Optional synchronous lossless-run path. The borrowed pairs must expand
 * to exactly w*h RGB565 pixels. A false return uses normal RGB rendering. */
typedef bool (*td_picture_runs_fn)(void *user, int w, int h, const uint16_t **runs, size_t *words);
typedef bool (*td_picture_runs_sink_fn)(void *user, int x, int y, const uint16_t *runs, size_t words, int w, int h);
void td_picture_set_runs_sink(td_picture_runs_sink_fn sink, void *user);
void td_picture_add_rgb_runs(int x, int y, int cols, int rows, uint32_t id,
                             td_picture_render_fn fallback, td_picture_render_rgb_fn render,
                             td_picture_runs_fn cached, void *user);
uint32_t td_picture_runs_ms(void);
 /* Cumulative diagnostics, sampled without stopping the renderer. */
void td_picture_stats(uint32_t *renders, uint32_t *allocation_failures, uint32_t *sink_failures);
void td_picture_profile_reset(void);
void td_picture_profile(uint32_t out[4]); /* frames, total ms, peak ms, last ms */
 /* Optional platform scanout telemetry. Zeroes when no local RGB display:
 * vsyncs, prepared frames, missed preparation deadlines, peak preparation
 * gap (us), submitted frames, pixel clock (Hz), interrupt core, bounce lines.
 * Missed deadlines diagnose scanout pressure, not application redraw errors. */
 /* Monitor snapshot: the eight counters followed by submit timestamp (ms),
 * last/peak/total LCD paint work (us). Older eight-value monitors are valid. */
void td_picture_set_scanout_monitor(void (*monitor)(uint32_t out[12]));
void td_picture_scanout_info(uint32_t out[8]);
void td_picture_scanout_timing(uint32_t out[4]);
 /* Optional local display: consume palette pixels synchronously. False
 * falls back to Sixel. The pixels are borrowed only during this call. */
typedef bool (*td_picture_sink_fn)(void *user, int x, int y, const uint8_t *px, int w, int h);
void td_picture_set_sink(td_picture_sink_fn sink, void *user);

  /* During composing: a picture over cols x rows cells at screen cell (x, y).
 * id changes whenever the picture does (a new image, a pressed look). */
void td_picture_add(int x, int y, int cols, int rows, uint32_t id, td_picture_render_fn render, void *user);
void td_picture_add_rgb(int x, int y, int cols, int rows, uint32_t id,
                        td_picture_render_fn fallback, td_picture_render_rgb_fn render, void *user);
uint16_t td_rgb565(uint8_t r, uint8_t g, uint8_t b);
uint16_t td_palette_rgb565(uint8_t index);
void td_rgb565_rgb(uint16_t pixel, uint8_t rgb[3]);
bool td_sixel_encode_rgb(const uint16_t *px, int w, int h,
                         void (*write)(void *user, const char *s, int n), void *user);

  /* The palette's colour of index i as 8-bit RGB. */
void td_palette_rgb(uint8_t i, uint8_t rgb[3]);

  /* Sixel data for w x h palette indexes, through write (any chunk sizes). */
void td_sixel_encode(const uint8_t *px, int w, int h, void (*write)(void *user, const char *s, int n), void *user);

  /* Used by the core (src/td.c) around composing and sending a frame. */
void td_pictures_begin(void);
int td_pictures_available(void);
 /* Retain an existing image while its replacement is transmitted; update
 * the logical front cells without emitting their low-resolution fallback. */
void td_pictures_mark(td_buffer_t *front, const td_buffer_t *back);
void td_pictures_send(void (*write)(void *user, const char *s, int n), void *user, bool *cursor_moved);
void td_pictures_forget(void);   /* the terminal was set up anew: send all again */
 /* Only pictures inside area go out (w 0: all), placed from its corner:
 * the primary terminal's display on an extended screen. */
void td_pictures_clip(td_rect_t area);
 /* The window manager, while composing: pictures added so far, and drop
 * those of first .. last - 1 that r overlaps (a window, menu or the taskbar
 * drawn above them), so they cannot cover it. */
int td_pictures_count(void);
void td_pictures_cover(int first, int last, td_rect_t r);

#endif
