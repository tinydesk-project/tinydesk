/*
 * hal_mux.c - the tinydesk HAL for the ESP-IDF ports, and where the two
 * displays get their terminals (Settings -> Displays, td_display_ops_t):
 *
 *   display 1  the board's local link (link.h), or a logged-in Telnet
 *              client, which then takes the desktop over (the local link
 *              shows a note until it leaves);
 *   display 2  off, a Telnet client (the second one when display 1 is
 *              Telnet too), or a terminal on a second serial port (board
 *              keys display2.*), attached with td_screen2_attach and
 *              duplicating or extending the desktop (td_screens_layout_t).
 *
 * The sources and the layout are kept in NVS ("tinydesk": disp_src1,
 * disp_src2, disp_layout); without them the old Telnet mode (telnet_mode)
 * decides.
 */
#include "hal_mux.h"

#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "link.h"
#include "nvs.h"
#include "td_apps.h"
#include "tdsh_board.h"
#include "telnet.h"
#include "tinydesk/td.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "displays";

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

    /* ------------------------------------------------------------ settings */

static uint8_t s_src[2] = {TD_DISPLAY_BOARD, TD_DISPLAY_OFF};
enum
{
    OLD_TELNET_ONLY,
    OLD_DUPLICATE,
    OLD_EXTEND
};

static uint8_t s_old_mode = OLD_TELNET_ONLY;

static void load_settings(void)
{
    td_screens_layout_t l = {.mode = TD_SCREENS_DUPLICATE, .place = TD_PLACE_RIGHT};
    nvs_handle_t h;
    bool have = false;
    if (nvs_open("tinydesk", NVS_READONLY, &h) == ESP_OK)
    {
        uint8_t a = 0, b = 0;
        size_t len = sizeof(l);
        nvs_get_u8(h, "telnet_mode", &s_old_mode);   /* read before telnet_start() */
        if (nvs_get_u8(h, "disp_src1", &a) == ESP_OK && nvs_get_u8(h, "disp_src2", &b) == ESP_OK)
        {
            s_src[0] = a == TD_DISPLAY_TELNET ? TD_DISPLAY_TELNET : TD_DISPLAY_BOARD;
            s_src[1] = b == TD_DISPLAY_TELNET || b == TD_DISPLAY_SERIAL ? b : TD_DISPLAY_OFF;
            have = true;
        }
        td_screens_layout_t saved;
        if (nvs_get_blob(h, "disp_layout", &saved, &len) == ESP_OK && len == sizeof(saved))
            l = saved;
        nvs_close(h);
    }
    if (!have)
    {
          /* The Telnet mode of 0.2.x builds: Telnet only, Duplicate, Extend. */
        switch (s_old_mode)
        {
        case OLD_DUPLICATE:
        case OLD_EXTEND:
            s_src[0] = TD_DISPLAY_BOARD;
            s_src[1] = TD_DISPLAY_TELNET;
            l.mode = s_old_mode == OLD_EXTEND ? TD_SCREENS_EXTEND : TD_SCREENS_DUPLICATE;
            break;
        default:
            s_src[0] = TD_DISPLAY_TELNET;
            s_src[1] = TD_DISPLAY_OFF;
            break;
        }
    }
    td_screens_set_layout(&l);
}

static void save_u8(const char *key, uint8_t v)
{
    nvs_handle_t h;
    if (nvs_open("tinydesk", NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_set_u8(h, key, v);
        nvs_commit(h);
        nvs_close(h);
    }
}

    /* Telnet client index of each display, or -1. */
static int telnet_client(int display)
{
    if (s_src[display] != TD_DISPLAY_TELNET)
        return -1;
    return display == 1 && s_src[0] == TD_DISPLAY_TELNET ? 1 : 0;
}

    /* ------------------------------------------------- second serial port */

static int s_s2_uart = -2;   /* -2: not read yet, -1: not configured */
static int s_s2_tx, s_s2_rx, s_s2_baud;
static char s_s2_label[48];
static bool s_s2_open;

static void serial2_config(void)
{
    if (s_s2_uart != -2)
        return;
    s_s2_uart = tdsh_board_int("display2.uart", -1);
    s_s2_tx = tdsh_board_int("display2.tx", -1);
    s_s2_rx = tdsh_board_int("display2.rx", -1);
    s_s2_baud = tdsh_board_int("display2.baud", 115200);
    if (s_s2_baud < 9600)
        s_s2_baud = 115200;
    if (s_s2_uart < 1 || s_s2_uart >= UART_NUM_MAX || s_s2_tx < 0 || s_s2_rx < 0)
    {
        s_s2_uart = -1;
        snprintf(s_s2_label, sizeof(s_s2_label), "Serial 2 (set the display2.* keys)");
        return;
    }
    snprintf(s_s2_label, sizeof(s_s2_label), "Serial 2: UART%d, TX %d / RX %d", s_s2_uart, s_s2_tx, s_s2_rx);
}

static bool serial2_open(void)
{
    serial2_config();
    if (s_s2_open || s_s2_uart < 0)
        return s_s2_open;
    uart_config_t cfg = {
        .baud_rate = s_s2_baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_driver_install((uart_port_t)s_s2_uart, 2048, 8192, 0, NULL, 0);
    if (err == ESP_OK)
        err = uart_param_config((uart_port_t)s_s2_uart, &cfg);
    if (err == ESP_OK)
        err = uart_set_pin((uart_port_t)s_s2_uart, s_s2_tx, s_s2_rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "display 2 on UART%d: %s", s_s2_uart, esp_err_to_name(err));
        if (uart_is_driver_installed((uart_port_t)s_s2_uart))
            uart_driver_delete((uart_port_t)s_s2_uart);
        s_s2_uart = -1;   /* in use by something else: not offered again */
        return false;
    }
    s_s2_open = true;
    ESP_LOGI(TAG, "display 2 on UART%d (TX %d, RX %d, %d baud)", s_s2_uart, s_s2_tx, s_s2_rx, s_s2_baud);
    return true;
}

static void serial2_close(void)
{
    if (!s_s2_open)
        return;
    uart_driver_delete((uart_port_t)s_s2_uart);
    s_s2_open = false;
}

static int serial2_read(void)
{
    uint8_t b;
    return s_s2_open && uart_read_bytes((uart_port_t)s_s2_uart, &b, 1, 0) == 1 ? b : -1;
}

static int serial2_write(const uint8_t *buf, int len)
{
    size_t room = 0;
    if (!s_s2_open || uart_get_tx_buffer_free_size((uart_port_t)s_s2_uart, &room) != ESP_OK)
        return 0;
    if ((size_t)len > room)
        len = (int)room;   /* a short write: the desktop paces itself */
    return len > 0 ? uart_write_bytes((uart_port_t)s_s2_uart, buf, (size_t)len) : 0;
}

    /* ---------------------------------------------- following the sources */

static bool s_remote;            /* display 1 is a Telnet client */
static int s_attached_src;       /* display 2's source while attached, else TD_DISPLAY_OFF */
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
    return s_attached_src == TD_DISPLAY_SERIAL ? serial2_read() : telnet_read_byte_n(telnet_client(1));
}

static int screen2_write(void *ctx, const uint8_t *buf, int len)
{
    (void)ctx;
    return s_attached_src == TD_DISPLAY_SERIAL ? serial2_write(buf, len) : telnet_write_n(telnet_client(1), buf, len);
}

    /* Display 2: attached while its source can show it. It sees (and drives)
 * the desktop as it is. */
static void follow_display2(void)
{
    int src = s_src[1];
    bool ready = (src == TD_DISPLAY_TELNET && telnet_active_n(telnet_client(1))) ||
                 (src == TD_DISPLAY_SERIAL && serial2_open());
    if (s_attached_src != TD_DISPLAY_OFF && (!ready || s_attached_src != src))
    {
        s_attached_src = TD_DISPLAY_OFF;
        td_screen2_detach();
    }
    if (src != TD_DISPLAY_SERIAL)
        serial2_close();
    if (ready && s_attached_src == TD_DISPLAY_OFF)
    {
        s_attached_src = src;
        if (!td_screen2_attach(screen2_read, screen2_write, NULL))
            s_attached_src = TD_DISPLAY_OFF;   /* no memory for a second screen */
    }
}

    /* Display 1: a Telnet client takes the desktop over (as the user who
 * logged in) and it comes back to the local link and its user when the
 * client leaves. Only root may log in over Telnet. */
static void follow_display1(void)
{
    int k = telnet_client(0);
    bool remote = k >= 0 && telnet_active_n(k);
    if (remote == s_remote)
        return;
    s_remote = remote;
    if (remote)
    {
        char note[160];
        const char *peer = telnet_peer_n(k);
        snprintf(note, sizeof(note),
                 "\x1b[0m\x1b[2J\x1b[H\r\n  The TinyDesk desktop is in use over Telnet%s%s.\r\n"
                 "  It comes back here when that session ends.\r\n",
                 peer ? " from " : "", peer ? peer : "");
        link_write((const uint8_t *)note, (int)strlen(note));
        snprintf(s_local_user, sizeof(s_local_user), "%s", td_session_user());
        s_local_locked = td_session_locked();
        become(telnet_user_n(k));
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
      /* The main loop reads until -1 once per pass: poll Telnet and the
     * sources on the first read of each pass only. */
    static bool s_drained = true;
    if (s_drained)
    {
        telnet_poll();
        follow_display1();
        follow_display2();
    }
    int b = s_remote ? telnet_read_byte_n(telnet_client(0)) : local_read_byte(ctx);
    s_drained = (b < 0);
    return b;
}

static int mux_write(void *ctx, const uint8_t *buf, int len)
{
    (void)ctx;
    return s_remote ? telnet_write_n(telnet_client(0), buf, len) : link_write(buf, len);
}

static const td_hal_t s_hal = {mux_read_byte, mux_write, hal_millis, hal_sleep, &s_ctx};

    /* ------------------------------------------------------ display ops */

__attribute__((weak)) int board_display_rotation(void)
{
    return -1;   /* a terminal: its window decides */
}

__attribute__((weak)) bool board_set_display_rotation(int degrees)
{
    (void)degrees;
    return false;
}

static bool ops_available(int display, int src)
{
    if (display == 0)
        return src == TD_DISPLAY_BOARD || src == TD_DISPLAY_TELNET;
    if (display != 1)
        return false;
    serial2_config();
    return src == TD_DISPLAY_OFF || src == TD_DISPLAY_TELNET || (src == TD_DISPLAY_SERIAL && s_s2_uart >= 0);
}

static int ops_source(int display)
{
    return display == 0 || display == 1 ? s_src[display] : TD_DISPLAY_OFF;
}

static void ops_set_source(int display, int src)
{
    if (!ops_available(display, src) || !td_session_has_right(TD_RIGHT_SYSTEM))
        return;
    s_src[display] = (uint8_t)src;
    save_u8("disp_src1", s_src[0]);
    save_u8("disp_src2", s_src[1]);
    telnet_set_clients(s_src[0] == TD_DISPLAY_TELNET && s_src[1] == TD_DISPLAY_TELNET ? 2 : 1);
}

static const char *ops_label(int src)
{
    switch (src)
    {
    case TD_DISPLAY_BOARD:
        return board_platform();
    case TD_DISPLAY_TELNET:
        return "Telnet (port 23)";
    case TD_DISPLAY_SERIAL:
        serial2_config();
        return s_s2_label;
    default:
        return "Off";
    }
}

static void ops_status(int display, char *buf, int cap)
{
    int k = telnet_client(display);
    if (k >= 0)
    {
        if (!telnet_enabled())
            snprintf(buf, cap, "Telnet is switched off (Network)");
        else if (telnet_active_n(k))
            snprintf(buf, cap, "%s (%s)", telnet_peer_n(k) ? telnet_peer_n(k) : "?", telnet_user_n(k));
        else
            snprintf(buf, cap, "%s", display == 0 ? "the board's link until a client logs in" : "waiting for a client");
        return;
    }
    if (display == 1 && s_src[1] == TD_DISPLAY_SERIAL)
    {
        td_display_info_t info;
        bool answered = td_screens_display(1, &info) && info.term_cols > 0;
        snprintf(buf, cap, "%s", !s_s2_open ? "the port could not be opened" : answered ? "connected"
                                                                                        : "no terminal answers yet");
        return;
    }
    snprintf(buf, cap, "%s", display == 0 ? "connected" : "off");
}

static void ops_save_layout(const td_screens_layout_t *l)
{
    nvs_handle_t h;
    if (!td_session_has_right(TD_RIGHT_SYSTEM) || nvs_open("tinydesk", NVS_READWRITE, &h) != ESP_OK)
        return;
    nvs_set_blob(h, "disp_layout", l, sizeof(*l));
    nvs_commit(h);
    nvs_close(h);
}

static int ops_rotation(int display)
{
    return display == 0 && s_src[0] == TD_DISPLAY_BOARD ? board_display_rotation() : -1;
}

static bool ops_set_rotation(int display, int degrees)
{
    return display == 0 && td_session_has_right(TD_RIGHT_SYSTEM) && board_set_display_rotation(degrees);
}

static const td_display_ops_t s_display_ops = {
    .source_available = ops_available,
    .source = ops_source,
    .set_source = ops_set_source,
    .source_label = ops_label,
    .status = ops_status,
    .save_layout = ops_save_layout,
    .rotation = ops_rotation,
    .set_rotation = ops_set_rotation,
};

const td_display_ops_t *hal_mux_displays(void)
{
    return &s_display_ops;
}

const td_hal_t *hal_mux_init(void)
{
    if (!link_init())
        return NULL;
    load_settings();
    telnet_set_clients(s_src[0] == TD_DISPLAY_TELNET && s_src[1] == TD_DISPLAY_TELNET ? 2 : 1);
    return &s_hal;
}
