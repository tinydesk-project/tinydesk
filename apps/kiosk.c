/*
 * kiosk.c - kiosk mode: one Python app on the whole screen.
 *
 * When the port names a kiosk app (td_sysinfo()->kiosk_app, from the board
 * configuration key kiosk.app on the ESP32 boards), the desktop starts in
 * kiosk mode (td_wm_set_kiosk: no taskbar, Start menu, desktop icons or
 * window frames) and types `py <app>` into the shell. The app's tdui
 * windows cover the screen; when it ends, it is started again.
 *
 * The way out for a technician: hold a finger (or a mouse button) on the
 * top-right corner for 5 seconds, or, from a keyboard, press F10 three
 * times within 2 seconds or Ctrl+Alt+Delete (a USB keyboard; a PC keeps
 * that one for itself). An on-screen keyboard asks for root's password;
 * the right one leaves kiosk mode until the next restart (the app keeps
 * running, in a normal window).
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "td_apps.h"
#include "tinydesk/td.h"
#include "tinydesk/td_sysinfo.h"
#include "tinydesk/td_widgets.h"
#include "tinydesk/td_wm.h"
#include "tinydesk/td_picture.h"

#define FIRST_START_MS 1500      /* let the shell come up first */
#define RESTART_MS     5000      /* an app that ended is started again after this */
#define CHECK_MS       100
#define HOLD_MS        5000      /* corner hold that asks for the password */
#define HOLD_GAP_MS    150       /* a release and press this close count as one hold */
#define F10_TIMES      3         /* F10 pressed this often ... */
#define F10_WITHIN_MS  2000      /* ... within this long asks for the password */
#define CORNER_W       4         /* the top-right corner: cells from the right edge */
#define CORNER_H       2
#define PW_MAX         32
#define FAIL_MAX       3         /* wrong passwords before a pause */
#define FAIL_PAUSE_MS  30000

static bool s_active;
static char s_cmd[TD_PATH_MAX + 8];
static uint32_t s_launch_ms, s_idle_ms;
static bool s_launched;
static td_window_t *s_status;     /* behind the app: what kiosk mode is doing */
static uint8_t *s_splash;
static bool s_splash_rgb;
static int s_splash_w, s_splash_h;

static void load_splash(void)
{
    const char *path = td_sysinfo()->kiosk_splash;
    if (!path || !path[0] || !td_sysinfo()->fs)
        return;
    const td_fs_ops_t *fs = td_sysinfo()->fs;
    char resolved[TD_PATH_MAX];
    /* board.conf uses shell paths (/root/...), while the native file
     * reader needs /fs/root/... on ESP32, or the host sandbox root. */
    const char *root = fs->root;
    size_t n = root ? strlen(root) : 0;
    if (n && (strncmp(path, root, n) || (path[n] && path[n] != '/')))
    {
        int length = snprintf(resolved, sizeof(resolved), "%s%s%s", root, path[0] == '/' ? "" : "/", path);
        if (length < 0 || length >= (int)sizeof(resolved))
            return;
        path = resolved;
    }
    td_image_t image;
    char error[96];
    if (!td_image_load(&image, fs, path, error, sizeof(error)))
    {
        td_logf('E', "kiosk artwork: %s", error);
        return;
    }
    if (image.w < 1 || image.h < 1 || (long)image.w * image.h > 800L * 480L)
    {
        td_image_close(&image);
        return;
    }
    bool rgb = TD_PYUI_MAX_CELLS > 2000;
    uint8_t *pixels = malloc((size_t)image.w * image.h * (rgb ? 2 : 1));
    uint8_t *row = malloc((size_t)image.w * 3);
    int y, count = 0;
    if (pixels && row)
        while (td_image_row(&image, &y, row))
        {
            for (int x = 0; x < image.w; x++)
            {
                if (rgb)
                    ((uint16_t *)pixels)[y * image.w + x] = td_rgb565(row[x * 3], row[x * 3 + 1], row[x * 3 + 2]);
                else
                    pixels[y * image.w + x] = td_rgb256(row[x * 3], row[x * 3 + 1], row[x * 3 + 2]);
            }
            count++;
        }
    free(row);
    if (pixels && count == image.h)
    {
        s_splash = pixels;
        s_splash_rgb = rgb;
        s_splash_w = image.w;
        s_splash_h = image.h;
    }
    else
        free(pixels);
    td_image_close(&image);
}

static void splash_pixels(void *user, uint8_t *pixels, int w, int h)
{
    (void)user;
    memset(pixels, 0, (size_t)w * h);
    int dw = s_splash_w, dh = s_splash_h;
    if (dw > w)
    {
        dh = dh * w / dw;
        dw = w;
    }
    if (dh > h)
    {
        dw = dw * h / dh;
        dh = h;
    }
    if (dw < 1 || dh < 1)
        return;
    int ox = (w - dw) / 2, oy = (h - dh) / 2;
    for (int y = 0; y < dh; y++)
        for (int x = 0; x < dw; x++)
        {
            int at = (y * s_splash_h / dh) * s_splash_w + x * s_splash_w / dw;
            uint8_t color = s_splash[at];
            if (s_splash_rgb)
            {
                uint8_t rgb[3];
                td_rgb565_rgb(((uint16_t *)s_splash)[at], rgb);
                color = td_rgb256(rgb[0], rgb[1], rgb[2]);
            }
            pixels[(y + oy) * w + x + ox] = color;
        }
}

static void splash_pixels_rgb(void *user, uint16_t *pixels, int w, int h)
{
    (void)user;
    memset(pixels, 0, (size_t)w * h * sizeof(*pixels));
    int dw = s_splash_w, dh = s_splash_h;
    if (dw > w)
    {
        dh = dh * w / dw;
        dw = w;
    }
    if (dh > h)
    {
        dw = dw * h / dh;
        dh = h;
    }
    if (dw < 1 || dh < 1)
        return;
    int ox = (w - dw) / 2, oy = (h - dh) / 2;
    for (int y = 0; y < dh; y++)
        for (int x = 0; x < dw; x++)
        {
            int at = (y * s_splash_h / dh) * s_splash_w + x * s_splash_w / dw;
            pixels[(y + oy) * w + x + ox] = s_splash_rgb ? ((uint16_t *)s_splash)[at] : td_palette_rgb565(s_splash[at]);
        }
}

/* ----------------------------------------------------- the corner hold */

static bool s_hold, s_down;
static uint32_t s_hold_ms, s_release_ms;

static bool in_corner(int x, int y)
{
    td_rect_t m = td_wm_main_monitor();   /* the main display's corner */
    return x >= m.x + m.w - CORNER_W && x < m.x + m.w && y >= m.y && y < m.y + CORNER_H;
}

static void open_unlock(void);

/* F10 times, oldest first (the window manager's F10 does nothing here). */
static uint32_t s_f10_ms[F10_TIMES];

static bool key_hook(const td_event_t *ev)
{
    if (ev->key == TD_KEY_DELETE && ev->mods == (TD_MOD_CTRL | TD_MOD_ALT))
    {
        open_unlock();
        return true;
    }
    if (ev->key != TD_KEY_F10 || ev->mods != 0)
        return false;
    uint32_t now = td_millis();
    memmove(s_f10_ms, s_f10_ms + 1, sizeof(s_f10_ms) - sizeof(s_f10_ms[0]));
    s_f10_ms[F10_TIMES - 1] = now;
    if (s_f10_ms[0] && now - s_f10_ms[0] <= F10_WITHIN_MS)
    {
        memset(s_f10_ms, 0, sizeof(s_f10_ms));
        open_unlock();
    }
    return true;
}

static bool kiosk_hook(const td_event_t *ev)
{
    if (ev->type == TD_EV_KEY)
        return key_hook(ev);
    if (ev->type != TD_EV_MOUSE)
        return false;
    uint32_t now = td_millis();
    if (ev->action == TD_MOUSE_PRESS)
    {
        if (in_corner(ev->x, ev->y))
        {
            /* A long touch turns into a right button: the same hold. */
            if (!s_hold || s_down || now - s_release_ms > HOLD_GAP_MS)
                s_hold_ms = now;
            s_hold = true;
        }
        else
        {
            s_hold = false;
        }
        s_down = true;
    }
    else if (ev->action == TD_MOUSE_RELEASE)
    {
        s_down = false;
        s_release_ms = now;
    }
    else if (ev->action == TD_MOUSE_DRAG && !in_corner(ev->x, ev->y))
    {
        s_hold = false;
    }
    return false;                 /* the app sees every event too */
}

/* ------------------------------------------------- the unlock keyboard */

/* A password box at the top; the On-screen keyboard (apps/osk.c, the one
 * keyboard) opens below it and types into it as a keyboard would. */
static td_window_t *s_kb;
static char s_pw[PW_MAX + 1];
static char s_note[128];
static int s_fails;
static uint32_t s_pause_until;

static void kb_draw(td_window_t *win, int w, int h)
{
    (void)win;
    const td_theme_t *t = td_theme();
    td_text(2, 0, "Root's password leaves kiosk mode until the next restart.", t->win_fg, t->win_bg, 0);
    td_fill(td_rect(2, 1, w - 4, 1), ' ', t->input_fg, t->input_bg);
    for (int i = 0; s_pw[i] && i < w - 6; i++)
        td_putc(3 + i, 1, '*', t->input_fg, t->input_bg, 0);
    td_textfit(2, 2, s_note, w - 4, t->accent, t->win_bg, TD_BOLD);
    (void)h;
}

static void kb_close(void)
{
    memset(s_pw, 0, sizeof(s_pw));
    if (td_win_is_open(s_kb))
        td_win_close(s_kb);
    s_kb = NULL;
}

static void unlock(void)
{
    uint32_t now = td_millis();
    if (s_fails >= FAIL_MAX && (int32_t)(s_pause_until - now) > 0)
    {
        snprintf(s_note, sizeof(s_note), "Too many tries: wait %u s.", (unsigned)((s_pause_until - now) / 1000 + 1));
        return;
    }
    const td_sysinfo_t *si = td_sysinfo();
    bool ok = !si->authenticate || si->authenticate("root", s_pw);
    if (!ok)
    {
        if (++s_fails >= FAIL_MAX)
            s_pause_until = now + FAIL_PAUSE_MS;
        snprintf(s_note, sizeof(s_note), "Wrong password.");
        memset(s_pw, 0, sizeof(s_pw));
        td_logf('W', "kiosk: wrong password for leaving kiosk mode");
        return;
    }
    s_fails = 0;
    kb_close();
    s_active = false;
    td_wm_set_kiosk(false, NULL);
    if (td_win_is_open(s_status))
        td_win_close(s_status);
    td_logf('I', "kiosk: left kiosk mode (until the next restart)");
}

/* A key, from the touch keys or a keyboard (Telnet, SSH). */
static void type_key(int code)
{
    size_t n = strlen(s_pw);
    s_note[0] = '\0';
    if (code == TD_KEY_ENTER)
        unlock();
    else if (code == TD_KEY_ESC)
        kb_close();
    else if (code == TD_KEY_BACKSPACE && n > 0)
        s_pw[n - 1] = '\0';
    else if (code >= 32 && code < 127 && n < PW_MAX)
        s_pw[n] = (char)code;
}

static bool kb_event(td_window_t *win, const td_event_t *ev)
{
    if (ev->type == TD_EV_MOUSE)
        return true;   /* nothing to click: the keys are the keyboard's */
    if (ev->type == TD_EV_KEY)
    {
        if (!(ev->mods & (TD_MOD_CTRL | TD_MOD_ALT)))
            type_key((int)ev->key);
    }
    else
        return false;
    if (td_win_is_open(win))
        td_win_invalidate(win);
    return true;
}

static void kb_closed(td_window_t *win)
{
    (void)win;
    memset(s_pw, 0, sizeof(s_pw));
    s_kb = NULL;
    td_osk_show(false);
}

static void open_unlock(void)
{
    if (td_win_is_open(s_kb))
        return;
    memset(s_pw, 0, sizeof(s_pw));
    s_note[0] = '\0';
    const td_stats_t *st = td_stats();
    int w = st->cols - 4 < 70 ? st->cols - 4 : 70;
    td_window_desc_t d = {
        .title = "Leave kiosk mode",
        .rect = td_rect(-1, 0, w, 5),   /* at the top: the keyboard goes below */
        .flags = TD_WIN_MODAL | TD_WIN_RAW_KEYS,
        .on_draw = kb_draw,
        .on_event = kb_event,
        .on_close = kb_closed,
    };
    s_kb = td_win_create(&d);
    if (s_kb)
        td_osk_show(true);
}

/* ----------------------------------------------------- the app itself */

static void status_draw(td_window_t *win, int w, int h)
{
    const td_theme_t *t = td_theme();
    if (s_splash || td_sysinfo()->kiosk_hide_status)
    {
        td_fill(td_rect(0, 0, w, h), ' ', 231, 0);
        /* Pictures are emitted after cell composition. Loading artwork
         * belongs only to this screen, never underneath an app or dialog. */
        if (td_win_focused() != win)
            return;
        if (s_splash && td_term_images())
        {
            int cw, ch;
            td_term_cell_px(&cw, &ch);
            int cols = (s_splash_w + cw - 1) / cw, rows = (s_splash_h + ch - 1) / ch;
            if (cols > w)
                cols = w;
            if (rows > h)
                rows = h;
            td_rect_t client = td_win_client(win);
            if (s_splash_rgb)
                td_picture_add_rgb(client.x + (w - cols) / 2, client.y + (h - rows) / 2, cols, rows, 0x53504c31u, splash_pixels, splash_pixels_rgb, NULL);
            else
                td_picture_add(client.x + (w - cols) / 2, client.y + (h - rows) / 2, cols, rows,
                               0x53504c31u, splash_pixels, NULL);
        }
        return;
    }
    char line[TD_TEXT_MAX * 2];
    const char *app = td_sysinfo()->kiosk_app;
    if (!s_launched)
        snprintf(line, sizeof(line), "Starting %s ...", app);
    else
        snprintf(line, sizeof(line), "%s is not running: it starts again in a few seconds.", app);
    int len = td_utf8_len(line);
    td_text((w - len) / 2 > 0 ? (w - len) / 2 : 0, h / 2, line, t->win_fg, t->win_bg, TD_BOLD);
}

static void launch(void)
{
    td_terminal_command(s_cmd);
    s_launch_ms = td_millis();
    s_launched = true;
    td_logf('I', "kiosk: %s", s_cmd);
}

static void check(void *user)
{
    (void)user;
    if (!s_active)
        return;
    uint32_t now = td_millis();
    if (s_hold && s_down && now - s_hold_ms >= HOLD_MS)
    {
        s_hold = false;
        open_unlock();
    }
    if (td_pyui_running())
    {
        s_idle_ms = now;
        return;
    }
    uint32_t wait = s_launched ? RESTART_MS : FIRST_START_MS;
    if (now - s_idle_ms >= wait && (!s_launched || now - s_launch_ms >= RESTART_MS))
    {
        launch();
        if (td_win_is_open(s_status))
            td_win_invalidate(s_status);
    }
}

bool td_kiosk_active(void)
{
    return s_active;
}
static int s_check_timer = -1;
void td_kiosk_stop_for_login(void)
{
    s_active = false;
    s_launched = false;
    td_wm_set_kiosk(false, NULL);
    if (td_win_is_open(s_status))
        td_win_close(s_status);
    s_status = NULL;
}

void td_kiosk_start(void)
{
    const char *app = td_sysinfo()->kiosk_app;
    if (!td_session_has_right(TD_RIGHT_APPS) || s_active || !app || !app[0])
        return;
    if (!td_session_is_root() && !strncmp(app, "/root/", 6))
        return;
    snprintf(s_cmd, sizeof(s_cmd), "py %s", app);
    s_active = true;
    load_splash();
    s_idle_ms = td_millis();
    td_wm_set_kiosk(true, kiosk_hook);
    td_window_desc_t d = {
        .title = "Kiosk",
        .rect = td_rect(0, 0, -1, -1),
        .on_draw = status_draw,
    };
    s_status = td_win_create(&d);
    if (s_check_timer < 0)
        s_check_timer = td_timer_start(CHECK_MS, true, check, NULL, td_millis());
    td_logf('I', "kiosk mode: %s", app);
}
