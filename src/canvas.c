/*
 * canvas.c - pixel surfaces drawn with block characters (td_canvas.h).
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

static td_canvas_detail_t s_detail = TD_CANVAS_HALF;

void td_canvas_set_detail(td_canvas_detail_t detail)
{
    s_detail = detail == TD_CANVAS_QUAD ? TD_CANVAS_QUAD : TD_CANVAS_HALF;
}

td_canvas_detail_t td_canvas_detail(void)
{
    return s_detail;
}

/* Quadrants need UTF-8: ASCII mode draws half blocks. */
static bool quad_on(void)
{
    return s_detail == TD_CANVAS_QUAD && !td_get_ascii_mode();
}

void td_canvas_fit(int cols, int rows, int *w, int *h)
{
    *w = cols > 0 ? cols * (quad_on() ? 2 : 1) : 0;
    *h = rows > 0 ? rows * 2 : 0;
}

static void draw_quad(const td_canvas_t *c, int x, int y);

void td_canvas_draw(const td_canvas_t *c, int x, int y)
{
    if (!c)
        return;
    if (quad_on())
    {
        draw_quad(c, x, y);
        return;
    }
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

/* Every grey of the palette (the cube's six and the ramp's 24), ascending,
 * and its index. */
static const uint8_t s_grey[30] = {0, 8, 18, 28, 38, 48, 58, 68, 78, 88, 95, 98, 108, 118, 128,
                                   135, 138, 148, 158, 168, 175, 178, 188, 198, 208, 215, 218, 228, 238, 255};
static const uint8_t s_grey_index[30] = {16, 232, 233, 234, 235, 236, 237, 238, 239, 240,
                                         59, 241, 242, 243, 244, 102, 245, 246, 247, 248,
                                         145, 249, 250, 251, 252, 188, 253, 254, 255, 231};

/* The level at or below v, or the one above it when v's distance past the
 * lower one exceeds the threshold t (0-15) as a share of the gap. */
static int dither_pick(const uint8_t *levels, int n, int v, int t)
{
    int i = 0;
    while (i + 1 < n && levels[i + 1] <= v)
        i++;
    if (i + 1 >= n)
        return i;
    int lo = levels[i], hi = levels[i + 1];
    return (v - lo) * 32 > (2 * t + 1) * (hi - lo) ? i + 1 : i;
}

uint8_t td_rgb256_dither(uint8_t r, uint8_t g, uint8_t b, int x, int y)
{
    static const uint8_t bayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};
    int t = bayer[y & 3][x & 3];
    int lo = r < g ? (r < b ? r : b) : (g < b ? g : b);
    int hi = r > g ? (r > b ? r : b) : (g > b ? g : b);
    if (hi - lo <= 12) /* (nearly) grey: the ramp has finer steps */
        return s_grey_index[dither_pick(s_grey, 30, (r + g + b) / 3, t)];
    return (uint8_t)(16 + 36 * dither_pick(s_level, 6, r, t) + 6 * dither_pick(s_level, 6, g, t) +
                     dither_pick(s_level, 6, b, t));
}

/* The red, green and blue of a palette index (0-15: xterm's defaults). */
static void palette_rgb(uint8_t i, int rgb[3])
{
    static const uint8_t basic[16][3] = {{0, 0, 0}, {205, 0, 0}, {0, 205, 0}, {205, 205, 0}, {0, 0, 238}, {205, 0, 205}, {0, 205, 205}, {229, 229, 229}, {127, 127, 127}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0}, {92, 92, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}};
    if (i < 16)
    {
        rgb[0] = basic[i][0];
        rgb[1] = basic[i][1];
        rgb[2] = basic[i][2];
    }
    else if (i >= 232)
    {
        rgb[0] = rgb[1] = rgb[2] = 8 + 10 * (i - 232);
    }
    else
    {
        rgb[0] = s_level[(i - 16) / 36];
        rgb[1] = s_level[(i - 16) / 6 % 6];
        rgb[2] = s_level[(i - 16) % 6];
    }
}

/* Quadrant characters by the pixels in the foreground colour: bit 3 upper
 * left, 2 upper right, 1 lower left, 0 lower right. Masks with the upper
 * left bit set are drawn inverted, so only the first eight are used. */
static const uint32_t s_quad_glyph[8] = {' ', 0x2597, 0x2596, 0x2584, 0x259D, 0x2590, 0x259E, 0x259F};

void td_canvas_quad_cell(const uint8_t px[4], uint32_t *ch, uint8_t *fg, uint8_t *bg)
{
    /* The distinct colours, and which one each pixel has. */
    uint8_t col[4];
    int n = 0, idx[4];
    for (int i = 0; i < 4; ++i)
    {
        int k = 0;
        while (k < n && col[k] != px[i])
            k++;
        if (k == n)
            col[n++] = px[i];
        idx[i] = k;
    }

    /* Two colours of them, and each pixel's nearer one; with three or four
     * colours, the pair that changes the pixels least. */
    int a = 0, b = n > 1 ? 1 : 0;
    int d[4][4] = {{0}};
    if (n > 2)
    {
        int rgb[4][3];
        for (int k = 0; k < n; ++k)
            palette_rgb(col[k], rgb[k]);
        for (int k = 0; k < n; ++k)
            for (int l = 0; l < n; ++l)
                d[k][l] = dist2(rgb[k][0], rgb[k][1], rgb[k][2], rgb[l][0], rgb[l][1], rgb[l][2]);
        int best = -1;
        for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j)
            {
                int err = 0;
                for (int p = 0; p < 4; ++p)
                    err += d[idx[p]][i] < d[idx[p]][j] ? d[idx[p]][i] : d[idx[p]][j];
                if (best < 0 || err < best)
                {
                    best = err;
                    a = i;
                    b = j;
                }
            }
    }
    int mask = 0;
    for (int p = 0; p < 4; ++p)
        if (idx[p] == a || (idx[p] != b && d[idx[p]][a] <= d[idx[p]][b]))
            mask |= 8 >> p;
    if (mask & 8)
    {
        mask ^= 15;
        int t = a;
        a = b;
        b = t;
    }
    *ch = s_quad_glyph[mask];
    *fg = mask ? col[a] : col[b];
    *bg = col[b];
}

/* Two pixels by two in each cell. Pixels past the edge are colour 0. */
static void draw_quad(const td_canvas_t *c, int x, int y)
{
    for (int row = 0; row < (c->h + 1) / 2; ++row)
    {
        const uint8_t *top = c->px + (2 * row) * c->w;
        const uint8_t *bottom = 2 * row + 1 < c->h ? top + c->w : NULL;
        for (int col = 0; col < (c->w + 1) / 2; ++col)
        {
            int x0 = 2 * col, x1 = 2 * col + 1;
            uint8_t px[4] = {top[x0], x1 < c->w ? top[x1] : 0, bottom ? bottom[x0] : 0,
                             bottom && x1 < c->w ? bottom[x1] : 0};
            uint32_t ch;
            uint8_t fg, bg;
            td_canvas_quad_cell(px, &ch, &fg, &bg);
            td_putc(x + col, y + row, ch, fg, bg, 0);
        }
    }
}
