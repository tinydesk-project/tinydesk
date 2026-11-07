/*
 * link_usj.c - the Arduino Nano ESP32: the desktop on the ESP32-S3's
 * built-in USB Serial/JTAG port, which is what the board's USB-C connector
 * is wired to (GPIO19 D-, GPIO20 D+). There is no USB-UART bridge chip on
 * this board, so UART0 carries nothing.
 *
 * The port is owned exclusively by the desktop (the ESP-IDF console is
 * off). Writes use a short timeout: when no terminal is reading, the USB
 * FIFO fills up and frames are dropped instead of stalling the UI.
 */
#include "link.h"

#include "driver/usb_serial_jtag.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "rs485.h"

#define RX_BUFFER        8192         /* a full ring drops USB packets; hold a paste while a frame is drawn */
#define TX_BUFFER        16384        /* one full 100x40 frame is about 15 KB with its escape sequences */
#define WRITE_CHUNK      256          /* the driver queues whole chunks or nothing */
/* A terminal that drains slowly (minicom repaints through its own screen
 * handling) would otherwise cut frames short: a dropped frame is a frame
 * missing its right edge, which reads as flicker. Wait long enough for a
 * frame to leave the FIFO, but not so long that a terminal that has stopped
 * reading stalls the UI indefinitely. */
#define WRITE_TIMEOUT_MS 200

static const char *TAG = "link_usj";

bool link_init(void)
{
    usb_serial_jtag_driver_config_t cfg = {
        .rx_buffer_size = RX_BUFFER,
        .tx_buffer_size = TX_BUFFER,
    };
    esp_err_t err = usb_serial_jtag_driver_install(&cfg);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "usb_serial_jtag_driver_install failed: %s", esp_err_to_name(err));
        return false;
    }
    return true;
}

int link_read(uint8_t *buf, int cap)
{
    int n = usb_serial_jtag_read_bytes(buf, (uint32_t)cap, 0);
    return n > 0 ? n : 0;
}

int link_write(const uint8_t *buf, int len)
{
    /* No USB host at all (power-only cable, PC asleep): don't wait. */
    if (!usb_serial_jtag_is_connected())
        return 0;
    int done = 0;
    while (done < len)
    {
        int n = len - done > WRITE_CHUNK ? WRITE_CHUNK : len - done;
        if (usb_serial_jtag_write_bytes(buf + done, (size_t)n, pdMS_TO_TICKS(WRITE_TIMEOUT_MS)) != n)
            break;
        done += n;
    }
    return done;
}

const char *board_platform(void)
{
    return "Arduino Nano ESP32 (USB Serial/JTAG)";
}
const char *board_hostname(void)
{
    return "nanoesp32";
}
const td_mb_serial_t *board_rtu_lines(void)
{
    return rs485_serial();
}
