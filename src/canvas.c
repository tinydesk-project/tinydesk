/*
 * canvas.c - pixel surfaces drawn with half-block characters (td_canvas.h).
 */
#include <stdlib.h>
#include <string.h>

#include "tinydesk/td_canvas.h"
#include "tinydesk/td_screen.h"

#define UPPER_HALF_BLOCK 0x2580u

static td_canvas_alloc_fn s_alloc = malloc;
static td_canvas_free_fn s_free = free;

void td_canvas_set_allocator(td_canvas_alloc_fn alloc_fn, td_canvas_free_fn free_fn)
{
    s_alloc = alloc_fn ? alloc_fn : malloc;
    s_free = free_fn ? free_fn : free;
}

/* w x h cleared pixels from the port's allocator. */
static uint8_t *pixels_new(int w, int h)
{
    size_t bytes = (size_t)w * (size_t)h;
    uint8_t *px = s_alloc(bytes);
    if (px)
        memset(px, 0, bytes);
    return px;
}

td_canvas_t *td_canvas_new(int w, int h)
{
    if (w < 1 || h < 1)
        return NULL;
    td_canvas_t *c = malloc(sizeof(*c));
    if (!c)
        return NULL;
    c->px = pixels_new(w, h);
    if (!c->px)
    {
        free(c);
        return NULL;
    }
    c->w = w;
    c->h = h;
    return c;
}

void td_canvas_free(td_canvas_t *c)
{
    if (!c)
        return;
    s_free(c->px);
    free(c);
}

bool td_canvas_resize(td_canvas_t *c, int w, int h)
{
    if (!c || w < 1 || h < 1)
        return false;
    uint8_t *px = pixels_new(w, h);
    if (!px)
        return false;
    s_free(c->px);
    c->px = px;
    c->w = w;
    c->h = h;
    return true;
}

void td_canvas_clear(td_canvas_t *c, uint8_t color)
{
    if (c)
        memset(c->px, color, (size_t)c->w * (size_t)c->h);
}

void td_canvas_pixel(td_canvas_t *c, int x, int y, uint8_t color)
{
    if (c && x >= 0 && y >= 0 && x < c->w && y < c->h)
        c->px[y * c->w + x] = color;
}

uint8_t td_canvas_get(const td_canvas_t *c, int x, int y)
{
    if (c && x >= 0 && y >= 0 && x < c->w && y < c->h)
        return c->px[y * c->w + x];
    return 0;
}

void td_canvas_fill_rect(td_canvas_t *c, int x, int y, int w, int h, uint8_t color)
{
    if (!c)
        return;
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > c->w ? c->w : x + w, y1 = y + h > c->h ? c->h : y + h;
    for (int row = y0; row < y1; ++row)
    {
        if (x1 > x0)
            memset(c->px + row * c->w + x0, color, (size_t)(x1 - x0));
    }
}

void td_canvas_line(td_canvas_t *c, int x0, int y0, int x1, int y1, uint8_t color)
{
    /* Bresenham; every point is clipped by td_canvas_pixel(). */
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;)
    {
        td_canvas_pixel(c, x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        int e2 = 2 * err;
        if (e2 >= dy)
        {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

void td_canvas_blit(td_canvas_t *c, int x, int y, const uint8_t *src, int w, int h, int stride)
{
    if (!c || !src)
        return;
    for (int row = 0; row < h; ++row)
    {
        int ty = y + row;
        if (ty < 0 || ty >= c->h)
            continue;
        int from = x < 0 ? -x : 0;
        int to = x + w > c->w ? c->w - x : w;
        if (to > from)
            memcpy(c->px + ty * c->w + x + from, src + row * stride + from, (size_t)(to - from));
    }
}

void td_canvas_draw(const td_canvas_t *c, int x, int y)
{
    if (!c)
        return;
    for (int row = 0; row < (c->h + 1) / 2; ++row)
    {
        const uint8_t *top = c->px + (2 * row) * c->w;
        const uint8_t *bottom = 2 * row + 1 < c->h ? top + c->w : NULL;
        for (int col = 0; col < c->w; ++col)
            td_putc(x + col, y + row, UPPER_HALF_BLOCK, top[col], bottom ? bottom[col] : 0, 0);
    }
}

/* The xterm cube's six levels per channel. */
static const uint8_t s_level[6] = {0, 95, 135, 175, 215, 255};

static int nearest_level(int v)
{
    if (v < 48)
        return 0;
    if (v < 115)
        return 1;
    return (v - 35) / 40;
}

static int dist2(int r1, int g1, int b1, int r2, int g2, int b2)
{
    return (r1 - r2) * (r1 - r2) + (g1 - g2) * (g1 - g2) + (b1 - b2) * (b1 - b2);
}

uint8_t td_rgb256(uint8_t r, uint8_t g, uint8_t b)
{
    int ri = nearest_level(r), gi = nearest_level(g), bi = nearest_level(b);
    int cube = 16 + 36 * ri + 6 * gi + bi;
    int cube_d = dist2(r, g, b, s_level[ri], s_level[gi], s_level[bi]);

    /* Grey ramp 232-255: 8, 18, ..., 238. */
    int avg = (r + g + b) / 3;
    int gray_i = avg < 3 ? 0 : avg > 233 ? 23
                                         : (avg - 3) / 10;
    int gray = 8 + 10 * gray_i;
    int gray_d = dist2(r, g, b, gray, gray, gray);

    return (uint8_t)(gray_d < cube_d ? 232 + gray_i : cube);
}
