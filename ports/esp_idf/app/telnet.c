/*
 * telnet.c - the desktop over Wi-Fi / Ethernet on TCP port 23.
 *
 * Polled from the UI loop with non-blocking sockets, so it costs no task
 * stack (RAM is tight next to TinyDesk Shell's SSH server). Up to
 * TELNET_CLIENTS clients at once, as many as telnet_set_clients() allows
 * (two when both displays show Telnet clients), each:
 *
 *   FREE -> LOGIN (username, password; TinyDesk Shell accounts) -> ACTIVE
 *
 * While ACTIVE hal_mux.c shows the client a display of the desktop
 * (display 1 or 2, Settings -> Displays) until the connection ends. Telnet commands
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
    ST_FREE,
    ST_USER,
    ST_PASS,
    ST_ACTIVE
} state_t;

    /* One connection: its login, input filter and a small buffer of filtered
 * bytes. */
typedef struct
{
    state_t state;
    int fd;
    int64_t login_deadline_us;
    int tries;
    char peer[16];
    char line[65];
    int line_len;
    char user[TDSH_USERNAME_MAX];
    uint8_t filter_state;
    bool after_cr;
    uint8_t in[64];
    int in_len, in_pos;
} client_t;

static const char *TAG = "telnet";

static bool s_enabled = false;
static int s_mode = 0;   /* TD_TELNET_* */
static int s_allowed = 1;   /* clients at once (telnet_set_clients) */
static int s_listen = -1;
static int64_t s_next_try_us;
static client_t s_cl[TELNET_CLIENTS] = {{.fd = -1}, {.fd = -1}};

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

static void say(client_t *c, const char *s)
{
    if (c->fd >= 0)
        send(c->fd, s, strlen(s), 0);
}

    /* Strip telnet commands in place; returns the number of data bytes. */
static int filter(client_t *c, uint8_t *buf, int n)
{
    int m = 0;
    for (int i = 0; i < n; i++)
    {
        uint8_t b = buf[i];
        switch (c->filter_state)
        {
        case 0:
            if (b == IAC)
            {
                c->filter_state = 1;
                break;
            }
            if (c->after_cr && (b == '\n' || b == 0))
            {
                c->after_cr = false;
                break;
            }
            c->after_cr = (b == '\r');
            buf[m++] = b;
            break;
        case 1:
            if (b == IAC)
            {
                buf[m++] = IAC;
                c->filter_state = 0;
            }    /* escaped 0xFF */
            else if (b == SB)
                c->filter_state = 3;
            else if (b >= WILL && b <= DONT)
                c->filter_state = 2;
            else
                c->filter_state = 0;
            break;
        case 2:
            c->filter_state = 0;
            break;                           /* option code */
        case 3:
            if (b == IAC)
                c->filter_state = 4;
            break;
        case 4:
            c->filter_state = (b == SE) ? 0 : 3;
            break;
        default:
            c->filter_state = 0;
            break;
        }
    }
    return m;
}

    /* Fetch whatever the client sent (non-blocking). Returns false when the
 * connection has ended. */
static bool fill_input(client_t *c)
{
    if (c->in_pos < c->in_len)
        return true;
    int n = recv(c->fd, c->in, sizeof(c->in), MSG_DONTWAIT);
    if (n == 0)
        return false;
    if (n < 0)
        return errno == EAGAIN || errno == EWOULDBLOCK;
    c->in_len = filter(c, c->in, n);
    c->in_pos = 0;
    return true;
}

static void drop_client(client_t *c)
{
    if (c->fd >= 0)
    {
        shutdown(c->fd, SHUT_RDWR);
        close(c->fd);
    }
    if (c->state == ST_ACTIVE)
        ESP_LOGI(TAG, "client %s left", c->peer);
    c->fd = -1;
    c->peer[0] = '\0';
    c->in_len = c->in_pos = 0;
    c->state = ST_FREE;
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
    client_t *c = NULL;
    for (int i = 0; i < s_allowed && i < TELNET_CLIENTS && !c; i++)
        if (s_cl[i].state == ST_FREE)
            c = &s_cl[i];
    if (!c)
    {
        static const char busy[] = "\r\ntinydesk: the desktop is already in use over Telnet.\r\n";
        send(fd, busy, sizeof(busy) - 1, 0);
        shutdown(fd, SHUT_RDWR);
        close(fd);
        return;
    }

    memset(c, 0, sizeof(*c));
    c->fd = fd;
    inet_ntoa_r(peer.sin_addr, c->peer, sizeof(c->peer));
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
      /* Sends may block briefly, so a full frame (larger than the TCP send
     * buffer) still gets out; reads never block (MSG_DONTWAIT). */
    struct timeval tv = {.tv_sec = 0, .tv_usec = 300000};
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    c->login_deadline_us = now_us() + LOGIN_TIMEOUT_US;

    static const uint8_t negotiate[] = {IAC, WILL, OPT_ECHO, IAC, WILL, OPT_SGA, IAC, DO, OPT_SGA};
    send(fd, negotiate, sizeof(negotiate), 0);
    say(c, "\r\ntinydesk - log in with your TinyDesk account.\r\n\r\nlogin: ");
    c->state = ST_USER;
}

    /* Collect a line of input during login; true when Enter was pressed. */
static bool login_line(client_t *c, bool secret)
{
    while (c->in_pos < c->in_len)
    {
        uint8_t b = c->in[c->in_pos++];
        if (b == '\r' || b == '\n')
        {
            c->line[c->line_len] = '\0';
            c->line_len = 0;
            say(c, "\r\n");
            return true;
        }
        if ((b == 0x7F || b == 0x08) && c->line_len > 0)
        {
            c->line_len--;
            say(c, "\b \b");
        }
        else if (b >= 0x20 && b < 0x7F && c->line_len < (int)sizeof(c->line) - 1)
        {
            c->line[c->line_len++] = (char)b;
            char echo[2] = {secret ? '*' : (char)b, 0};
            say(c, echo);
        }
    }
    return false;
}

static void login_step(client_t *c)
{
    if (!fill_input(c) || now_us() > c->login_deadline_us)
    {
        drop_client(c);
        return;
    }
    if (c->state == ST_USER)
    {
        if (!login_line(c, false))
            return;
        snprintf(c->user, sizeof(c->user), "%.*s", (int)sizeof(c->user) - 1, c->line);
        say(c, "Password: ");
        c->state = ST_PASS;
        return;
    }
    if (!login_line(c, true))
        return;
      /* This takes over a shared shell, including any command already running.
     * Until isolated sessions exist, only root may take over the desktop. */
    bool ok = strcmp(c->user, "root") == 0 && tdsh_user_authenticate_remote(c->user, c->line);
    memset(c->line, 0, sizeof(c->line));
    if (ok)
    {
        if (!tdsh_console_mark_remote())
        {
            say(c, "Physical recovery is in progress. Try again later.\r\n");
            drop_client(c);
            return;
        }
        ESP_LOGI(TAG, "%s logged in from %s", c->user, c->peer);
        c->in_len = c->in_pos = 0;
        c->state = ST_ACTIVE;              /* the HAL now shows it the desktop */
        return;
    }
    ESP_LOGW(TAG, "failed login for '%s' from %s", c->user, c->peer);
    if (++c->tries >= LOGIN_TRIES)
    {
        say(c, "Login incorrect. Goodbye.\r\n");
        drop_client(c);
        return;
    }
    say(c, "Login incorrect.\r\n\r\nlogin: ");
    c->state = ST_USER;
}

static client_t *active(int i)
{
    return i >= 0 && i < TELNET_CLIENTS && s_cl[i].state == ST_ACTIVE ? &s_cl[i] : NULL;
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
    s_next_try_us = 0;
}

void telnet_set_clients(int n)
{
    s_allowed = n < 1 ? 1 : n > TELNET_CLIENTS ? TELNET_CLIENTS
                                               : n;
    for (int i = s_allowed; i < TELNET_CLIENTS; i++)
        if (s_cl[i].state != ST_FREE)
        {
            say(&s_cl[i], "\r\nThe second Telnet display was switched off.\r\n");
            drop_client(&s_cl[i]);
        }
}

void telnet_poll(void)
{
    if (!s_enabled)
        return;
    if (s_listen < 0)
        open_listener();
    if (s_listen >= 0)
        try_accept();
    for (int i = 0; i < TELNET_CLIENTS; i++)
    {
        client_t *c = &s_cl[i];
        if (c->state == ST_USER || c->state == ST_PASS)
            login_step(c);
        else if (c->state == ST_ACTIVE && !fill_input(c))
            drop_client(c);
    }
}

bool telnet_active_n(int i)
{
    return active(i) != NULL;
}

int telnet_read_byte_n(int i)
{
    client_t *c = active(i);
    if (!c)
        return -1;
    if (c->in_pos >= c->in_len && !fill_input(c))
    {
        drop_client(c);
        return -1;
    }
    return c->in_pos < c->in_len ? c->in[c->in_pos++] : -1;
}

int telnet_write_n(int i, const uint8_t *buf, int len)
{
    client_t *c = active(i);
    if (!c)
        return 0;
    int done = 0;
    while (done < len)
    {
        int n = send(c->fd, buf + done, (size_t)(len - done), 0);
        if (n <= 0)
        {
            if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                break;   /* stalled: drop frame */
            if (n < 0)
                drop_client(c);
            break;
        }
        done += n;
    }
    return done;
}

const char *telnet_peer_n(int i)
{
    client_t *c = active(i);
    return c && c->peer[0] ? c->peer : NULL;
}

const char *telnet_user_n(int i)
{
    client_t *c = active(i);
    return c ? c->user : NULL;
}

bool telnet_active(void)
{
    return telnet_active_n(0);
}
int telnet_read_byte(void)
{
    return telnet_read_byte_n(0);
}
int telnet_write(const uint8_t *buf, int len)
{
    return telnet_write_n(0, buf, len);
}
const char *telnet_peer(void)
{
    return telnet_peer_n(0) ? telnet_peer_n(0) : telnet_peer_n(1);
}
const char *telnet_user(void)
{
    return telnet_user_n(0);
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
        for (int i = 0; i < TELNET_CLIENTS; i++)
        {
            say(&s_cl[i], "\r\nTelnet access was switched off.\r\n");
            drop_client(&s_cl[i]);
        }
        close_listener();
    }
    else
        s_next_try_us = 0;
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
