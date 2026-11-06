/*
 * hal_mux.c - the tinydesk HAL for the ESP-IDF ports: the board's local
 * link (link.h), and a logged-in Telnet client. Depending on the Telnet
 * mode (telnet_mode) the client takes the desktop over (TD_TELNET_ONLY:
 * the local link shows a note until it leaves) or is a second screen beside
 * the local one (TD_TELNET_DUPLICATE / _EXTEND: td_screen2_attach).
 */
#include "hal_mux.h"

#include <stdio.h>
#include <string.h>

#include "link.h"
#include "td_apps.h"
#include "telnet.h"
#include "tinydesk/td.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

typedef struct
{
    uint8_t rx[64];              /* bytes fetched from the link, not yet read */
    int rx_len, rx_pos;
} link_ctx_t;

static link_ctx_t s_ctx;

static int local_read_byte(link_ctx_t *c)
{
    if (c->rx_pos == c->rx_len)
    {
        int n = link_read(c->rx, sizeof(c->rx));
        if (n <= 0)
            return -1;
        c->rx_len = n;
        c->rx_pos = 0;
    }
    return c->rx[c->rx_pos++];
}

static uint32_t hal_millis(void *ctx)
{
    (void)ctx;
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void hal_sleep(void *ctx, uint32_t ms)
{
    (void)ctx;
    TickType_t ticks = pdMS_TO_TICKS(ms);
    vTaskDelay(ticks > 0 ? ticks : 1);   /* always yield to lower priorities */
}

/* ---------------------------------------------------- local or Telnet */

static bool s_remote;            /* the client took the desktop over */
static bool s_shared;            /* the client is a second screen */
static int s_shared_mode;
static char s_local_user[32];    /* whose desktop the local link had before Telnet took it */
static bool s_local_locked;

/* Switch the desktop's user unless it is already theirs. */
static void become(const char *user)
{
    if (user && user[0] && (td_session_locked() || strcmp(user, td_session_user()) != 0))
        td_session_switch(user, true);
}

static int screen2_read(void *ctx)
{
    (void)ctx;
    return telnet_read_byte();
}

static int screen2_write(void *ctx, const uint8_t *buf, int len)
{
    (void)ctx;
    return telnet_write(buf, len);
}

/* Follow the Telnet session. As a second screen the client sees (and
 * drives) the desktop as it is, beside the local terminal; it changes
 * between duplicate and extend at once. Otherwise the desktop moves to the
 * client (as the user who logged in) and comes back to the local link and
 * its user when it leaves. Only root may log in over Telnet. */
static void follow_session(void)
{
    bool active = telnet_active();
    if (s_shared)
    {
        if (!active)
        {
            s_shared = false;
            td_screen2_detach();
        }
        else if (telnet_mode() != s_shared_mode && telnet_mode() != TD_TELNET_ONLY)
        {
            s_shared_mode = telnet_mode();
            td_screens_set_mode(s_shared_mode == TD_TELNET_EXTEND ? TD_SCREENS_EXTEND : TD_SCREENS_DUPLICATE);
        }
        return;
    }
    if (active && !s_remote && telnet_mode() != TD_TELNET_ONLY)
    {
        s_shared_mode = telnet_mode();
        td_screens_set_mode(s_shared_mode == TD_TELNET_EXTEND ? TD_SCREENS_EXTEND : TD_SCREENS_DUPLICATE);
        if (td_screen2_attach(screen2_read, screen2_write, NULL))
        {
            s_shared = true;
            return;
        }
        /* No memory for a second screen: the client takes the desktop over. */
    }
    bool remote = active;
    if (remote == s_remote)
        return;
    s_remote = remote;
    if (remote)
    {
        char note[160];
        const char *peer = telnet_peer();
        snprintf(note, sizeof(note),
                 "\x1b[0m\x1b[2J\x1b[H\r\n  The TinyDesk desktop is in use over Telnet%s%s.\r\n"
                 "  It comes back here when that session ends.\r\n",
                 peer ? " from " : "", peer ? peer : "");
        link_write((const uint8_t *)note, (int)strlen(note));
        snprintf(s_local_user, sizeof(s_local_user), "%s", td_session_user());
        s_local_locked = td_session_locked();
        become(telnet_user());
    }
    else
    {
        if (s_local_locked)
            td_session_lock();
        else
            become(s_local_user[0] ? s_local_user : "root");
    }
    td_full_redraw();
}

static int mux_read_byte(void *ctx)
{
    /* The main loop reads until -1 once per pass: poll Telnet on the first
     * read of each pass only. */
    static bool s_drained = true;
    if (s_drained)
    {
        telnet_poll();
        follow_session();
    }
    int b = s_remote ? telnet_read_byte() : local_read_byte(ctx);
    s_drained = (b < 0);
    return b;
}

static int mux_write(void *ctx, const uint8_t *buf, int len)
{
    (void)ctx;
    return s_remote ? telnet_write(buf, len) : link_write(buf, len);
}

static const td_hal_t s_hal = {mux_read_byte, mux_write, hal_millis, hal_sleep, &s_ctx};

const td_hal_t *hal_mux_init(void)
{
    return link_init() ? &s_hal : NULL;
}
