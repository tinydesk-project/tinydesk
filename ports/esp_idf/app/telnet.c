/*
 * telnet.c - the desktop over Wi-Fi / Ethernet on TCP port 23.
 *
 * Polled from the UI loop with non-blocking sockets, so it costs no task
 * stack (RAM is tight next to TinyDesk Shell's SSH server). One client at a time:
 *
 *   LISTEN -> LOGIN (username, password; TinyDesk Shell accounts) -> ACTIVE
 *
 * While ACTIVE the client owns the desktop: hal_mux.c reads and writes
 * through here instead of USB until the connection ends. Telnet commands
 * are negotiated for character mode with the server echoing, and stripped
 * from the input together with the NUL / LF that telnet sends after CR.
 *
 * Telnet is not encrypted: the password crosses your network in the clear.
 * Use it on a network you trust, or switch it off in the Network app.
 */
#include "telnet.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "lwip/sockets.h"
#include "nvs.h"
#include "tdsh_espidf.h"

#define PORT             23
#define LOGIN_TRIES      3
#define LOGIN_TIMEOUT_US (90 * 1000000LL)
#define RETRY_US         (2 * 1000000LL)

enum
{
    IAC = 255,
    DONT = 254,
    DO = 253,
    WONT = 252,
    WILL = 251,
    SB = 250,
    SE = 240
};
enum
{
    OPT_ECHO = 1,
    OPT_SGA = 3
};
typedef enum
{
    ST_OFF,
    ST_LISTEN,
    ST_USER,
    ST_PASS,
    ST_ACTIVE
} state_t;

static const char *TAG = "telnet";

static state_t s_state = ST_OFF;
static bool s_enabled = false;
static int s_mode = 0;   /* TD_TELNET_* */
static int s_listen = -1;
static int s_fd = -1;
static int64_t s_next_try_us;
static int64_t s_login_deadline_us;
static int s_tries;
static char s_peer[16];
static char s_line[65];
static int s_line_len;
static char s_user[TDSH_USERNAME_MAX];

/* Input filter state and a small buffer of filtered bytes. */
static uint8_t s_filter_state;
static bool s_after_cr;
static uint8_t s_in[64];
static int s_in_len, s_in_pos;

/* ------------------------------------------------------------ helpers */

static int64_t now_us(void)
{
    return esp_timer_get_time();
}

static void set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void say(const char *s)
{
    if (s_fd >= 0)
        send(s_fd, s, strlen(s), 0);
}

/* Strip telnet commands in place; returns the number of data bytes. */
static int filter(uint8_t *buf, int n)
{
    int m = 0;
    for (int i = 0; i < n; i++)
    {
        uint8_t b = buf[i];
        switch (s_filter_state)
        {
        case 0:
            if (b == IAC)
            {
                s_filter_state = 1;
                break;
            }
            if (s_after_cr && (b == '\n' || b == 0))
            {
                s_after_cr = false;
                break;
            }
            s_after_cr = (b == '\r');
            buf[m++] = b;
            break;
        case 1:
            if (b == IAC)
            {
                buf[m++] = IAC;
                s_filter_state = 0;
            }    /* escaped 0xFF */
            else if (b == SB)
                s_filter_state = 3;
            else if (b >= WILL && b <= DONT)
                s_filter_state = 2;
            else
                s_filter_state = 0;
            break;
        case 2:
            s_filter_state = 0;
            break;                           /* option code */
        case 3:
            if (b == IAC)
                s_filter_state = 4;
            break;
        case 4:
            s_filter_state = (b == SE) ? 0 : 3;
            break;
        default:
            s_filter_state = 0;
            break;
        }
    }
    return m;
}

/* Fetch whatever the client sent (non-blocking). Returns false when the
 * connection has ended. */
static bool fill_input(void)
{
    if (s_in_pos < s_in_len)
        return true;
    int n = recv(s_fd, s_in, sizeof(s_in), MSG_DONTWAIT);
    if (n == 0)
        return false;
    if (n < 0)
        return errno == EAGAIN || errno == EWOULDBLOCK;
    s_in_len = filter(s_in, n);
    s_in_pos = 0;
    return true;
}

static void drop_client(void)
{
    if (s_fd >= 0)
    {
        shutdown(s_fd, SHUT_RDWR);
        close(s_fd);
    }
    if (s_state == ST_ACTIVE)
        ESP_LOGI(TAG, "client %s left", s_peer);
    s_fd = -1;
    s_peer[0] = '\0';
    s_in_len = s_in_pos = 0;
    s_state = s_enabled ? ST_LISTEN : ST_OFF;
}

static void close_listener(void)
{
    if (s_listen >= 0)
        close(s_listen);
    s_listen = -1;
}

/* ----------------------------------------------------------- states */

static void open_listener(void)
{
    if (now_us() < s_next_try_us)
        return;
    s_next_try_us = now_us() + RETRY_US;
    int ls = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (ls < 0)
        return;
    int one = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    struct sockaddr_in addr = {.sin_family = AF_INET, .sin_port = htons(PORT), .sin_addr.s_addr = htonl(INADDR_ANY)};
    if (bind(ls, (struct sockaddr *)&addr, sizeof(addr)) != 0 || listen(ls, 1) != 0)
    {
        close(ls);
        return;
    }
    set_nonblocking(ls);
    s_listen = ls;
    ESP_LOGI(TAG, "listening on port %d", PORT);
}

static void try_accept(void)
{
    struct sockaddr_in peer;
    socklen_t plen = sizeof(peer);
    int fd = accept(s_listen, (struct sockaddr *)&peer, &plen);
    if (fd < 0)
        return;

    s_fd = fd;
    inet_ntoa_r(peer.sin_addr, s_peer, sizeof(s_peer));
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    /* Sends may block briefly, so a full frame (larger than the TCP send
     * buffer) still gets out; reads never block (MSG_DONTWAIT). */
    struct timeval tv = {.tv_sec = 0, .tv_usec = 300000};
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    s_filter_state = 0;
    s_after_cr = false;
    s_in_len = s_in_pos = 0;
    s_tries = 0;
    s_line_len = 0;
    s_login_deadline_us = now_us() + LOGIN_TIMEOUT_US;

    static const uint8_t negotiate[] = {IAC, WILL, OPT_ECHO, IAC, WILL, OPT_SGA, IAC, DO, OPT_SGA};
    send(fd, negotiate, sizeof(negotiate), 0);
    say("\r\ntinydesk - log in with your TinyDesk account.\r\n\r\nlogin: ");
    s_state = ST_USER;
}

/* Collect a line of input during login; true when Enter was pressed. */
static bool login_line(bool secret)
{
    while (s_in_pos < s_in_len)
    {
        uint8_t b = s_in[s_in_pos++];
        if (b == '\r' || b == '\n')
        {
            s_line[s_line_len] = '\0';
            s_line_len = 0;
            say("\r\n");
            return true;
        }
        if ((b == 0x7F || b == 0x08) && s_line_len > 0)
        {
            s_line_len--;
            say("\b \b");
        }
        else if (b >= 0x20 && b < 0x7F && s_line_len < (int)sizeof(s_line) - 1)
        {
            s_line[s_line_len++] = (char)b;
            char echo[2] = {secret ? '*' : (char)b, 0};
            say(echo);
        }
    }
    return false;
}

static void login_step(void)
{
    if (!fill_input() || now_us() > s_login_deadline_us)
    {
        drop_client();
        return;
    }
    if (s_state == ST_USER)
    {
        if (!login_line(false))
            return;
        snprintf(s_user, sizeof(s_user), "%.*s", (int)sizeof(s_user) - 1, s_line);
        say("Password: ");
        s_state = ST_PASS;
        return;
    }
    if (!login_line(true))
        return;
    /* This takes over a shared shell, including any command already running.
     * Until isolated sessions exist, only root may take over the desktop. */
    bool ok = strcmp(s_user, "root") == 0 && tdsh_user_authenticate_remote(s_user, s_line);
    memset(s_line, 0, sizeof(s_line));
    if (ok)
    {
        if (!tdsh_console_mark_remote())
        {
            say("Physical recovery is in progress. Try again later.\r\n");
            drop_client();
            return;
        }
        ESP_LOGI(TAG, "%s logged in from %s", s_user, s_peer);
        s_in_len = s_in_pos = 0;
        s_state = ST_ACTIVE;              /* the HAL now hands over the desktop */
        return;
    }
    ESP_LOGW(TAG, "failed login for '%s' from %s", s_user, s_peer);
    if (++s_tries >= LOGIN_TRIES)
    {
        say("Login incorrect. Goodbye.\r\n");
        drop_client();
        return;
    }
    say("Login incorrect.\r\n\r\nlogin: ");
    s_state = ST_USER;
}

/* --------------------------------------------------------------- api */

void telnet_start(void)
{
    nvs_handle_t h;
    uint8_t v = 0;
    if (nvs_open("tinydesk", NVS_READONLY, &h) == ESP_OK)
    {
        nvs_get_u8(h, "telnet", &v);
        uint8_t m = 0;
        if (nvs_get_u8(h, "telnet_mode", &m) == ESP_OK && m <= 2)
            s_mode = m;
        nvs_close(h);
    }
    s_enabled = v != 0 && tdsh_remote_access_ready();
    s_state = s_enabled ? ST_LISTEN : ST_OFF;
}

void telnet_poll(void)
{
    switch (s_state)
    {
    case ST_OFF:
        break;
    case ST_LISTEN:
        if (s_listen < 0)
            open_listener();
        if (s_listen >= 0)
            try_accept();
        break;
    case ST_USER:
    case ST_PASS:
        login_step();
        break;
    case ST_ACTIVE:
        if (!fill_input())
            drop_client();
        break;
    }
}

bool telnet_active(void)
{
    return s_state == ST_ACTIVE;
}

int telnet_read_byte(void)
{
    if (s_state != ST_ACTIVE)
        return -1;
    if (s_in_pos >= s_in_len && !fill_input())
    {
        drop_client();
        return -1;
    }
    return s_in_pos < s_in_len ? s_in[s_in_pos++] : -1;
}

int telnet_write(const uint8_t *buf, int len)
{
    if (s_state != ST_ACTIVE)
        return 0;
    int done = 0;
    while (done < len)
    {
        int n = send(s_fd, buf + done, (size_t)(len - done), 0);
        if (n <= 0)
        {
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                break;   /* stalled: drop frame */
            if (n < 0)
                drop_client();
            break;
        }
        done += n;
    }
    return done;
}

bool telnet_enabled(void)
{
    return s_enabled;
}

void telnet_set_enabled(bool on)
{
    if (on && !tdsh_remote_access_ready())
    {
        ESP_LOGW(TAG, "Change the factory root password with passwd before enabling Telnet (old password: " TDSH_FACTORY_ROOT_PASSWORD ")");
        return;
    }
    s_enabled = on;
    nvs_handle_t h;
    if (nvs_open("tinydesk", NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_set_u8(h, "telnet", on ? 1 : 0);
        nvs_commit(h);
        nvs_close(h);
    }
    if (!on)
    {
        if (s_fd >= 0)
            say("\r\nTelnet access was switched off.\r\n");
        drop_client();
        close_listener();
        s_state = ST_OFF;
    }
    else if (s_state == ST_OFF)
    {
        s_state = ST_LISTEN;
        s_next_try_us = 0;
    }
}

int telnet_mode(void)
{
    return s_mode;
}

void telnet_set_mode(int mode)
{
    if (mode < 0 || mode > 2)
        return;
    s_mode = mode;
    nvs_handle_t h;
    if (nvs_open("tinydesk", NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_set_u8(h, "telnet_mode", (uint8_t)mode);
        nvs_commit(h);
        nvs_close(h);
    }
}

const char *telnet_peer(void)
{
    return s_state == ST_ACTIVE && s_peer[0] ? s_peer : NULL;
}

const char *telnet_user(void)
{
    return s_state == ST_ACTIVE ? s_user : NULL;
}
