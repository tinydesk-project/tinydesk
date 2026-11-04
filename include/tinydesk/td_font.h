/* Portable generated bitmap fonts. The caller owns the immutable file bytes. */
#ifndef TD_FONT_H
#define TD_FONT_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TD_FONT_MAX_BYTES (256u * 1024u)
typedef struct {
    const uint8_t *data;
    size_t size;
    uint16_t height, ascent, count, pixels;
    uint8_t bpp;
} td_font_t;
bool td_font_open(td_font_t *font, const uint8_t *data, size_t size);
void td_font_measure(const td_font_t *font, const char *text, int width, bool wrap, int *w, int *h);
/* Draw into an existing palette buffer; 0/1/2 alignment = start/center/end. */
void td_font_draw(const td_font_t *font, const char *text, uint8_t *pixels,
                  int width, int height, uint8_t color, int align, int valign, bool wrap);
void td_font_draw_rgb(const td_font_t *font,const char *text,uint16_t *pixels,
                      int width,int height,uint8_t color,int align,int valign,bool wrap);
#endif
