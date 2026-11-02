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
 * under it was sent in this frame (something drew over it). The cells
 * under a picture should be plain blanks while it shows.
 */
#ifndef TD_PICTURE_H
#define TD_PICTURE_H

#include <stdbool.h>
#include <stdint.h>

#include "td_screen.h"

/* What the terminal can show (set from its answers). */
bool td_term_images(void);             /* Sixel and the cell size are known */
void td_term_cell_px(int *w, int *h);  /* a cell in pixels (0 x 0 if unknown) */
void td_term_set_sixel(bool on);
void td_term_set_cell_px(int w, int h);

/* Fill w x h pixels (xterm 256-colour palette indexes, row by row) with the
 * picture, for the terminal's cell size. */
typedef void (*td_picture_render_fn)(void *user, uint8_t *px, int w, int h);

/* During composing: a picture over cols x rows cells at screen cell (x, y).
 * id changes whenever the picture does (a new image, a pressed look). */
void td_picture_add(int x, int y, int cols, int rows, uint32_t id, td_picture_render_fn render, void *user);

/* The palette's colour of index i as 8-bit RGB. */
void td_palette_rgb(uint8_t i, uint8_t rgb[3]);

/* Sixel data for w x h palette indexes, through write (any chunk sizes). */
void td_sixel_encode(const uint8_t *px, int w, int h, void (*write)(void *user, const char *s, int n), void *user);

/* Used by the core (src/td.c) around composing and sending a frame. */
void td_pictures_begin(void);
void td_pictures_mark(const td_buffer_t *front, const td_buffer_t *back);
void td_pictures_send(void (*write)(void *user, const char *s, int n), void *user, bool *cursor_moved);
void td_pictures_forget(void);   /* the terminal was set up anew: send all again */

#endif
