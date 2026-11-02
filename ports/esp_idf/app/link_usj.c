/*
 * link_usj.c - the desktop on the chip's built-in USB Serial/JTAG port
 * (ports/esp32c6, ports/esp32s3), and Modbus RTU on the RS-485 lines of the
 * board configuration.
 *
 * The port is owned exclusively by the desktop (the ESP-IDF console is
 * off). A write waits up to WRITE_TIMEOUT_MS for room: long enough that a
 * terminal which reads slowly (PuTTY while it draws) paces the desktop
 * instead of losing frames, short enough that with no terminal reading the
 * USB FIFO fills up and frames are dropped instead of stalling the UI.
 */
#include "link.h"

#include "driver/usb_serial_jtag.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "rs485.h"

#define RX_BUFFER        8192         /* a full ring drops USB packets; hold a paste while a frame is drawn */
#define TX_BUFFER        1024
#define WRITE_CHUNK      256          /* the driver queues whole chunks or nothing */
#define WRITE_TIMEOUT_MS 150          /* was 20: PuTTY dropped frames at full speed */

#if CONFIG_IDF_TARGET_ESP32S3
#define CHIP_NAME "ESP32-S3"
#else
#define CHIP_NAME "ESP32-C6"
#endif

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
    return CHIP_NAME " (USB Serial/JTAG)";
}
const char *board_hostname(void)
{
    return CONFIG_IDF_TARGET; /* "esp32c6", "esp32s3" */
}
const td_mb_serial_t *board_rtu_lines(void)
{
    return rs485_serial();
}
