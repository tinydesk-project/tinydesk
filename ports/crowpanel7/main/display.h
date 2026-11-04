/*
 * display.h - the CrowPanel's 800x480 LCD as a terminal: the desktop's
 * output goes into a td_vterm, and a task paints its cells onto the panel:
 * 8x16 pixels, 100x30 of them, or with the board configuration key
 * lcd.font = 20, 24 or 32 a larger font (Terminus Font, bold): 10x20 (80x24
 * cells), 12x24 (66x20), 16x32 (50x15). Larger cells make all text larger
 * and still sharp; the desktop follows the cell count like a terminal's.
 * Pictures come as Sixel and are painted at the panel's own resolution
 * (the desktop's td_picture.h): the terminal says it has Sixel and gives
 * its cell size in pixels.
 */
#ifndef CROWPANEL_DISPLAY_H
#define CROWPANEL_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define DISPLAY_W    800
#define DISPLAY_H    480
#define DISPLAY_MAX_COLS (DISPLAY_W / 8)    /* with the smallest font */
#define DISPLAY_MAX_ROWS (DISPLAY_H / 16)

/* The cells the panel shows (after display_init()). */
int display_cols(void);
int display_rows(void);

/* Called with the terminal's answers (to the desktop's size and status
 * queries); they go back to the desktop as input. */
typedef void (*display_reply_fn)(const char *data, int len);

/* Panel, backlight, terminal and painting task; false on error. */
bool display_init(display_reply_fn reply);

/* Desktop output (the UI task). */
void display_write(const uint8_t *buf, int len);
bool display_picture(void *user, int x, int y, const uint8_t *pixels, int w, int h);
bool display_picture_rgb(void *user,int x,int y,const uint16_t *pixels,int w,int h);
bool display_picture_runs(void *user,int x,int y,const uint16_t *runs,size_t words,int w,int h);
void display_scanout_info(uint32_t out[8]);
void display_scanout_snapshot(uint32_t out[12]);

#endif
