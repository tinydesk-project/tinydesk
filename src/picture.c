/*
 * picture.c - pictures at the terminal's own resolution (td_picture.h):
 * what the terminal can show, the pictures of a frame, and Sixel data.
 */
#include "tinydesk/td_picture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>

#include "tinydesk/td.h"
#include "tinydesk/td_wm.h"

#ifndef TD_PICTURE_MAX
#define TD_PICTURE_MAX 64
#endif
#define MAX_PICS TD_PICTURE_MAX

static bool s_sixel;
static int s_cw, s_ch;
static void (*s_scanout_monitor)(uint32_t out[12]);

void td_picture_set_scanout_monitor(void (*monitor)(uint32_t out[12]))
{
    s_scanout_monitor = monitor;
}
void td_picture_scanout_info(uint32_t out[8])
{
    uint32_t all[12] = {0};
    if (s_scanout_monitor)
        s_scanout_monitor(all);
    memcpy(out, all, 8 * sizeof(*out));
}
void td_picture_scanout_timing(uint32_t out[4])
{
    uint32_t all[12] = {0};
    if (s_scanout_monitor)
        s_scanout_monitor(all);
    memcpy(out, all + 8, 4 * sizeof(*out));
}

bool td_term_images(void)
{
    return s_sixel && s_cw > 0 && s_ch > 0;
}

void td_term_cell_px(int *w, int *h)
{
    *w = s_cw;
    *h = s_ch;
}

/* A change shows at once: every window is drawn again, with pictures or
 * without. */
void td_term_set_sixel(bool on)
{
    if (on == s_sixel)
        return;
    s_sixel = on;
    td_pictures_forget();
    td_wm_invalidate();
}

void td_term_set_cell_px(int w, int h)
{
    if (w == s_cw && h == s_ch)
        return;
    s_cw = w > 0 && w < 256 ? w : 0;
    s_ch = h > 0 && h < 256 ? h : 0;
    td_pictures_forget();
    td_wm_invalidate();
}

/* ------------------------------------------------------------ palette */

void td_palette_rgb(uint8_t i, uint8_t rgb[3])
{
    static const uint8_t basic[16][3] = {
        {0, 0, 0},
        {205, 0, 0},
        {0, 205, 0},
        {205, 205, 0},
        {0, 0, 238},
        {205, 0, 205},
        {0, 205, 205},
        {229, 229, 229},
        {127, 127, 127},
        {255, 0, 0},
        {0, 255, 0},
        {255, 255, 0},
        {92, 92, 255},
        {255, 0, 255},
        {0, 255, 255},
        {255, 255, 255},
    };
    static const uint8_t level[6] = {0, 95, 135, 175, 215, 255};
    if (i < 16)
    {
        memcpy(rgb, basic[i], 3);
    }
    else if (i < 232)
    {
        int n = i - 16;
        rgb[0] = level[n / 36];
        rgb[1] = level[(n / 6) % 6];
        rgb[2] = level[n % 6];
    }
    else
    {
        rgb[0] = rgb[1] = rgb[2] = (uint8_t)(8 + (i - 232) * 10);
    }
}

/* -------------------------------------------------------------- sixel */

typedef struct
{
    void (*write)(void *user, const char *s, int n);
    void *user;
} out_t;

static void put(out_t *o, const char *s)
{
    o->write(o->user, s, (int)strlen(s));
}

/* A run of n sixels ch: "!n" + ch when that is shorter. */
static void run(out_t *o, int n, char ch)
{
    char b[16];
    if (n <= 0)
        return;
    if (n > 3)
    {
        snprintf(b, sizeof(b), "!%d%c", n, ch);
        put(o, b);
        return;
    }
    for (int i = 0; i < n; i++)
        b[i] = ch;
    o->write(o->user, b, n);
}

uint16_t td_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
}

uint16_t td_palette_rgb565(uint8_t index)
{
    uint8_t rgb[3];
    td_palette_rgb(index, rgb);
    return td_rgb565(rgb[0], rgb[1], rgb[2]);
}

void td_rgb565_rgb(uint16_t p, uint8_t rgb[3])
{
    rgb[0] = (uint8_t)(((p >> 11) << 3) | (p >> 13));
    rgb[1] = (uint8_t)((((p >> 5) & 63) << 2) | ((p >> 9) & 3));
    rgb[2] = (uint8_t)(((p & 31) << 3) | ((p & 31) >> 2));
}

static void sixel_palette(const uint8_t *px, int w, int h, const uint8_t palette[256][3], void (*write)(void *user, const char *s, int n), void *user)
{
    out_t o = {write, user};
    char b[48];
    /* P2 = 1: pixels not drawn stay as they are; 1:1 pixel aspect. */
    put(&o, "\x1bP0;1;0q");
    snprintf(b, sizeof(b), "\"1;1;%d;%d", w, h);
    put(&o, b);
    static bool used[256];
    memset(used, 0, sizeof(used));
    for (int i = 0; i < w * h; i++)
        used[px[i]] = true;
    for (int c = 0; c < 256; c++)
    {
        if (!used[c])
            continue;
        uint8_t rgb[3];
        if (palette)
            memcpy(rgb, palette[c], 3);
        else
            td_palette_rgb((uint8_t)c, rgb);
        snprintf(b, sizeof(b), "#%d;2;%d;%d;%d", c, (rgb[0] * 100 + 127) / 255, (rgb[1] * 100 + 127) / 255,
                 (rgb[2] * 100 + 127) / 255);
        put(&o, b);
    }
    static bool in_band[256];
    for (int y0 = 0; y0 < h; y0 += 6)
    {
        int rows = h - y0 < 6 ? h - y0 : 6;
        memset(in_band, 0, sizeof(in_band));
        for (int k = 0; k < rows; k++)
            for (int x = 0; x < w; x++)
                in_band[px[(y0 + k) * w + x]] = true;
        bool first = true;
        for (int c = 0; c < 256; c++)
        {
            if (!in_band[c])
                continue;
            if (!first)
                put(&o, "$");   /* back to the band's start for the next colour */
            first = false;
            snprintf(b, sizeof(b), "#%d", c);
            put(&o, b);
            char last = 0;
            int count = 0;
            for (int x = 0; x < w; x++)
            {
                int bits = 0;
                for (int k = 0; k < rows; k++)
                    if (px[(y0 + k) * w + x] == c)
                        bits |= 1 << k;
                char ch = (char)(63 + bits);
                if (ch == last)
                {
                    count++;
                    continue;
                }
                run(&o, count, last);
                last = ch;
                count = 1;
            }
            if (last != 63)   /* trailing blanks need not be sent */
                run(&o, count, last);
        }
        put(&o, "-");
    }
    put(&o, "\x1b\\");
}

void td_sixel_encode(const uint8_t *px, int w, int h, void (*write)(void *, const char *, int), void *user)
{
    sixel_palette(px, w, h, NULL, write, user);
}

bool td_sixel_encode_rgb(const uint16_t *px, int w, int h, void (*write)(void *, const char *, int), void *user)
{
    /* Exact per-picture colors for skins/gradients. A bounded RGB332 palette
     * handles photographs with more than 256 colors; terminal cells keep
     * their existing palette. No global palette mutation is involved. */
    uint8_t *indexed = malloc((size_t)w * h);
    uint32_t *map = calloc(512, sizeof(*map));
    uint8_t(*palette)[3] = calloc(256, 3);
    if (!indexed || !map || !palette)
    {
        free(indexed);
        free(map);
        free(palette);
        return false;
    }
    int colors = 0;
    bool overflow = false;
    for (int i = 0; i < w * h; i++)
    {
        uint16_t c = px[i];
        unsigned slot = ((unsigned)c * 40503u) & 511u;
        while (map[slot] && (uint16_t)(map[slot] >> 8) != c)
            slot = (slot + 1) & 511u;
        if (!map[slot])
        {
            if (colors == 256)
            {
                overflow = true;
                break;
            }
            td_rgb565_rgb(c, palette[colors]);
            map[slot] = 0x1000000u | ((uint32_t)c << 8) | (unsigned)colors++;
        }
        indexed[i] = (uint8_t)map[slot];
    }
    if (overflow)
    {
        for (int i = 0; i < 256; i++)
        {
            palette[i][0] = (uint8_t)(((i >> 5) * 255 + 3) / 7);
            palette[i][1] = (uint8_t)((((i >> 2) & 7) * 255 + 3) / 7);
            palette[i][2] = (uint8_t)(((i & 3) * 255 + 1) / 3);
        }
        for (int i = 0; i < w * h; i++)
        {
            uint8_t rgb[3];
            td_rgb565_rgb(px[i], rgb);
            indexed[i] = (uint8_t)((rgb[0] & 224) | ((rgb[1] >> 3) & 28) | (rgb[2] >> 6));
        }
    }
    sixel_palette(indexed, w, h, (const uint8_t(*)[3])palette, write, user);
    free(indexed);
    free(map);
    free(palette);
    return true;
}

/* ------------------------------------------------------------ frames */

typedef struct
{
    int x, y, w, h;
    uint32_t id;
    td_picture_render_fn render;
    td_picture_render_rgb_fn render_rgb;
    td_picture_runs_fn runs;
    void *user;
    bool send;
    bool retained, completed;
    uint32_t previous_id;
} pic_t;

static pic_t s_cur[MAX_PICS];
static int s_ncur;
static int s_clip_cols;   /* 0: all; else only pictures within these columns go out */

void td_pictures_clip(int cols)
{
    s_clip_cols = cols > 0 ? cols : 0;
}
int td_pictures_available(void)
{
    return MAX_PICS - s_ncur;
}

typedef struct
{
    int x, y, w, h;
    uint32_t id;
} sent_t;

static sent_t s_sent[MAX_PICS];
static int s_nsent;
static atomic_uint s_render_count, s_alloc_failures, s_sink_failures;
static atomic_uint s_picture_frames, s_picture_total, s_picture_peak, s_picture_last;
static atomic_uint s_runs_ms;
static td_picture_runs_sink_fn s_runs_sink;
static void *s_runs_user;
void td_picture_set_runs_sink(td_picture_runs_sink_fn sink, void *user)
{
    s_runs_sink = sink;
    s_runs_user = user;
    td_pictures_forget();
}
uint32_t td_picture_runs_ms(void)
{
    return atomic_load(&s_runs_ms);
}
void td_picture_profile_reset(void)
{
    atomic_store(&s_picture_frames, 0);
    atomic_store(&s_picture_total, 0);
    atomic_store(&s_picture_peak, 0);
    atomic_store(&s_picture_last, 0);
    atomic_store(&s_runs_ms, 0);
}
void td_picture_profile(uint32_t out[4])
{
    out[0] = atomic_load(&s_picture_frames);
    out[1] = atomic_load(&s_picture_total);
    out[2] = atomic_load(&s_picture_peak);
    out[3] = atomic_load(&s_picture_last);
}

void td_picture_stats(uint32_t *renders, uint32_t *allocation_failures, uint32_t *sink_failures)
{
    *renders = atomic_load(&s_render_count);
    *allocation_failures = atomic_load(&s_alloc_failures);
    *sink_failures = atomic_load(&s_sink_failures);
}

void td_pictures_begin(void)
{
    s_ncur = 0;
}

void td_picture_add(int x, int y, int cols, int rows, uint32_t id, td_picture_render_fn render, void *user)
{
    const td_stats_t *st = td_stats();
    if (!td_term_images() || s_ncur >= MAX_PICS || !render)
        return;
    /* Only wholly on the screen: a cut picture would need cutting here. */
    if (x < 0 || y < 0 || cols <= 0 || rows <= 0 || x + cols > st->cols || y + rows > st->rows)
        return;
    s_cur[s_ncur++] = (pic_t){.x = x, .y = y, .w = cols, .h = rows, .id = id, .render = render, .user = user};
}

void td_picture_add_rgb(int x, int y, int cols, int rows, uint32_t id, td_picture_render_fn fallback, td_picture_render_rgb_fn render, void *user)
{
    int before = s_ncur;
    td_picture_add(x, y, cols, rows, id, fallback, user);
    if (s_ncur > before)
        s_cur[s_ncur - 1].render_rgb = render;
}

void td_picture_add_rgb_runs(int x, int y, int cols, int rows, uint32_t id, td_picture_render_fn fallback, td_picture_render_rgb_fn render, td_picture_runs_fn cached, void *user)
{
    int before = s_ncur;
    td_picture_add_rgb(x, y, cols, rows, id, fallback, render, user);
    if (s_ncur > before)
        s_cur[s_ncur - 1].runs = cached;
}

void td_pictures_mark(td_buffer_t *front, const td_buffer_t *back)
{
    /* An image that disappears must expose all its fallback cells, even
     * those whose logical contents have not changed since it was drawn. */
    for (int j = 0; j < s_nsent; j++)
    {
        sent_t *old = &s_sent[j];
        bool covered = false;
        for (int i = 0; i < s_ncur && !covered; i++)
            covered = old->x == s_cur[i].x && old->y == s_cur[i].y && old->w == s_cur[i].w && old->h == s_cur[i].h;
        if (!covered)
            for (int y = old->y; y < old->y + old->h && y < front->rows; y++)
                for (int x = old->x; x < old->x + old->w && x < front->cols; x++)
                    front->cells[y * front->cols + x].ch = UINT32_MAX;
    }
    for (int i = 0; i < s_ncur; i++)
    {
        pic_t *p = &s_cur[i];
        bool known = false;
        for (int j = 0; j < s_nsent; j++)
            if (s_sent[j].x == p->x && s_sent[j].y == p->y && s_sent[j].w == p->w && s_sent[j].h == p->h)
            {
                p->retained = true;
                p->previous_id = s_sent[j].id;
                known = s_sent[j].id == p->id;
                break;
            }
        p->send = !known || front->cols != back->cols || front->rows != back->rows;
        /* A changed lower layer overwrites unchanged pictures above it.
         * Propagate that damage in painter order, including RGB backdrops. */
        for (int j = 0; j < i && !p->send; j++)
        {
            const pic_t *lower = &s_cur[j];
            if (lower->send && lower->x < p->x + p->w && p->x < lower->x + lower->w && lower->y < p->y + p->h && p->y < lower->y + lower->h)
                p->send = true;
        }
        for (int y = p->y; y < p->y + p->h && !p->send; y++)
            if (memcmp(&front->cells[y * back->cols + p->x], &back->cells[y * back->cols + p->x],
                       sizeof(td_cell_t) * (size_t)p->w) != 0)
                p->send = true;
        /* Do not paint block characters before a native image, including
         * a newly loaded page as well as a pressed/replacement image.
         * CrowPanel swaps the new image in when its complete DCS arrives. */
        if (front->cols == back->cols && front->rows == back->rows)
            for (int y = p->y; y < p->y + p->h; y++)
                memcpy(&front->cells[y * back->cols + p->x], &back->cells[y * back->cols + p->x], sizeof(td_cell_t) * (size_t)p->w);
    }
}

static td_picture_sink_fn s_sink;
static void *s_sink_user;
static td_picture_rgb_sink_fn s_rgb_sink;
static void *s_rgb_user;
bool td_picture_local_rgb(void)
{
    return s_rgb_sink != NULL;
}

void td_picture_set_rgb_sink(td_picture_rgb_sink_fn sink, void *user)
{
    s_rgb_sink = sink;
    s_rgb_user = user;
    td_pictures_forget();
}

void td_picture_set_sink(td_picture_sink_fn sink, void *user)
{
    s_sink = sink;
    s_sink_user = user;
    td_pictures_forget();
}

void td_pictures_send(void (*write)(void *user, const char *s, int n), void *user, bool *cursor_moved)
{
    uint32_t start = td_millis();
    bool changed = false;
    int cw = s_cw, ch = s_ch;
    for (int i = 0; i < s_ncur; i++)
    {
        pic_t *p = &s_cur[i];
        if (!p->send || (s_clip_cols && p->x + p->w > s_clip_cols))
        {
            /* Nothing new, or on the extended part of the screen, which the
             * second terminal shows (without pictures). */
            p->completed = true;
            continue;
        }
        int w = p->w * cw, h = p->h * ch;
        changed = true;
        atomic_fetch_add(&s_render_count, 1);
        if (p->render_rgb)
        {
            const uint16_t *runs = NULL;
            size_t words = 0;
            if (s_runs_sink && p->runs && p->runs(p->user, w, h, &runs, &words))
            {
                uint32_t began = td_millis();
                bool completed = s_runs_sink(s_runs_user, p->x, p->y, runs, words, w, h);
                atomic_fetch_add(&s_runs_ms, td_millis() - began);
                if (completed)
                {
                    p->completed = true;
                    continue;
                }
                atomic_fetch_add(&s_sink_failures, 1);
            }
            uint16_t *rgb = malloc((size_t)w * h * sizeof(*rgb));
            if (!rgb)
            {
                atomic_fetch_add(&s_alloc_failures, 1);
                continue;
            }
            p->render_rgb(p->user, rgb, w, h);
            bool completed = s_rgb_sink && s_rgb_sink(s_rgb_user, p->x, p->y, rgb, w, h);
            if (s_rgb_sink && !completed)
                atomic_fetch_add(&s_sink_failures, 1);
            if (!completed)
            {
                char seq[32];
                int n = snprintf(seq, sizeof(seq), "\x1b"
                                                   "7\x1b[%d;%dH",
                                 p->y + 1, p->x + 1);
                write(user, seq, n);
                completed = td_sixel_encode_rgb(rgb, w, h, write, user);
                write(user, "\x1b"
                            "8",
                      2);
                *cursor_moved = true;
            }
            free(rgb);
            p->completed = completed;
            continue;
        }
        uint8_t *px = malloc((size_t)w * (size_t)h);
        if (!px)
        {
            atomic_fetch_add(&s_alloc_failures, 1);
            continue;
        }
        p->render(p->user, px, w, h);
        if (s_sink && s_sink(s_sink_user, p->x, p->y, px, w, h))
        {
            free(px);
            p->completed = true;
            continue;
        }
        char seq[32];
        int n = snprintf(seq, sizeof(seq), "\x1b"
                                           "7\x1b[%d;%dH",
                         p->y + 1, p->x + 1);
        write(user, seq, n);
        td_sixel_encode(px, w, h, write, user);
        write(user, "\x1b"
                    "8",
              2);
        free(px);
        p->completed = true;
        *cursor_moved = true;
    }
    s_nsent = 0;
    for (int i = 0; i < s_ncur; i++)
    {
        pic_t *p = &s_cur[i];
        if (p->completed || p->retained)
            s_sent[s_nsent++] = (sent_t){p->x, p->y, p->w, p->h, p->completed ? p->id : p->previous_id};
    }
    if (changed)
    {
        uint32_t ms = td_millis() - start;
        atomic_store(&s_picture_last, ms);
        atomic_fetch_add(&s_picture_total, ms);
        if (ms > atomic_load(&s_picture_peak))
            atomic_store(&s_picture_peak, ms);
        atomic_fetch_add(&s_picture_frames, 1);
    }
}

void td_pictures_forget(void)
{
    s_nsent = 0;
}
