/*
 * terminal.c - a window that runs a text program (normally the embedded
 * TinyDesk Shell) through a td_term_backend_t supplied by the port.
 *
 * Program output is fed through the td_vterm emulator; key events are turned
 * back into the byte sequences a VT100 terminal would send. The session
 * keeps running when the window is closed and reappears when it is opened
 * again.
 *
 * Keys handled here instead of being passed on:
 *   Shift+PgUp / Shift+PgDn, mouse wheel   scroll back through history
 *   F11                                    full screen (handled by the WM)
 */
#include <stdio.h>
#include <string.h>

#include "td_apps.h"
#include "tinydesk/td_vterm.h"

static const td_term_backend_t *s_backend;
static td_window_t *s_win;
static td_vterm_t s_vt;
static bool s_vt_ready;
static bool s_started;
static int s_scroll;          /* lines scrolled back */
static int s_bg_timer = -1;
static td_window_t *s_viewer;  /* another window showing the session */
static void (*s_tap)(const uint8_t *data, int len);

/* Answers to terminal queries go straight back to the program. */
static void vt_reply(void *user, const char *data, int len)
{
    (void)user;
    if (s_backend && s_backend->write)
        s_backend->write(s_backend->ctx, (const uint8_t *)data, len);
}

static void send(const char *bytes, int len)
{
    if (!s_backend || !s_backend->write || len <= 0)
        return;
    s_backend->write(s_backend->ctx, (const uint8_t *)bytes, len);
}

/* Translate a key event into what a VT100/xterm would send. Returns the
 * byte count written to out (0 = nothing to send). */
static int encode_key(const td_event_t *ev, char *out)
{
    uint32_t k = ev->key;
    uint8_t m = ev->mods;
    int n = 0;

    if (m & TD_MOD_ALT)
        out[n++] = '\x1b';

    if ((m & TD_MOD_CTRL) && k >= 'a' && k <= 'z')
    {
        out[n++] = (char)(k - 'a' + 1);
        return n;
    }
    if (m & TD_MOD_CTRL)
    {
        switch (k)
        {
        case ' ':
            out[n++] = 0;
            return n;
        case '\\':
            out[n++] = 0x1C;
            return n;
        case ']':
            out[n++] = 0x1D;
            return n;
        case '^':
            out[n++] = 0x1E;
            return n;
        case '_':
            out[n++] = 0x1F;
            return n;
        default:
            break;
        }
    }

    const char *seq = NULL;
    switch (k)
    {
    case TD_KEY_ENTER:
        seq = "\r";
        break;
    case TD_KEY_TAB:
        seq = (m & TD_MOD_SHIFT) ? "\x1b[Z" : "\t";
        break;
    case TD_KEY_BACKSPACE:
        seq = "\x7f";
        break;
    case TD_KEY_ESC:
        seq = "\x1b";
        break;
    case TD_KEY_INSERT:
        seq = "\x1b[2~";
        break;
    case TD_KEY_DELETE:
        seq = "\x1b[3~";
        break;
    case TD_KEY_PGUP:
        seq = "\x1b[5~";
        break;
    case TD_KEY_PGDN:
        seq = "\x1b[6~";
        break;
    case TD_KEY_F1:
        seq = "\x1bOP";
        break;
    case TD_KEY_F2:
        seq = "\x1bOQ";
        break;
    case TD_KEY_F3:
        seq = "\x1bOR";
        break;
    case TD_KEY_F4:
        seq = "\x1bOS";
        break;
    case TD_KEY_F5:
        seq = "\x1b[15~";
        break;
    case TD_KEY_F7:
        seq = "\x1b[18~";
        break;
    case TD_KEY_F8:
        seq = "\x1b[19~";
        break;
    case TD_KEY_F9:
        seq = "\x1b[20~";
        break;
    case TD_KEY_F12:
        seq = "\x1b[24~";
        break;
    default:
        break;
    }
    if (seq)
    {
        size_t len = strlen(seq);
        memcpy(out + n, seq, len);
        return n + (int)len;
    }

    /* Cursor keys, with xterm-style modifiers when any are held. */
    const char *letters = "ABCDHF";
    uint32_t cursor_keys[6] = {TD_KEY_UP, TD_KEY_DOWN, TD_KEY_RIGHT, TD_KEY_LEFT, TD_KEY_HOME, TD_KEY_END};
    for (int i = 0; i < 6; i++)
    {
        if (k != cursor_keys[i])
            continue;
        n = 0;   /* modifiers are encoded in the sequence instead */
        int mod = 1 + ((m & TD_MOD_SHIFT) ? 1 : 0) + ((m & TD_MOD_ALT) ? 2 : 0) + ((m & TD_MOD_CTRL) ? 4 : 0);
        if (mod == 1)
            n = sprintf(out, "\x1b[%c", letters[i]);
        else
            n = sprintf(out, "\x1b[1;%d%c", mod, letters[i]);
        return n;
    }

    if (k < TD_KEY_BASE && k >= 0x20)
    {
        uint8_t utf8[4];
        int len = td_utf8_encode(k, utf8);
        memcpy(out + n, utf8, (size_t)len);
        return n + len;
    }
    return 0;
}

static void ensure_vt(int cols, int rows)
{
    if (!s_vt_ready)
    {
        s_vt.reply = vt_reply;
        td_vterm_init(&s_vt, cols, rows);
        s_vt_ready = true;
    }
    if (!s_started && s_backend && s_backend->start)
    {
        s_started = s_backend->start(s_backend->ctx, cols, rows) == 0;
    }
}

/* Pull pending program output into the emulator. */
static void pump(void)
{
    if (!s_backend || !s_backend->read || !s_vt_ready)
        return;
    uint8_t buf[512];
    for (int budget = 0; budget < 16; budget++)
    {   /* at most 8 KB per tick */
        int n = s_backend->read(s_backend->ctx, buf, sizeof(buf));
        if (n <= 0)
            break;
        td_vterm_write(&s_vt, buf, n);
        if (s_tap)
            s_tap(buf, n);
    }
    if (s_vt.dirty)
    {
        s_vt.dirty = false;
        if (td_win_is_open(s_win))
            td_win_invalidate(s_win);
        if (td_win_is_open(s_viewer))
            td_win_invalidate(s_viewer);
    }
}

void td_terminal_pump(void)
{
    pump();
}

/* Keep draining output while the window is closed, via a timer, so a
 * chatty program never blocks on a full pipe. */
static void background_pump(void *user)
{
    (void)user;
    pump();
}

const td_term_backend_t *td_terminal_backend(void)
{
    return s_backend;
}

void td_terminal_reset(void)
{
    if (!s_vt_ready)
        return;
    pump();                            /* drop what the old session printed */
    int cols = s_vt.cols, rows = s_vt.rows;
    td_vterm_init(&s_vt, cols, rows);  /* keeps the reply callback */
    s_scroll = 0;
    td_wm_invalidate();
}

/* Default outer size of the Terminal window for the current screen. */
static td_rect_t default_rect(void)
{
    int cols = td_stats()->cols;
    int desk = td_wm_desktop_rows();
    int w = cols - 2 < 82 ? cols - 2 : 82;
    int h = desk - 1 < 26 ? desk - 1 : 26;
    return td_rect(-1, -1, w, h);
}

/* Install the backend and start the program right away (a shell's startup
 * script should run at boot, not when the window is first opened). Its
 * output is collected in the background until the window is shown. */
void td_terminal_set_backend(const td_term_backend_t *backend)
{
    s_backend = backend;
    if (!backend)
        return;
    td_rect_t r = default_rect();
    ensure_vt(r.w - 2, r.h - 2);
    if (s_bg_timer < 0)
        s_bg_timer = td_timer_start(100, true, background_pump, NULL, td_millis());
    td_session_init();    /* the desktop belongs to the shell's user */
}

static void on_draw(td_window_t *win, int w, int h)
{
    (void)win;
    const td_theme_t *t = td_theme();
    if (!s_backend)
    {
        td_fill(td_rect(0, 0, w, h), ' ', 7, 0);
        td_text(1, 1, "No shell backend in this build.", 7, 0, 0);
        td_text(1, 2, "The port supplies one with td_terminal_set_backend().", 8, 0, 0);
        return;
    }
    if (s_vt.cols != w || s_vt.rows != h)
    {
        td_vterm_resize(&s_vt, w, h);
        if (s_backend->resize)
            s_backend->resize(s_backend->ctx, w, h);
    }
    td_fill(td_rect(0, 0, w, h), ' ', s_vt.def_fg, s_vt.def_bg);
    td_vterm_draw(&s_vt, 0, 0, s_scroll, win == td_win_focused());
    if (s_scroll > 0)
    {
        char tag[32];
        int n = snprintf(tag, sizeof(tag), " history -%d ", s_scroll);
        td_text(w - n, 0, tag, t->title_fg, t->title_bg, 0);
    }
}

static void scroll_by(int delta)
{
    int max = td_vterm_scrollback_lines(&s_vt);
    s_scroll += delta;
    if (s_scroll < 0)
        s_scroll = 0;
    if (s_scroll > max)
        s_scroll = max;
    td_wm_invalidate();
}

static void menu_chosen(int item, void *user)
{
    (void)user;
    if (!td_win_is_open(s_win))
        return;
    switch (item)
    {
    case 0:
        td_win_set_fullscreen(s_win, !(s_win->flags & TD_WIN_FULLSCREEN));
        break;
    case 1:
        s_vt.sb_count = 0;      /* forget the scrollback */
        s_scroll = 0;
        td_wm_invalidate();
        break;
    case 3:
        td_win_close(s_win);
        break;
    default:
        break;
    }
}

static void context_menu(td_window_t *win, const td_event_t *ev)
{
    const char *items[] = {
        (win->flags & TD_WIN_FULLSCREEN) ? "Leave full screen (F11)" : "Full screen (F11)",
        "Clear history",
        "-",
        "Close window",
    };
    td_rect_t c = td_win_client(win);
    td_menu_popup(c.x + ev->x, c.y + ev->y, items, 4, menu_chosen, NULL);
}

static bool on_event(td_window_t *win, const td_event_t *ev)
{
    if (ev->type == TD_EV_MOUSE)
    {
        if (ev->button == TD_BUTTON_WHEEL_UP)
            scroll_by(3);
        else if (ev->button == TD_BUTTON_WHEEL_DOWN)
            scroll_by(-3);
        else if (ev->button == TD_BUTTON_RIGHT && ev->action == TD_MOUSE_PRESS)
            context_menu(win, ev);
        return true;
    }
    if (ev->type == TD_EV_PASTE)
    {
        /* Typed into the shell as it came (lines end in CR, like Enter). */
        int n = 0;
        const char *text = td_paste_text(&n);
        if (text && n > 0)
        {
            s_scroll = 0;
            send(text, n);
        }
        return true;
    }
    if (ev->type != TD_EV_KEY)
        return false;

    if ((ev->mods & TD_MOD_SHIFT) && (ev->key == TD_KEY_PGUP || ev->key == TD_KEY_PGDN))
    {
        int page = td_win_client(win).h - 1;
        scroll_by(ev->key == TD_KEY_PGUP ? page : -page);
        return true;
    }
    char out[16];
    int n = encode_key(ev, out);
    if (n > 0)
    {
        s_scroll = 0;
        send(out, n);
    }
    return true;
}

/* A file dropped on the Terminal types its path (as the shell sees it). */
static bool on_drop(td_window_t *win, int x, int y, const td_drag_item_t *item)
{
    (void)win;
    (void)x;
    (void)y;
    const char *p = td_shell_path(item->path);
    bool quote = strchr(p, ' ') != NULL;
    char text[200];
    int n = snprintf(text, sizeof(text), quote ? "\"%s\" " : "%s ", p);
    if (n > 0 && n < (int)sizeof(text))
        send(text, n);
    return true;
}

static void on_tick(td_window_t *win)
{
    (void)win;
    pump();
}

static void on_close(td_window_t *win)
{
    (void)win;
    s_win = NULL;
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    char title[TD_TITLE_MAX];
    snprintf(title, sizeof(title), "Terminal - %s", s_backend ? s_backend->name : "none");

    td_window_desc_t d = {
        .title = title,
        .rect = default_rect(),
        .flags = TD_WIN_DEFAULT | TD_WIN_RAW_KEYS,
        .min_w = 30,
        .min_h = 8,
        .on_draw = on_draw,
        .on_event = on_event,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = 15,
        .on_drop = on_drop,
    };
    s_win = td_win_create(&d);
    if (!s_win)
        return;

    td_rect_t c = td_win_client(s_win);
    ensure_vt(c.w, c.h);
    if (s_bg_timer < 0)
        s_bg_timer = td_timer_start(100, true, background_pump, NULL, td_millis());
}

static const td_app_t s_app = {"Terminal", launch, ">_"};

/* ------------------------------------------------------ run a command */

bool td_terminal_command(const char *command)
{
    if (!s_backend || !s_backend->write)
    {
        td_msgbox("Terminal", "This build has no shell to run it in.", "OK", NULL, NULL);
        return false;
    }
    s_scroll = 0;
    send("\x15", 1);                       /* Ctrl+U: drop a half-typed line */
    send(command, (int)strlen(command));
    send("\r", 1);
    return true;
}

bool td_terminal_run(const char *command)
{
    if (!s_backend || !s_backend->write)
        return td_terminal_command(command);   /* says why not */
    launch();
    return td_terminal_command(command);
}

int td_terminal_draw_view(td_window_t *win, int x, int y, int w, int h, int scroll_back, bool cursor)
{
    if (!s_backend || w <= 0 || h <= 0)
        return 0;
    if (!s_vt_ready)
        ensure_vt(w, h);
    if (!td_win_is_open(s_win) && (s_vt.cols != w || s_vt.rows != h))
    {
        td_vterm_resize(&s_vt, w, h);
        if (s_backend->resize)
            s_backend->resize(s_backend->ctx, w, h);
    }
    if (scroll_back > td_vterm_scrollback_lines(&s_vt))
        scroll_back = td_vterm_scrollback_lines(&s_vt);
    if (scroll_back < 0)
        scroll_back = 0;
    /* Taller than the view (the Terminal window is open): the rows that
     * end at the cursor. */
    int first = s_vt.rows > h && s_vt.cy >= h ? s_vt.cy - h + 1 : 0;
    td_rect_t c = td_win_client(win);
    td_rect_t old = td_draw_get_clip();
    td_rect_t view = td_rect(c.x + x, c.y + y, w, h);
    td_rect_t clip = td_rect_intersect(old, view);
    td_draw_clip(clip);
    td_fill(td_rect(x, y, w, h), ' ', s_vt.def_fg, s_vt.def_bg);
    td_vterm_draw(&s_vt, x, y - first, scroll_back, cursor);
    td_draw_clip(old);
    return scroll_back;
}

void td_terminal_key(const td_event_t *ev)
{
    char out[16];
    int n = encode_key(ev, out);
    if (n > 0)
    {
        s_scroll = 0;
        send(out, n);
    }
}

void td_terminal_send(const char *text, int len)
{
    s_scroll = 0;
    send(text, len);
}

void td_terminal_set_viewer(td_window_t *win, void (*tap)(const uint8_t *data, int len))
{
    s_viewer = win;
    s_tap = win ? tap : NULL;
}

bool td_is_script(const char *name)
{
    size_t n = strlen(name), e = sizeof(TD_SCRIPT_EXT) - 1;
    return n > e && strcmp(name + n - e, TD_SCRIPT_EXT) == 0;
}

bool td_is_python(const char *name)
{
    size_t n = strlen(name);
    return n > 3 && strcmp(name + n - 3, ".py") == 0;
}

bool td_is_runnable(const char *name)
{
    return td_is_script(name) || td_is_python(name);
}

bool td_script_run(const char *path)
{
    const char *p = td_shell_path(path);
    char cmd[TD_PATH_MAX + 16];
    const char *fmt = td_is_python(path) ? "py \"%s\"" : "tdsh run \"%s\"";
    if (strchr(p, '"') || snprintf(cmd, sizeof(cmd), fmt, p) >= (int)sizeof(cmd))
    {
        td_msgbox("Run", "This file name cannot be passed to the shell.", "OK", NULL, NULL);
        return false;
    }
    return td_terminal_run(cmd);
}

/* Rough brightness (0..255) of a 256-colour palette entry. */
static int palette_brightness(uint8_t c)
{
    static const uint8_t basic[16] = {0, 60, 60, 110, 50, 70, 110, 190, 110, 150, 160, 230, 130, 160, 220, 255};
    if (c < 16)
        return basic[c];
    if (c >= 232)
        return 8 + (c - 232) * 10;
    c -= 16;
    static const uint8_t level[6] = {0, 95, 135, 175, 215, 255};
    int r = level[c / 36], g = level[(c / 6) % 6], b = level[c % 6];
    return (r * 3 + g * 6 + b) / 10;
}

uint8_t td_script_colour(uint8_t bg)
{
    return palette_brightness(bg) < 128 ? 10 : 28;
}

void td_terminal_register(void)
{
    td_app_register(&s_app);
}
