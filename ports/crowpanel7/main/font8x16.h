/*
 * font8x16.h - the 8x16 font of the CrowPanel's LCD (font8x16.c, generated
 * by tools/make_lcd_font.py).
 */
#ifndef FONT8X16_H
#define FONT8X16_H

#include <stdint.h>

/* The 16 rows of a character (bit 7 is the leftmost pixel), or NULL when
 * the font does not have it. */
const uint8_t *font8x16_glyph(uint16_t cp);

#endif
