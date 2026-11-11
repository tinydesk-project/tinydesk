/*
 * imageview.c - the Image Viewer: BMP and PPM/PGM files (imagefile.c) shown
 * on a pixel canvas (td_canvas.h), scaled to fit the window.
 *
 *   D  dithering on / off      R  load again      O  open another file
 *
 * Opened from Files or the desktop (double-click an image), or from the
 * Start menu. Decoding runs a slice at a time from the window's timer, so
 * the desktop stays responsive and a large image appears from top to
 * bottom; shrinking averages every source pixel into its target pixel.
 * Memory while open: the canvas, the fitted image (3 bytes per pixel shown),
 * one image row (3 bytes per pixel) and the file window (imagefile.c).
 */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "td_apps.h"
#include "tinydesk/td_canvas.h"

#define TICK_MS   20
#define BUDGET_MS 15 /* decoding per tick */

static td_window_t *s_win;
static td_canvas_t *s_canvas;
static td_image_t *s_img;      /* NULL: no image loaded */
static char s_path[TD_PATH_MAX];
static char s_status[96];      /* an error, or empty */
static bool s_dither = true;

/* The current decode: the image fitted into dw x dh pixels at (ox, oy). */
static bool s_busy;
static int s_dw, s_dh, s_ox, s_oy;
static int s_aspect = 1;       /* canvas pixels per picture pixel across: 2 in quadrant detail */
static uint8_t *s_rgb;         /* dw * dh * 3, the fitted image */
static uint8_t *s_row;         /* one image row, w * 3 */
static uint32_t *s_acc;        /* shrinking: dw * 4 sums (r, g, b, count) */
static int s_acc_row;          /* target row s_acc belongs to, -1 none */
static int s_rows_done;

static const char *base_name(const char *p)
{
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

/* Dithering mixes neighbouring palette colours, so gradients look smoother
 * than with the nearest colour alone. */
static uint8_t quantize(int x, int y, const uint8_t *rgb)
{
    return s_dither ? td_rgb256_dither(rgb[0], rgb[1], rgb[2], x, y) : td_rgb256(rgb[0], rgb[1], rgb[2]);
}

/* Target row ty is complete: onto the canvas. */
static void show_row(int ty)
{
    for (int tx = 0; tx < s_dw; ++tx)
        td_canvas_pixel(s_canvas, s_ox + tx, s_oy + ty, quantize(tx, ty, s_rgb + (ty * s_dw + tx) * 3));
}

static void flush_acc(void)
{
    if (s_acc_row < 0)
        return;
    uint8_t *out = s_rgb + s_acc_row * s_dw * 3;
    for (int tx = 0; tx < s_dw; ++tx)
    {
        uint32_t *a = s_acc + tx * 4;
        uint32_t n = a[3] ? a[3] : 1;
        for (int c = 0; c < 3; ++c)
            out[tx * 3 + c] = (uint8_t)(a[c] / n);
    }
    show_row(s_acc_row);
    memset(s_acc, 0, (size_t)s_dw * 4 * sizeof(uint32_t));
    s_acc_row = -1;
}

static void free_decode(void)
{
    free(s_rgb);
    free(s_row);
    free(s_acc);
    s_rgb = s_row = NULL;
    s_acc = NULL;
    s_busy = false;
}

/* Fit the image into the canvas and start decoding it from the top of the
 * file. */
static void start_decode(void)
{
    free_decode();
    if (!s_img || !s_canvas)
        return;
    /* A quadrant pixel is half as wide as it is tall: the picture is
     * s_aspect times as many pixels wide. */
    int cw = s_canvas->w, ch = s_canvas->h, iw = s_img->w * s_aspect, ih = s_img->h;
    if ((long)iw * ch <= (long)ih * cw)
    {
        s_dh = ch;
        s_dw = (int)((long)iw * ch / ih);
    }
    else
    {
        s_dw = cw;
        s_dh = (int)((long)ih * cw / iw);
    }
    s_dw = s_dw < 1 ? 1 : s_dw;
    s_dh = s_dh < 1 ? 1 : s_dh;
    s_ox = (cw - s_dw) / 2;
    s_oy = (ch - s_dh) / 2;
    s_rgb = calloc((size_t)s_dw * (size_t)s_dh, 3);
    s_row = malloc((size_t)s_img->w * 3);
    s_acc = calloc((size_t)s_dw * 4, sizeof(uint32_t));
    if (!s_rgb || !s_row || !s_acc)
    {
        free_decode();
        snprintf(s_status, sizeof(s_status), "Not enough memory to show the image.");
        return;
    }
    td_canvas_clear(s_canvas, td_theme()->win_bg);
    td_image_rewind(s_img);
    s_acc_row = -1;
    s_rows_done = 0;
    s_busy = true;
}

static void add(uint32_t *a, const uint8_t *rgb)
{
    a[0] += rgb[0];
    a[1] += rgb[1];
    a[2] += rgb[2];
    a[3]++;
}

/* One picture row into s_acc, across: shrinking adds every pixel into its
 * target pixel, enlarging gives every target pixel its nearest one. */
static void add_row(const uint8_t *rgb)
{
    int iw = s_img->w;
    if (s_dw < iw)
        for (int x = 0; x < iw; ++x)
            add(s_acc + ((long)x * s_dw / iw) * 4, rgb + x * 3);
    else
        for (int tx = 0; tx < s_dw; ++tx)
            add(s_acc + tx * 4, rgb + ((long)tx * iw / s_dw) * 3);
}

/* One picture row (row y) into the target rows it covers. Each direction
 * shrinks or enlarges on its own (quadrant pixels are not square). */
static void take_row(int y, const uint8_t *rgb)
{
    int ih = s_img->h;
    if (s_dh < ih)
    {
        /* Shrinking: the rows of one target row add up in s_acc. */
        int ty = (int)((long)y * s_dh / ih);
        if (ty != s_acc_row)
        {
            flush_acc();
            s_acc_row = ty;
        }
        add_row(rgb);
        return;
    }
    /* Enlarging: the row is repeated over the target rows nearest to it. */
    int first = (int)(((long)y * s_dh + ih - 1) / ih);
    int last = (int)(((long)(y + 1) * s_dh + ih - 1) / ih) - 1;
    if (last >= s_dh)
        last = s_dh - 1;
    if (first > last)
        return;
    add_row(rgb);
    s_acc_row = first;
    flush_acc();
    for (int ty = first + 1; ty <= last; ++ty)
    {
        memcpy(s_rgb + ty * s_dw * 3, s_rgb + first * s_dw * 3, (size_t)s_dw * 3);
        show_row(ty);
    }
}

static void decode_some(void)
{
    uint32_t start = td_millis();
    int y;
    while (s_busy && td_millis() - start < BUDGET_MS)
    {
        if (!td_image_row(s_img, &y, s_row))
        {
            flush_acc();
            if (s_rows_done < s_img->h)
                snprintf(s_status, sizeof(s_status), "The file ends early (row %d of %d).", s_rows_done, s_img->h);
            s_busy = false;
            free(s_row);
            free(s_acc);
            s_row = NULL;
            s_acc = NULL;
            break;
        }
        take_row(y, s_row);
        s_rows_done++;
    }
}

/* Free the image and everything decoded from it. */
static void unload(void)
{
    if (s_img)
    {
        td_image_close(s_img);
        free(s_img);
        s_img = NULL;
    }
    free_decode();
}

static void load(const char *path)
{
    unload();
    s_status[0] = '\0';
    snprintf(s_path, sizeof(s_path), "%s", path ? path : "");
    char name[TD_TITLE_MAX - 15], title[TD_TITLE_MAX];
    td_utf8_copy(name, sizeof(name), path ? base_name(path) : "", INT_MAX); /* never half a character */
    snprintf(title, sizeof(title), "%s - Image Viewer", name);
    td_win_set_title(s_win, path ? title : "Image Viewer");
    if (s_canvas)
        td_canvas_clear(s_canvas, td_theme()->win_bg);
    if (!path)
        return;
    s_img = malloc(sizeof(*s_img));
    if (!s_img)
    {
        snprintf(s_status, sizeof(s_status), "Not enough memory.");
        return;
    }
    if (!td_image_load(s_img, td_sysinfo()->fs, path, s_status, (int)sizeof(s_status)))
    {
        free(s_img);
        s_img = NULL;
        return;
    }
    start_decode();
}

static void on_draw(td_window_t *win, int cw, int ch)
{
    (void)win;
    const td_theme_t *th = td_theme();
    int pw, ph;
    td_canvas_fit(cw, ch - 1, &pw, &ph); /* the last row is the status line */
    if (pw < 2 || ph < 2)
        return;
    if (!s_canvas || s_canvas->w != pw || s_canvas->h != ph)
    {
        s_aspect = pw / cw;
        if (!s_canvas)
            s_canvas = td_canvas_new(pw, ph);
        else if (!td_canvas_resize(s_canvas, pw, ph))
        {
            td_canvas_free(s_canvas);
            s_canvas = NULL;
        }
        if (s_canvas)
        {
            td_canvas_clear(s_canvas, th->win_bg);
            start_decode(); /* new size: fit again */
        }
    }
    if (!s_canvas)
    {
        td_text(1, 0, "Not enough memory for the picture.", th->win_fg, th->win_bg, 0);
        return;
    }
    td_canvas_draw(s_canvas, 0, 0);

    char line[160];
    if (s_status[0])
        snprintf(line, sizeof(line), " %s  O: open", s_status);
    else if (!s_img)
        snprintf(line, sizeof(line), " Open a .bmp, .ppm or .pgm file in Files or on the desktop, or press O.");
    else if (s_busy)
        snprintf(line, sizeof(line), " %dx%d %s  loading %d%%", s_img->w, s_img->h, s_img->format,
                 s_rows_done * 100 / s_img->h);
    else
        snprintf(line, sizeof(line), " %dx%d %s at %dx%d  D: dither %s  R: reload  O: open", s_img->w,
                 s_img->h, s_img->format, s_dw, s_dh, s_dither ? "on" : "off");
    td_fill(td_rect(0, ch - 1, cw, 1), ' ', th->win_fg, th->win_bg);
    td_textfit(0, ch - 1, line, cw, th->dim, th->win_bg, 0);
}

static void on_tick(td_window_t *win)
{
    if (!s_busy)
        return;
    decode_some();
    td_win_invalidate(win);
}

static void open_answer(const char *text, void *user)
{
    (void)user;
    char real[TD_PATH_MAX];
    if (!td_session_real_path(text, real, sizeof(real)))
    {
        snprintf(s_status, sizeof(s_status), "That path cannot be opened.");
        td_win_invalidate(s_win);
        return;
    }
    load(real);
    td_win_invalidate(s_win);
}

static bool on_event(td_window_t *win, const td_event_t *ev)
{
    if (ev->type != TD_EV_KEY)
        return false;
    switch (ev->key)
    {
    case 'd':
    case 'D':
        s_dither = !s_dither;
        if (s_rgb && !s_busy)
        {
            for (int ty = 0; ty < s_dh; ++ty)
                show_row(ty);
        }
        break;
    case 'r':
    case 'R':
        if (s_path[0])
        {
            char again[TD_PATH_MAX];
            snprintf(again, sizeof(again), "%s", s_path);
            load(again);
        }
        break;
    case 'o':
    case 'O':
    {
        /* Start in the folder of the picture shown: type only a name. */
        char dir[TD_PATH_MAX] = "~/";
        if (s_path[0])
        {
            snprintf(dir, sizeof(dir), "%s", td_shell_path(s_path));
            char *slash = strrchr(dir, '/');
            if (slash)
                slash[1] = '\0';
        }
        td_inputbox("Open image", "File (~/ is your home):", dir, open_answer, NULL);
        break;
    }
    default:
        return false;
    }
    td_win_invalidate(win);
    return true;
}

static void on_close(td_window_t *win)
{
    (void)win;
    unload();
    td_canvas_free(s_canvas);
    s_canvas = NULL;
    s_win = NULL;
}

void td_imageview_open(const char *path)
{
    if (!td_win_is_open(s_win))
    {
        td_window_desc_t d = {
            .title = "Image Viewer",
            .rect = td_rect(-1, -1, 60, 20),
            .flags = TD_WIN_DEFAULT,
            .on_draw = on_draw,
            .on_event = on_event,
            .on_close = on_close,
            .on_tick = on_tick,
            .tick_ms = TICK_MS,
        };
        s_win = td_win_create(&d);
        if (!s_win)
            return;
    }
    td_win_focus(s_win);
    if (path)
        load(path);
    else if (!s_img)
        load(NULL);
    td_win_invalidate(s_win);
}

static void launch(void)
{
    td_imageview_open(NULL);
}

static const td_app_t s_app = {"Image Viewer", launch, "\xE2\x96\x93\xE2\x96\x92"}; /* ▓▒ */

void td_imageview_register(void)
{
    td_app_register(&s_app);
}
