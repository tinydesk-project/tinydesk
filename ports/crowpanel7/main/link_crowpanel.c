/*
 * link_crowpanel.c - the Elecrow CrowPanel 7.0 (ESP32-S3-WROOM-1-N4R8):
 * the desktop on the board's 800x480 touch LCD, with the keyboard on UART0.
 *
 * Output goes to the LCD (display.c) and, unless lcd.mirror is 0, also to
 * UART0, so PuTTY shows the same picture (set its window to 100x30).
 * Input is UART0 (the keyboard, and PuTTY's mouse) and the touch screen
 * (touch.c). The desktop's size is the LCD's: its answers to the size
 * query are the only ones the desktop gets; PuTTY's are dropped.
 *
 * Board configuration keys (the Board configuration page of the docs):
 *   console.uart  0 (default): UART0 is the keyboard; -1: TinyDesk leaves
 *                 UART0 alone (for a kiosk app's Modbus line, say)
 *   console.baud  UART0 speed (default 921600)
 *   lcd.mirror    1 (default): also send the picture to UART0; 0: LCD only
 */
#include "link.h"

#include <stdio.h>
#include <string.h>

#include "display.h"
#include "tinydesk/td_picture.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rs485.h"
#include "tdsh_board.h"
#include "touch.h"

#define LINK_UART    UART_NUM_0
#define DEFAULT_BAUD 921600
#define RX_BUFFER    8192          /* a paste arrives at ~90 KB/s; hold one while a frame is drawn */
#define TX_BUFFER    4096
#define INJECT_SIZE  512           /* the LCD's answers and touch reports, waiting for link_read */
#define HOLD_US      30000         /* a lone ESC from UART0 waits this long to see if a size reply follows */

static const char *TAG = "link_crowpanel";
static bool s_mirror = true;
static bool s_uart = true;       /* UART0 is the keyboard (console.uart 0) */
static bool s_touch;
static char s_platform[64] = "ESP32-S3 CrowPanel 7.0";

/* ------------------------------------------ input from the LCD and touch */

static portMUX_TYPE s_inject_mux = portMUX_INITIALIZER_UNLOCKED;
static char s_inject[INJECT_SIZE];
static int s_inject_head, s_inject_tail;

static void inject(const char *data, int len)
{
    taskENTER_CRITICAL(&s_inject_mux);
    for (int i = 0; i < len; i++)
    {
        int next = (s_inject_head + 1) % INJECT_SIZE;
        if (next == s_inject_tail)
            break;                 /* full: drop the rest */
        s_inject[s_inject_head] = data[i];
        s_inject_head = next;
    }
    taskEXIT_CRITICAL(&s_inject_mux);
}

static int take_injected(uint8_t *buf, int cap)
{
    int n = 0;
    taskENTER_CRITICAL(&s_inject_mux);
    while (n < cap && s_inject_tail != s_inject_head)
    {
        buf[n++] = (uint8_t)s_inject[s_inject_tail];
        s_inject_tail = (s_inject_tail + 1) % INJECT_SIZE;
    }
    taskEXIT_CRITICAL(&s_inject_mux);
    return n;
}

/* ------------------------------------- UART0 without PuTTY's size replies */

/* A cursor position report, ESC [ rows ; cols R, is PuTTY answering the
 * desktop's size query: the desktop must see the LCD's size instead. Bytes
 * that might start one are held until it is clear. */
static uint8_t s_held[16];
static int s_held_len;
static int64_t s_held_since;

static bool could_be_report(const uint8_t *s, int n)
{
    if (n >= 1 && s[0] != 0x1b)
        return false;
    if (n >= 2 && s[1] != '[')
        return false;
    for (int i = 2; i < n; i++)
        if (!((s[i] >= '0' && s[i] <= '9') || s[i] == ';' || (s[i] == 'R' && i == n - 1 && i > 2)))
            return false;
    return true;
}

/* Filter in place; returns the bytes kept. */
static int filter_reports(uint8_t *buf, int n, int cap)
{
    uint8_t out[RX_BUFFER < 256 ? RX_BUFFER : 256];
    int o = 0;
    for (int i = 0; i < n; i++)
    {
        if (s_held_len == 0 && buf[i] != 0x1b)
        {
            out[o++] = buf[i];
            continue;
        }
        if (s_held_len == 0)
            s_held_since = esp_timer_get_time();
        s_held[s_held_len++] = buf[i];
        if (!could_be_report(s_held, s_held_len) || s_held_len == (int)sizeof(s_held))
        {
            /* Not a report: let the bytes through, a new ESC starts over. */
            int keep_last = (buf[i] == 0x1b && s_held_len > 1);
            int pass = s_held_len - keep_last;
            memcpy(out + o, s_held, (size_t)pass);
            o += pass;
            s_held_len = 0;
            if (keep_last)
            {
                s_held[s_held_len++] = 0x1b;
                s_held_since = esp_timer_get_time();
            }
        }
        else if (s_held[s_held_len - 1] == 'R')
        {
            s_held_len = 0;        /* a whole report: dropped */
        }
    }
    /* A lone ESC (the Esc key) or a cut-off sequence: give it out after a while. */
    if (s_held_len && esp_timer_get_time() - s_held_since > HOLD_US)
    {
        memcpy(out + o, s_held, (size_t)s_held_len);
        o += s_held_len;
        s_held_len = 0;
    }
    if (o > cap)
        o = cap;
    memcpy(buf, out, (size_t)o);
    return o;
}

static int uart_take(uint8_t *buf, int cap)
{
    if (cap > 200)
        cap = 200;                 /* room for held bytes in filter_reports */
    size_t avail = 0;
    int n = 0;
    if (uart_get_buffered_data_len(LINK_UART, &avail) == ESP_OK && avail > 0)
    {
        if (avail > (size_t)cap)
            avail = (size_t)cap;
        n = uart_read_bytes(LINK_UART, buf, (uint32_t)avail, 0);
        if (n < 0)
            n = 0;
    }
    if (n == 0 && s_held_len == 0)
        return 0;
    return filter_reports(buf, n, cap);
}

/* --------------------------------------------------------------- link */

bool link_init(void)
{
    int baud = tdsh_board_int("console.baud", DEFAULT_BAUD);
    if (baud < 9600)
        baud = DEFAULT_BAUD;
    s_mirror = tdsh_board_int("lcd.mirror", 1) != 0;
    /* console.uart = -1: UART0 is not the keyboard (a kiosk app's Modbus
     * line, for example): TinyDesk leaves it alone, except on a
     * maintenance start (BOOT held after RESET, main.c). */
    s_uart = tdsh_board_int("console.uart", 0) == 0 || board_maintenance_start();
    if (s_uart)
    {
        uart_config_t cfg = {
            .baud_rate = baud,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_DEFAULT,
        };
        esp_err_t err = uart_driver_install(LINK_UART, RX_BUFFER, TX_BUFFER, 0, NULL, 0);
        if (err == ESP_OK)
            err = uart_param_config(LINK_UART, &cfg);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "UART0 setup failed: %s", esp_err_to_name(err));
            return false;
        }
    }
    if (!display_init(inject))
    {
        ESP_LOGE(TAG, "the LCD did not start");
        return false;
    }
    td_picture_set_sink(!s_uart || !s_mirror ? display_picture : NULL, NULL);
    td_picture_set_rgb_sink(!s_uart || !s_mirror ? display_picture_rgb : NULL,NULL);
    td_picture_set_runs_sink(!s_uart || !s_mirror ? display_picture_runs : NULL,NULL);
    td_picture_set_scanout_monitor(display_scanout_snapshot);
    s_touch = touch_init(inject);
    if (s_uart)
        snprintf(s_platform, sizeof(s_platform), "ESP32-S3 CrowPanel 7.0 (LCD%s, UART0 %d baud)",
                 s_touch ? " + touch" : "", baud);
    else
        snprintf(s_platform, sizeof(s_platform), "ESP32-S3 CrowPanel 7.0 (LCD%s)", s_touch ? " + touch" : "");
    return true;
}

int link_read(uint8_t *buf, int cap)
{
    int n = take_injected(buf, cap);
    if (n > 0 || !s_uart)
        return n;
    return uart_take(buf, cap);
}

int link_write(const uint8_t *buf, int len)
{
    display_write(buf, len);
    if (s_uart && s_mirror)
        uart_write_bytes(LINK_UART, buf, (size_t)len);
    return len;
}

const char *board_platform(void)
{
    return s_platform;
}
const char *board_hostname(void)
{
    return "crowpanel";
}
const td_mb_serial_t *board_rtu_lines(void)
{
    return rs485_serial();
}
