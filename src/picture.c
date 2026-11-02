/*
 * picture.c - pictures at the terminal's own resolution (td_picture.h):
 * what the terminal can show, the pictures of a frame, and Sixel data.
 */
#include "tinydesk/td_picture.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tinydesk/td.h"
#include "tinydesk/td_wm.h"

#define MAX_PICS 64

static bool s_sixel;
static int s_cw, s_ch;

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

void td_sixel_encode(const uint8_t *px, int w, int h, void (*write)(void *user, const char *s, int n), void *user)
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

/* ------------------------------------------------------------ frames */

typedef struct
{
    int x, y, w, h;
    uint32_t id;
    td_picture_render_fn render;
    void *user;
    bool send;
} pic_t;

static pic_t s_cur[MAX_PICS];
static int s_ncur;

typedef struct
{
    int x, y, w, h;
    uint32_t id;
} sent_t;

static sent_t s_sent[MAX_PICS];
static int s_nsent;

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
    s_cur[s_ncur++] = (pic_t){x, y, cols, rows, id, render, user, false};
}

void td_pictures_mark(const td_buffer_t *front, const td_buffer_t *back)
{
    for (int i = 0; i < s_ncur; i++)
    {
        pic_t *p = &s_cur[i];
        bool known = false;
        for (int j = 0; j < s_nsent && !known; j++)
            known = s_sent[j].id == p->id && s_sent[j].x == p->x && s_sent[j].y == p->y && s_sent[j].w == p->w &&
                    s_sent[j].h == p->h;
        p->send = !known || front->cols != back->cols || front->rows != back->rows;
        for (int y = p->y; y < p->y + p->h && !p->send; y++)
            if (memcmp(&front->cells[y * back->cols + p->x], &back->cells[y * back->cols + p->x],
                       sizeof(td_cell_t) * (size_t)p->w) != 0)
                p->send = true;
    }
}

void td_pictures_send(void (*write)(void *user, const char *s, int n), void *user, bool *cursor_moved)
{
    int cw = s_cw, ch = s_ch;
    for (int i = 0; i < s_ncur; i++)
    {
        pic_t *p = &s_cur[i];
        if (!p->send)
            continue;
        int w = p->w * cw, h = p->h * ch;
        uint8_t *px = malloc((size_t)w * (size_t)h);
        if (!px)
            continue;
        p->render(p->user, px, w, h);
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
        *cursor_moved = true;
    }
    s_nsent = 0;
    for (int i = 0; i < s_ncur; i++)
        s_sent[s_nsent++] = (sent_t){s_cur[i].x, s_cur[i].y, s_cur[i].w, s_cur[i].h, s_cur[i].id};
}

void td_pictures_forget(void)
{
    s_nsent = 0;
}
