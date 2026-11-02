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

#include "td_apps.h"
#include "tinydesk/td.h"
#include "tinydesk/td_sysinfo.h"
#include "tinydesk/td_widgets.h"
#include "tinydesk/td_wm.h"

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

/* ----------------------------------------------------- the corner hold */

static bool s_hold, s_down;
static uint32_t s_hold_ms, s_release_ms;

static bool in_corner(int x, int y)
{
    return x >= td_stats()->cols - CORNER_W && y < CORNER_H;
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

#define KEY_W 7
#define KEY_H 3
#define KB_W  (10 * KEY_W + 4)
#define KB_H  22

enum
{
    K_CHAR,
    K_SHIFT,
    K_SYM,
    K_BACK,
    K_CANCEL,
    K_OK
};

typedef struct
{
    int x, y, w;                  /* cells, in the client area */
    int kind;
    char ch;
} kbkey_t;

static const char *const s_rows_lower[4] = {"1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm"};
static const char *const s_rows_upper[4] = {"1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM"};
static const char *const s_rows_sym[4] = {"1234567890", "!@#$%^&*()", "-_=+[]{};:", "'\",.<>/?\\|"};

static td_window_t *s_kb;
static char s_pw[PW_MAX + 1];
static bool s_shift, s_sym;
static char s_note[48];
static int s_fails;
static uint32_t s_pause_until;
static kbkey_t s_keys[48];
static int s_key_count;
static int s_pressed = -1;

static void layout(void)
{
    const char *const *rows = s_sym ? s_rows_sym : (s_shift ? s_rows_upper : s_rows_lower);
    s_key_count = 0;
    for (int r = 0; r < 4; r++)
    {
        int n = (int)strlen(rows[r]);
        int x0 = 2 + (10 - n) * KEY_W / 2;
        for (int i = 0; i < n; i++)
            s_keys[s_key_count++] = (kbkey_t){x0 + i * KEY_W, 5 + r * KEY_H, KEY_W - 1, K_CHAR, rows[r][i]};
    }
    int y = 5 + 4 * KEY_H;
    s_keys[s_key_count++] = (kbkey_t){2, y, 10, K_SHIFT, 0};
    s_keys[s_key_count++] = (kbkey_t){13, y, 10, K_SYM, 0};
    s_keys[s_key_count++] = (kbkey_t){24, y, 13, K_BACK, 0};
    s_keys[s_key_count++] = (kbkey_t){38, y, 15, K_CANCEL, 0};
    s_keys[s_key_count++] = (kbkey_t){54, y, 18, K_OK, 0};
}

static const char *key_label(const kbkey_t *k, char *buf)
{
    switch (k->kind)
    {
    case K_SHIFT:
        return s_shift ? "SHIFT*" : "Shift";
    case K_SYM:
        return s_sym ? "abc" : "#+=";
    case K_BACK:
        return "\xE2\x86\x90 Del";     /* U+2190 */
    case K_CANCEL:
        return "Cancel";
    case K_OK:
        return "Unlock";
    default:
        buf[0] = k->ch;
        buf[1] = '\0';
        return buf;
    }
}

static void kb_draw(td_window_t *win, int w, int h)
{
    (void)win;
    (void)h;
    const td_theme_t *t = td_theme();
    td_text(2, 0, "Root's password leaves kiosk mode until the next restart.", t->win_fg, t->win_bg, 0);
    td_fill(td_rect(2, 2, w - 4, 1), ' ', t->input_fg, t->input_bg);
    for (int i = 0; s_pw[i] && i < w - 6; i++)
        td_putc(3 + i, 2, '*', t->input_fg, t->input_bg, 0);
    td_text(2, 3, s_note, t->accent, t->win_bg, TD_BOLD);
    for (int i = 0; i < s_key_count; i++)
    {
        const kbkey_t *k = &s_keys[i];
        bool hot = (i == s_pressed);
        uint8_t fg = hot ? t->select_fg : t->button_fg, bg = hot ? t->select_bg : t->button_bg;
        td_fill(td_rect(k->x, k->y, k->w, KEY_H - 1), ' ', fg, bg);
        char one[2];
        const char *label = key_label(k, one);
        int len = td_utf8_len(label);
        td_text(k->x + (k->w - len) / 2, k->y + (KEY_H - 1) / 2, label, fg, bg, TD_BOLD);
    }
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

static void press_key(const kbkey_t *k)
{
    size_t n = strlen(s_pw);
    s_note[0] = '\0';
    switch (k->kind)
    {
    case K_CHAR:
        if (n < PW_MAX)
            s_pw[n] = k->ch;
        if (s_shift && !s_sym)
            s_shift = false;
        break;
    case K_SHIFT:
        s_shift = !s_shift;
        s_sym = false;
        break;
    case K_SYM:
        s_sym = !s_sym;
        break;
    case K_BACK:
        if (n > 0)
            s_pw[n - 1] = '\0';
        break;
    case K_CANCEL:
        kb_close();
        return;
    case K_OK:
        unlock();
        break;
    }
    layout();
}

static bool kb_event(td_window_t *win, const td_event_t *ev)
{
    if (ev->type == TD_EV_MOUSE)
    {
        int hit = -1;
        for (int i = 0; i < s_key_count; i++)
            if (ev->x >= s_keys[i].x && ev->x < s_keys[i].x + s_keys[i].w && ev->y >= s_keys[i].y &&
                ev->y < s_keys[i].y + KEY_H - 1)
                hit = i;
        if (ev->action == TD_MOUSE_PRESS && ev->button == TD_BUTTON_LEFT)
            s_pressed = hit;
        else if (ev->action == TD_MOUSE_RELEASE)
        {
            int was = s_pressed;
            s_pressed = -1;
            if (was >= 0 && was == hit)
                press_key(&s_keys[was]);   /* may close the window */
        }
        if (td_win_is_open(win))
            td_win_invalidate(win);
        return true;
    }
    if (ev->type == TD_EV_KEY)
    {
        /* A keyboard (Telnet, SSH) works too. */
        size_t n = strlen(s_pw);
        if (ev->key == TD_KEY_ENTER)
            unlock();
        else if (ev->key == TD_KEY_ESC)
            kb_close();
        else if (ev->key == TD_KEY_BACKSPACE && n > 0)
            s_pw[n - 1] = '\0';
        else if (ev->key >= 32 && ev->key < 127 && n < PW_MAX && !(ev->mods & (TD_MOD_CTRL | TD_MOD_ALT)))
            s_pw[n] = (char)ev->key;
        if (td_win_is_open(win))
            td_win_invalidate(win);
        return true;
    }
    return false;
}

static void kb_closed(td_window_t *win)
{
    (void)win;
    memset(s_pw, 0, sizeof(s_pw));
    s_kb = NULL;
}

static void open_unlock(void)
{
    if (td_win_is_open(s_kb))
        return;
    memset(s_pw, 0, sizeof(s_pw));
    s_shift = s_sym = false;
    s_note[0] = '\0';
    s_pressed = -1;
    layout();
    td_window_desc_t d = {
        .title = "Leave kiosk mode",
        .rect = td_rect(-1, -1, KB_W + 2, KB_H),
        .flags = TD_WIN_MODAL | TD_WIN_RAW_KEYS,
        .on_draw = kb_draw,
        .on_event = kb_event,
        .on_close = kb_closed,
    };
    s_kb = td_win_create(&d);
}

/* ----------------------------------------------------- the app itself */

static void status_draw(td_window_t *win, int w, int h)
{
    (void)win;
    const td_theme_t *t = td_theme();
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

void td_kiosk_start(void)
{
    const char *app = td_sysinfo()->kiosk_app;
    if (s_active || !app || !app[0])
        return;
    snprintf(s_cmd, sizeof(s_cmd), "py %s", app);
    s_active = true;
    s_idle_ms = td_millis();
    td_wm_set_kiosk(true, kiosk_hook);
    td_window_desc_t d = {
        .title = "Kiosk",
        .rect = td_rect(0, 0, -1, -1),
        .on_draw = status_draw,
    };
    s_status = td_win_create(&d);
    td_timer_start(CHECK_MS, true, check, NULL, td_millis());
    td_logf('I', "kiosk mode: %s", app);
}
