#include "tinydesk/td_font.h"
#include "tinydesk/td_canvas.h"
#include "tinydesk/td_picture.h"
#include <limits.h>
#include <string.h>

static uint16_t u16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (uint16_t)p[1] << 8);
}
static uint32_t u32(const uint8_t *p)
{
    return (uint32_t)u16(p) | (uint32_t)u16(p + 2) << 16;
}

bool td_font_open(td_font_t *f, const uint8_t *d, size_t n)
{
    if (!f || !d || n < 32 || n > TD_FONT_MAX_BYTES || memcmp(d, "TDFONT01", 8) ||
        u16(d + 8) != 32 || (d[10] != 1 && d[10] != 4) || d[11] ||
        !u16(d + 12) || u16(d + 12) > 128 || u16(d + 14) > u16(d + 12) ||
        !u16(d + 16) || u16(d + 16) > 512 || !u16(d + 18) || u16(d + 18) > 128 ||
        u32(d + 20) != 32 || u32(d + 24) != 32u + 16u * u16(d + 16) ||
        u32(d + 24) > n || u32(d + 28) != n)
        return false;
    uint32_t previous = 0, end = u32(d + 24);
    bool replacement = false;
    for (unsigned i = 0; i < u16(d + 16); i++)
    {
        const uint8_t *g = d + 32 + 16 * i;
        uint32_t cp = u32(g), offset = u32(g + 4);
        unsigned width = u16(g + 8), advance = u16(g + 12);
        size_t bytes = ((width * d[10] + 7u) / 8u) * u16(d + 12);
        if (cp < 32 || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff) ||
            (i && cp <= previous) || width > 256 || !advance || advance > 256 ||
            (int16_t)u16(g + 10) < -128 || (int16_t)u16(g + 10) > 128 ||
            u16(g + 14) || offset != end || bytes > n - offset)
            return false;
        end = offset + (uint32_t)bytes;
        previous = cp;
        replacement |= cp == '?';
    }
    if (end != n || !replacement)
        return false;
    *f = (td_font_t){d, n, u16(d + 12), u16(d + 14), u16(d + 16), u16(d + 18), d[10]};
    return true;
}

static const uint8_t *glyph(const td_font_t *f, uint32_t cp)
{
    unsigned lo = 0, hi = f->count;
    while (lo < hi)
    {
        unsigned mid = lo + (hi - lo) / 2;
        const uint8_t *g = f->data + 32 + 16 * mid;
        uint32_t code = u32(g);
        if (code == cp)
            return g;
        if (code < cp)
            lo = mid + 1;
        else
            hi = mid;
    }
    return cp == '?' ? NULL : glyph(f, '?');
}

typedef struct
{
    const char *end, *next;
    int width;
} line_t;
static line_t line(const td_font_t *f, const char *start, int limit, bool wrap)
{
    const char *p = start, *space = NULL, *after_space = NULL;
    int w = 0, before_space = 0;
    while (*p && *p != '\n')
    {
        const char *next = p;
        uint32_t cp = td_utf8_next(&next);
        const uint8_t *g = glyph(f, cp);
        int advance = g ? u16(g + 12) : 0;
        if (wrap && limit > 0 && w + advance > limit && p != start)
            return space ? (line_t){space, after_space, before_space} : (line_t){p, p, w};
        if (cp == ' ')
        {
            space = p;
            after_space = next;
            before_space = w;
        }
        if (w > INT_MAX - advance)
            break;
        w += advance;
        p = next;
    }
    return (line_t){p, *p == '\n' ? p + 1 : p, w};
}

void td_font_measure(const td_font_t *f, const char *text, int width, bool wrap, int *w, int *h)
{
    *w = *h = 0;
    if (!f || !f->data || !text)
        return;
    const char *p = text;
    for (;;)
    {
        line_t l = line(f, p, width, wrap);
        if (l.width > *w)
            *w = l.width;
        if (*h <= INT_MAX - f->height)
            *h += f->height;
        bool newline = l.next > l.end;
        p = l.next;
        if (!*p && !newline)
            break;
    }
}

static void draw(const td_font_t *f, const char *text, void *px, int w, int h,
                 uint8_t color, int align, int valign, bool wrap, bool native)
{
    if (!f || !f->data || !text || !px || w < 1 || h < 1)
        return;
    int tw, th;
    td_font_measure(f, text, w, wrap, &tw, &th);
    (void)tw;
    int y = valign == 1 ? (h - th) / 2 : valign == 2 ? h - th
                                                     : 0;
    uint8_t rgb[3];
    td_palette_rgb(color, rgb);
    const char *p = text;
    for (;;)
    {
        line_t l = line(f, p, w, wrap);
        int x = align == 1 ? (w - l.width) / 2 : align == 2 ? w - l.width
                                                            : 0;
        while (p < l.end)
        {
            const uint8_t *g = glyph(f, td_utf8_next(&p));
            if (!g)
                continue;
            int gw = u16(g + 8), bearing = (int16_t)u16(g + 10);
            unsigned stride = (unsigned)(gw * f->bpp + 7) / 8;
            const uint8_t *bits = f->data + u32(g + 4);
            for (int gy = 0; gy < f->height; gy++)
                for (int gx = 0; gx < gw; gx++)
                {
                    int dx = x + bearing + gx, dy = y + gy;
                    if (dx < 0 || dy < 0 || dx >= w || dy >= h)
                        continue;
                    unsigned value = bits[(unsigned)gy * stride + (unsigned)gx * f->bpp / 8];
                    unsigned a = f->bpp == 1 ? ((value >> (7 - gx % 8)) & 1u) * 15u : (value >> (gx % 2 ? 0 : 4)) & 15u;
                    if (!a)
                        continue;
                    size_t at = (size_t)dy * w + dx;
                    if (native)
                    {
                        uint16_t *dst = (uint16_t *)px + at;
                        if (a == 15)
                            *dst = td_rgb565(rgb[0], rgb[1], rgb[2]);
                        else
                        {
                            uint8_t bg[3];
                            td_rgb565_rgb(*dst, bg);
                            *dst = td_rgb565((uint8_t)((rgb[0] * a + bg[0] * (15 - a) + 7) / 15),
                                             (uint8_t)((rgb[1] * a + bg[1] * (15 - a) + 7) / 15),
                                             (uint8_t)((rgb[2] * a + bg[2] * (15 - a) + 7) / 15));
                        }
                        continue;
                    }
                    uint8_t *dst = (uint8_t *)px + at;
                    if (a == 15)
                        *dst = color;
                    else
                    {
                        uint8_t bg[3];
                        td_palette_rgb(*dst, bg);
                        *dst = td_rgb256((uint8_t)((rgb[0] * a + bg[0] * (15 - a) + 7) / 15),
                                         (uint8_t)((rgb[1] * a + bg[1] * (15 - a) + 7) / 15),
                                         (uint8_t)((rgb[2] * a + bg[2] * (15 - a) + 7) / 15));
                    }
                }
            x += u16(g + 12);
        }
        bool newline = l.next > l.end;
        p = l.next;
        y += f->height;
        if (!*p && !newline)
            break;
        if (y >= h)
            break;
    }
}

void td_font_draw(const td_font_t *f, const char *text, uint8_t *px, int w, int h, uint8_t color, int align, int valign, bool wrap)
{
    draw(f, text, px, w, h, color, align, valign, wrap, false);
}

void td_font_draw_rgb(const td_font_t *f, const char *text, uint16_t *px, int w, int h, uint8_t color, int align, int valign, bool wrap)
{
    draw(f, text, px, w, h, color, align, valign, wrap, true);
}
