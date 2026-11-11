/*
 * pixels.c - the pixel canvas demo (td_canvas.h): an animated plasma and
 * the 256-colour palette, with frames per second and the link's bytes per
 * second, to see what a terminal link can show.
 *
 *   Space  next picture      P  pause / go on
 *
 * The canvas follows the window: resize or maximise it for more pixels.
 * It is allocated while the window is open (width x height bytes).
 */
#include <stdio.h>

#include "td_apps.h"
#include "tinydesk/td_canvas.h"

#define TICK_MS 33   /* at most about 30 pictures a second */
#define SHADES  64

enum
{
    MODE_PLASMA,
    MODE_PALETTE,
    MODE_COUNT
};

static td_window_t *s_win;
static td_canvas_t *s_canvas;
static int s_mode;
static bool s_paused;
static unsigned s_t;            /* animation time, in ticks */
static int8_t s_sin[256];       /* one turn, -127..127 */
static uint8_t s_shade[SHADES]; /* a colour wheel in the 256-colour palette */
static int s_frames, s_fps;     /* pictures drawn this second, last second */
static uint32_t s_second_ms;

/* A sine table without floating point: a parabola per half turn. */
static void make_tables(void)
{
    for (int i = 0; i < 256; ++i)
    {
        int h = i & 127;
        int v = h * (128 - h) * 127 / 4096;
        s_sin[i] = (int8_t)(i < 128 ? v : -v);
    }
    for (int i = 0; i < SHADES; ++i)
    {
        /* Hue around the wheel: red, yellow, green, cyan, blue, magenta. */
        int seg = i * 6 / SHADES, f = (i * 6 % SHADES) * 255 / SHADES;
        int r = 0, g = 0, b = 0;
        switch (seg)
        {
        case 0:
            r = 255, g = f;
            break;
        case 1:
            r = 255 - f, g = 255;
            break;
        case 2:
            g = 255, b = f;
            break;
        case 3:
            g = 255 - f, b = 255;
            break;
        case 4:
            r = f, b = 255;
            break;
        default:
            r = 255, b = 255 - f;
            break;
        }
        s_shade[i] = td_rgb256((uint8_t)r, (uint8_t)g, (uint8_t)b);
    }
}

static void draw_plasma(td_canvas_t *c, unsigned t)
{
    for (int y = 0; y < c->h; ++y)
    {
        uint8_t *row = c->px + y * c->w;
        int sy = s_sin[(y * 6 + t * 3) & 255];
        for (int x = 0; x < c->w; ++x)
        {
            int v = s_sin[(x * 5 + t * 2) & 255] + sy + s_sin[((x + y) * 4 + t) & 255] +
                    s_sin[((x * x + y * y) / 16 + t * 4) & 255];
            row[x] = s_shade[((v + 512) / 8 + (int)t) & (SHADES - 1)];
        }
    }
}

/* All 256 palette colours as a 16 x 16 grid. */
static void draw_palette(td_canvas_t *c)
{
    for (int y = 0; y < c->h; ++y)
        for (int x = 0; x < c->w; ++x)
            c->px[y * c->w + x] = (uint8_t)((y * 16 / c->h) * 16 + x * 16 / c->w);
}

static void on_draw(td_window_t *win, int cw, int ch)
{
    (void)win;
    const td_theme_t *th = td_theme();
    int pw, ph;
    td_canvas_fit(cw, ch - 1, &pw, &ph); /* the last row is the status line */
    if (ph < 2 || pw < 2)
        return;
    if (!s_canvas)
        s_canvas = td_canvas_new(pw, ph);
    else if (s_canvas->w != pw || s_canvas->h != ph)
        td_canvas_resize(s_canvas, pw, ph);
    if (!s_canvas || s_canvas->w != pw || s_canvas->h != ph)
    {
        td_text(1, 0, "Not enough memory for the canvas.", th->win_fg, th->win_bg, 0);
        return;
    }

    if (s_mode == MODE_PLASMA)
        draw_plasma(s_canvas, s_t);
    else
        draw_palette(s_canvas);
    td_canvas_draw(s_canvas, 0, 0);

    s_frames++;
    char line[96];
    snprintf(line, sizeof(line), " %s%s  %d fps  %u KB/s  %dx%d px   Space: next  P: pause",
             s_mode == MODE_PLASMA ? "Plasma" : "Palette", s_paused ? " (paused)" : "", s_fps,
             (unsigned)(td_stats()->bytes_per_sec / 1024u), pw, ph);
    td_fill(td_rect(0, ch - 1, cw, 1), ' ', th->win_fg, th->win_bg);
    td_textfit(0, ch - 1, line, cw, th->dim, th->win_bg, 0);
}

static void on_tick(td_window_t *win)
{
    uint32_t now = td_millis();
    if (now - s_second_ms >= 1000u)
    {
        s_fps = s_frames;
        s_frames = 0;
        s_second_ms = now;
        td_win_invalidate(win); /* the status line, also while paused */
    }
    if (!s_paused && s_mode == MODE_PLASMA)
    {
        s_t++;
        td_win_invalidate(win);
    }
}

static bool on_event(td_window_t *win, const td_event_t *ev)
{
    if (ev->type != TD_EV_KEY)
        return false;
    if (ev->key == ' ')
        s_mode = (s_mode + 1) % MODE_COUNT;
    else if (ev->key == 'p' || ev->key == 'P')
        s_paused = !s_paused;
    else
        return false;
    td_win_invalidate(win);
    return true;
}

static void on_close(td_window_t *win)
{
    (void)win;
    td_canvas_free(s_canvas);
    s_canvas = NULL;
    s_win = NULL;
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    make_tables();
    td_window_desc_t d = {
        .title = "Pixels",
        .rect = td_rect(-1, -1, 44, 18),
        .flags = TD_WIN_DEFAULT,
        .on_draw = on_draw,
        .on_event = on_event,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = TICK_MS,
    };
    s_win = td_win_create(&d);
    s_mode = MODE_PLASMA; /* every opening starts animated */
    s_paused = false;
    s_frames = s_fps = 0;
    s_second_ms = td_millis();
}

static const td_app_t s_app = {"Pixels", launch, "\xE2\x96\x80\xE2\x96\x84"}; /* ▀▄ */

void td_pixels_register(void)
{
    td_app_register(&s_app);
}
