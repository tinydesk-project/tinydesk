/*
 * link_uart.c - the classic ESP32 boards: the desktop on a UART, normally
 * UART0 through the dev board's USB-UART chip (CP2102/CH340).
 *
 * Board configuration keys (tdsh_board.h; the Board configuration page of the docs):
 *   console.uart  UART number (default 0)
 *   console.tx    TX GPIO (default: the UART's usual pin, GPIO1 for UART0)
 *   console.rx    RX GPIO (default: the UART's usual pin, GPIO3 for UART0)
 *   console.baud  line speed (default 921600)
 *
 * 8N1, no flow control: set the same speed in the terminal. At 921600 a
 * full 80x25 redraw takes about 0.2 s (115200 would take about 2 s).
 * RS-485 lines come from the rs485.* keys (rs485.c), on other UARTs.
 */
#include "link.h"

#include <stdio.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "rs485.h"
#include "tdsh_board.h"

#define LINK_UART    s_uart
#define DEFAULT_BAUD 921600
#define RX_BUFFER    8192          /* a paste arrives at ~90 KB/s; hold one while a frame is drawn */
#define TX_BUFFER    4096          /* one frame; the UART drains it in ~45 ms */

static const char *TAG = "link_uart";
static uart_port_t s_uart = UART_NUM_0;
static int s_baud = DEFAULT_BAUD;
static char s_platform[48] = "ESP32 (UART0, 921600 baud)";   /* updated by link_init() */

bool link_init(void)
{
    int u = tdsh_board_int("console.uart", 0);
    s_uart = u >= 0 && u < UART_NUM_MAX ? (uart_port_t)u : UART_NUM_0;
    s_baud = tdsh_board_int("console.baud", DEFAULT_BAUD);
    if (s_baud < 9600)
        s_baud = DEFAULT_BAUD;
    int tx = tdsh_board_int("console.tx", -1), rx = tdsh_board_int("console.rx", -1);
#if CONFIG_IDF_TARGET_ESP32S3
    snprintf(s_platform, sizeof(s_platform), "ESP32-S3 (UART%d, %d baud)", (int)s_uart, s_baud);
#else
    snprintf(s_platform, sizeof(s_platform), "ESP32 (UART%d, %d baud)", (int)s_uart, s_baud);
#endif
    uart_config_t cfg = {
        .baud_rate = s_baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_driver_install(LINK_UART, RX_BUFFER, TX_BUFFER, 0, NULL, 0);
    if (err == ESP_OK)
        err = uart_param_config(LINK_UART, &cfg);
    if (err == ESP_OK)
        err = uart_set_pin(LINK_UART, tx >= 0 ? tx : UART_PIN_NO_CHANGE, rx >= 0 ? rx : UART_PIN_NO_CHANGE,
                           UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);   /* default: TX1 / RX3 on UART0 */
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "UART%d setup failed: %s", (int)s_uart, esp_err_to_name(err));
        return false;
    }
    return true;
}

int link_read(uint8_t *buf, int cap)
{
    size_t avail = 0;
    if (uart_get_buffered_data_len(LINK_UART, &avail) != ESP_OK || avail == 0)
        return 0;
    if (avail > (size_t)cap)
        avail = (size_t)cap;
    int n = uart_read_bytes(LINK_UART, buf, (uint32_t)avail, 0);
    return n > 0 ? n : 0;
}

int link_write(const uint8_t *buf, int len)
{
    /* The UART always drains (no flow control), so this only waits while the
     * TX buffer is full. */
    int n = uart_write_bytes(LINK_UART, buf, (size_t)len);
    return n > 0 ? n : 0;
}

const char *board_platform(void)
{
    return s_platform;
}
const char *board_hostname(void)
{
#if CONFIG_IDF_TARGET_ESP32S3
    return "esp32s3";
#else
    return "esp32";
#endif
}
const td_mb_serial_t *board_rtu_lines(void)
{
    return rs485_serial();
}
