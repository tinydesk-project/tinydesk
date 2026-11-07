/*
 * remote.c - Remote Desktop: another TinyDesk's desktop in a window, over
 * Telnet (its Network app: "Telnet desktop"), for service work.
 *
 * The window's top row holds the address, the port, Connect / Disconnect
 * and the state; the rest is a terminal (td_vterm) that shows what the
 * remote desktop sends. The remote's Telnet login (root) is typed into it
 * first. Keys go there as a VT100 would send them (td_terminal_encode_key),
 * mouse events as SGR reports in its cells, pastes as bracketed pastes; the
 * terminal answers the remote's size, device and frame queries itself, so
 * the remote desktop takes the window's size (it asks every second).
 * Telnet: the server's ECHO and SGA are accepted, every other option is
 * refused, sub-negotiations are skipped and 0xFF data bytes are doubled.
 * Pictures are not passed on (the remote shows what its windows draw under
 * them). Telnet is not encrypted: use it on a network you trust.
 */
#include "td_apps.h"
#include "td_sock.h"
#include "tinydesk/td_vterm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TICK_MS  20
#define OUT_MAX  4096
#define BAR_ROWS 1

enum
{
    IAC = 255,
    DONT = 254,
    DO = 253,
    WONT = 252,
    WILL = 251,
    SB = 250,
    SE = 240,
    OPT_ECHO = 1,
    OPT_SGA = 3
};

enum
{
    T_DATA,
    T_IAC,
    T_OPT,   /* after WILL / WONT / DO / DONT */
    T_SB,
    T_SB_IAC
};

enum
{
    S_IDLE,
    S_CONNECTING,
    S_CONNECTED
};

static td_window_t *s_win;
static td_widget_t *s_host, *s_port, *s_go, *s_state;
static td_vterm_t *s_vt;
static td_sock_t s_sock = TD_SOCK_INVALID;
static int s_status = S_IDLE;
static int s_tstate, s_tcmd;
static uint8_t s_out[OUT_MAX];
static int s_out_len;
static char s_host_text[64] = "192.168.4.1", s_port_text[8] = "23";

static void set_state(const char *text)
{
    if (s_state)
        td_widget_set_text(s_state, text);
}

/* --------------------------------------------------------- sending */

static void flush_out(void)
{
    if (s_sock == TD_SOCK_INVALID || s_status != S_CONNECTED)
        return;
    while (s_out_len > 0)
    {
        int n = td_sock_send(s_sock, s_out, s_out_len);
        if (n <= 0)
            break;
        memmove(s_out, s_out + n, (size_t)(s_out_len - n));
        s_out_len -= n;
    }
}

static void send_raw(const uint8_t *data, int len)
{
    if (s_out_len + len > OUT_MAX)
        flush_out();
    if (s_out_len + len > OUT_MAX)
        return;   /* the link is stuck: drop it rather than block the desktop */
    memcpy(s_out + s_out_len, data, (size_t)len);
    s_out_len += len;
}

/* Data for the remote: 0xFF doubled, as Telnet wants. */
static void send_data(const void *data, int len)
{
    const uint8_t *p = data;
    for (int i = 0; i < len; i++)
    {
        uint8_t b = p[i];
        send_raw(&b, 1);
        if (b == IAC)
            send_raw(&b, 1);
    }
}

/* The terminal's answers (size, device attributes, frame done). */
static void vt_reply(void *user, const char *data, int len)
{
    (void)user;
    send_data(data, len);
}

/* --------------------------------------------------------- receiving */

static void telnet_option(int cmd, int opt)
{
    uint8_t answer[3] = {IAC, 0, (uint8_t)opt};
    if (cmd == WILL)
        answer[1] = opt == OPT_ECHO || opt == OPT_SGA ? DO : DONT;
    else if (cmd == DO)
        answer[1] = opt == OPT_SGA ? WILL : WONT;
    else
        return;   /* WONT / DONT need no answer */
    send_raw(answer, 3);
}

static void receive(const uint8_t *buf, int len)
{
    uint8_t data[512];
    int n = 0;
    for (int i = 0; i < len; i++)
    {
        uint8_t b = buf[i];
        switch (s_tstate)
        {
        case T_DATA:
            if (b == IAC)
                s_tstate = T_IAC;
            else
                data[n++] = b;
            break;
        case T_IAC:
            if (b == IAC)
            {
                data[n++] = IAC;
                s_tstate = T_DATA;
            }
            else if (b == WILL || b == WONT || b == DO || b == DONT)
            {
                s_tcmd = b;
                s_tstate = T_OPT;
            }
            else
                s_tstate = b == SB ? T_SB : T_DATA;
            break;
        case T_OPT:
            telnet_option(s_tcmd, b);
            s_tstate = T_DATA;
            break;
        case T_SB:
            if (b == IAC)
                s_tstate = T_SB_IAC;
            break;
        default:   /* T_SB_IAC */
            s_tstate = b == SE ? T_DATA : T_SB;
            break;
        }
        if (n == (int)sizeof(data))
        {
            td_vterm_write(s_vt, data, n);
            n = 0;
        }
    }
    if (n)
        td_vterm_write(s_vt, data, n);
}

/* ------------------------------------------------------- connection */

static void disconnect(const char *why)
{
    if (s_sock != TD_SOCK_INVALID)
        td_sock_close(s_sock);
    s_sock = TD_SOCK_INVALID;
    s_status = S_IDLE;
    s_out_len = 0;
    free(s_vt);
    s_vt = NULL;
    if (td_win_is_open(s_win))
        s_win->flags &= (uint16_t)~TD_WIN_RAW_KEYS;   /* the address fields edit again */
    if (s_go)
        td_widget_set_text(s_go, "Connect");
    set_state(why);
    if (s_win)
        td_win_invalidate(s_win);
}

static void connect_to(void)
{
    char err[96] = "";
    int port = atoi(td_widget_text(s_port));
    const char *host = td_widget_text(s_host);
    if (!host[0] || port < 1 || port > 65535)
    {
        set_state("Give an address and a port (23).");
        return;
    }
    td_rect_t c = td_win_client(s_win);
    s_vt = malloc(sizeof(*s_vt));
    if (!s_vt)
    {
        set_state("Not enough memory.");
        return;
    }
    td_vterm_init(s_vt, c.w, c.h - BAR_ROWS > 2 ? c.h - BAR_ROWS : 2);
    s_vt->reply = vt_reply;
    s_vt->reply_user = NULL;
    td_addr_t addr;
    set_state("Connecting...");
    if (!td_sock_resolve(host, (uint16_t)port, &addr, err, sizeof(err)) ||
        (s_sock = td_sock_connect_start(&addr, err, sizeof(err))) == TD_SOCK_INVALID)
    {
        disconnect(err[0] ? err : "Cannot connect.");
        return;
    }
    s_status = S_CONNECTING;
    s_tstate = T_DATA;
    td_widget_set_text(s_go, "Disconnect");
}

static void on_go(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    if (s_status == S_IDLE)
        connect_to();
    else
        disconnect("Disconnected.");
}

static void on_tick(td_window_t *win)
{
    if (s_status == S_CONNECTING)
    {
        char err[96] = "";
        int r = td_sock_connect_poll(s_sock, err, sizeof(err));
        if (r < 0)
            disconnect(err[0] ? err : "Cannot connect.");
        else if (r > 0)
        {
            s_status = S_CONNECTED;
            win->flags |= TD_WIN_RAW_KEYS;   /* every key (Tab, Esc too) and pastes go to the remote */
            char text[96];
            snprintf(text, sizeof(text), "Connected to %s: sign in as root.", td_widget_text(s_host));
            set_state(text);
        }
        return;
    }
    if (s_status != S_CONNECTED)
        return;
    uint8_t buf[1024];
    bool got = false;
    for (int i = 0; i < 16; i++)   /* at most 16 KB a tick: the desktop stays responsive */
    {
        int n = td_sock_recv(s_sock, buf, sizeof(buf));
        if (n < 0)
        {
            disconnect("The remote closed the connection.");
            return;
        }
        if (n == 0)
            break;
        receive(buf, n);
        got = true;
    }
    flush_out();
    if (got)
        td_win_invalidate(win);
}

/* ------------------------------------------------------------ input */

static void send_mouse(const td_event_t *ev)
{
    int x = ev->x + 1, y = ev->y - BAR_ROWS + 1;
    if (y < 1 || !s_vt || x > s_vt->cols || y > s_vt->rows)
        return;
    /* Only what the remote asked for: at its login prompt nothing, so the
     * reports are not typed into the user name. */
    if (!s_vt->mouse || (ev->action == TD_MOUSE_MOVE && s_vt->mouse < 1003) ||
        (ev->action == TD_MOUSE_DRAG && s_vt->mouse < 1002))
        return;
    int b;
    char final = 'M';
    if (ev->button == TD_BUTTON_WHEEL_UP || ev->button == TD_BUTTON_WHEEL_DOWN)
        b = ev->button == TD_BUTTON_WHEEL_UP ? 64 : 65;
    else if (ev->action == TD_MOUSE_MOVE)
        b = 35;
    else
    {
        b = ev->button <= TD_BUTTON_RIGHT ? ev->button : 0;
        if (ev->action == TD_MOUSE_DRAG)
            b += 32;
        if (ev->action == TD_MOUSE_RELEASE)
            final = 'm';
    }
    if (ev->mods & TD_MOD_SHIFT)
        b += 4;
    if (ev->mods & TD_MOD_ALT)
        b += 8;
    if (ev->mods & TD_MOD_CTRL)
        b += 16;
    char seq[32];
    int n = snprintf(seq, sizeof(seq), "\x1b[<%d;%d;%d%c", b, x, y, final);
    send_data(seq, n);
}

static bool on_event(td_window_t *win, const td_event_t *ev)
{
    (void)win;
    if (s_status != S_CONNECTED)
        return false;   /* the address fields and the button work as usual */
    if (ev->type == TD_EV_MOUSE)
    {
        if (ev->y < BAR_ROWS)
            return false;   /* Disconnect stays clickable */
        send_mouse(ev);
        return true;
    }
    if (ev->type == TD_EV_KEY)
    {
        char out[16];
        int n = td_terminal_encode_key(ev, out);
        if (n > 0)
            send_data(out, n);
        return true;
    }
    if (ev->type == TD_EV_PASTE)
    {
        int len = 0;
        const char *text = td_paste_text(&len);
        if (text && len > 0)
        {
            send_data("\x1b[200~", 6);
            send_data(text, len);
            send_data("\x1b[201~", 6);
        }
        return true;
    }
    return false;
}

/* ----------------------------------------------------------- window */

static void on_draw(td_window_t *win, int w, int h)
{
    (void)win;
    const td_theme_t *t = td_theme();
    td_fill(td_rect(0, 0, w, BAR_ROWS), ' ', t->win_fg, t->win_bg);
    td_fill(td_rect(0, BAR_ROWS, w, h - BAR_ROWS), ' ', 250, 0);
    if (!s_vt || s_status != S_CONNECTED)
    {
        static const char *const help[] = {
            "Another TinyDesk's desktop, for service work.",
            "On that board: Network -> Telnet desktop on (root's password",
            "changed from the factory one). Here: its address, Connect, then",
            "sign in as root in this window. Telnet is not encrypted: use it",
            "on a network you trust.",
        };
        for (int i = 0; i < 5 && BAR_ROWS + 1 + i < h; i++)
            td_text(1, BAR_ROWS + 1 + i, help[i], 250, 0, 0);
        return;
    }
    int rows = h - BAR_ROWS;
    if (w > 1 && rows > 1 && (w != s_vt->cols || rows != s_vt->rows))
        td_vterm_resize(s_vt, w, rows);   /* the remote follows within a second */
    td_vterm_draw(s_vt, 0, BAR_ROWS, 0, false);
}

static void on_close(td_window_t *win)
{
    (void)win;
    disconnect("");
    s_win = NULL;
    s_host = s_port = s_go = s_state = NULL;
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    td_window_desc_t d = {
        .title = "Remote Desktop",
        .rect = td_rect(-1, -1, 82, 28),
        .min_w = 60,
        .min_h = 10,
        .flags = TD_WIN_DEFAULT | TD_WIN_MOUSE_MOVES,
        .on_draw = on_draw,
        .on_event = on_event,
        .on_tick = on_tick,
        .tick_ms = TICK_MS,
        .on_close = on_close,
    };
    s_win = td_win_create(&d);
    if (!s_win)
        return;
    td_label(s_win, 0, 0, 5, "Host");
    s_host = td_textbox(s_win, 5, 0, 18, (int)sizeof(s_host_text) - 1, on_go, NULL);
    td_textbox_set_buffer(s_host, s_host_text, (int)sizeof(s_host_text));
    td_label(s_win, 24, 0, 5, "Port");
    s_port = td_textbox(s_win, 29, 0, 6, (int)sizeof(s_port_text) - 1, on_go, NULL);
    td_textbox_set_buffer(s_port, s_port_text, (int)sizeof(s_port_text));
    s_go = td_button(s_win, 36, 0, "Connect", on_go, NULL);
    s_state = td_label(s_win, 52, 0, 0, "");
    td_widget_focus(s_host);
}

static const td_app_t s_app = {"Remote Desktop", launch, "RD"};

void td_remote_register(void)
{
    td_app_register(&s_app);
}
