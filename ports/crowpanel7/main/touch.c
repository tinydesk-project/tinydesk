/*
 * touch.c - the CrowPanel 7.0's GT911 touch controller (I2C on GPIO19/20)
 * as the desktop's mouse.
 *
 * A finger is the left button: down presses it on the cell under the
 * finger, moving drags, lifting releases. Clicks are made sure of: a
 * finger stays on the cell it pressed until it has moved DRAG_SLOP_PX
 * (a finger rolls a little, and lifting moves its point), so the release
 * comes on the same cell; and it has only lifted when the controller has
 * reported no finger LIFT_POLLS times running (it can lose a finger for
 * one poll, which would be a release and a second press: a double click).
 * Holding still for LONG_PRESS_MS
 * turns it into the right button (context menus open on the press), held
 * until the finger lifts, so a long hold stays visible (kiosk mode's way
 * out). The reports are the SGR mouse sequences a terminal sends, so the
 * desktop handles them like PuTTY's.
 *
 * Board version 3.0 resets the GT911 through a PCA9557 I/O expander (its
 * IO0 is the reset line, IO1 the interrupt line, which also picks the
 * GT911's address); version 2.0 has no expander.
 */
#include "touch.h"

#include <stdio.h>
#include <stdlib.h>

#include "display.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "esp_private/esp_gpio_reserve.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define PIN_SDA       19
#define PIN_SCL       20
#define I2C_HZ        400000
#define PCA9557_ADDR  0x18
#define GT911_ADDR_A  0x5D
#define GT911_ADDR_B  0x14
#define POLL_MS       15
#define LONG_PRESS_MS 700
#define DRAG_SLOP_PX  14          /* moved this far, a press becomes a drag */
#define LIFT_POLLS    2           /* no finger this many polls running: lifted */
#define TOUCH_PRIO    4
#define TOUCH_STACK   3072

#define GT911_STATUS 0x814E
#define GT911_POINT1 0x8150

static const char *TAG = "touch";
static i2c_master_bus_handle_t s_bus;
static i2c_master_dev_handle_t s_gt911;
static touch_input_fn s_input;

static esp_err_t gt911_read(uint16_t reg, uint8_t *buf, size_t len)
{
    uint8_t addr[2] = {(uint8_t)(reg >> 8), (uint8_t)reg};
    return i2c_master_transmit_receive(s_gt911, addr, 2, buf, len, 50);
}

static esp_err_t gt911_write8(uint16_t reg, uint8_t v)
{
    uint8_t data[3] = {(uint8_t)(reg >> 8), (uint8_t)reg, v};
    return i2c_master_transmit(s_gt911, data, 3, 50);
}

/* Version 3.0: reset the GT911 with INT low, which selects address 0x5D. */
static void pca9557_reset_touch(void)
{
    if (i2c_master_probe(s_bus, PCA9557_ADDR, 50) != ESP_OK)
        return;
    i2c_master_dev_handle_t pca;
    i2c_device_config_t dc = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = PCA9557_ADDR, .scl_speed_hz = I2C_HZ};
    if (i2c_master_bus_add_device(s_bus, &dc, &pca) != ESP_OK)
        return;
    uint8_t out_low[2] = {0x01, 0x00};      /* output register: IO0, IO1 low */
    uint8_t cfg_out[2] = {0x03, 0xFC};      /* configuration: IO0, IO1 outputs */
    uint8_t rst_high[2] = {0x01, 0x01};     /* IO0 (reset) high */
    uint8_t int_in[2] = {0x03, 0xFE};       /* IO1 (interrupt) back to an input */
    i2c_master_transmit(pca, out_low, 2, 50);
    i2c_master_transmit(pca, cfg_out, 2, 50);
    vTaskDelay(pdMS_TO_TICKS(20));
    i2c_master_transmit(pca, rst_high, 2, 50);
    vTaskDelay(pdMS_TO_TICKS(100));
    i2c_master_transmit(pca, int_in, 2, 50);
    vTaskDelay(pdMS_TO_TICKS(50));
    i2c_master_bus_rm_device(pca);
    ESP_LOGI(TAG, "touch reset through the PCA9557 (board v3.0)");
}

/* ------------------------------------------------------------ mouse */

static void report(int button, int col, int row, bool release)
{
    char s[24];
    int n = snprintf(s, sizeof(s), "\x1b[<%d;%d;%d%c", button, col + 1, row + 1, release ? 'm' : 'M');
    s_input(s, n);
}

typedef struct
{
    bool down;          /* finger on the screen */
    bool dragged;       /* moved past DRAG_SLOP_PX since it went down */
    bool long_done;     /* the long press switched to the right button */
    int col, row;       /* cell reported last */
    int x0, y0;         /* where it went down, in pixels */
    int gone;           /* polls running without a finger */
    int64_t since_us;   /* when it went down */
} finger_t;

static void finger(finger_t *f, bool touching, int x, int y)
{
    int cols = display_cols(), rows = display_rows();
    int col = x * cols / DISPLAY_W, row = y * rows / DISPLAY_H;
    if (col >= cols)
        col = cols - 1;
    if (row >= rows)
        row = rows - 1;
    int64_t now = esp_timer_get_time();

    if (touching)
        f->gone = 0;
    else if (f->down && ++f->gone < LIFT_POLLS)
        return;                             /* perhaps only lost for a moment */

    if (touching && !f->down)
    {
        *f = (finger_t){.down = true, .col = col, .row = row, .x0 = x, .y0 = y, .since_us = now};
        report(0, col, row, false);
    }
    else if (touching && f->down && !f->long_done)
    {
        if (!f->dragged && abs(x - f->x0) < DRAG_SLOP_PX && abs(y - f->y0) < DRAG_SLOP_PX)
            col = f->col, row = f->row;     /* still the press: its own cell */
        else
            f->dragged = true;
        if (col != f->col || row != f->row)
        {
            f->col = col;
            f->row = row;
            report(32, col, row, false);    /* motion with the left button down */
        }
        else if (!f->dragged && now - f->since_us >= LONG_PRESS_MS * 1000LL)
        {
            report(0, col, row, true);      /* let go of the left button ... */
            report(2, col, row, false);     /* ... and hold the right one until the finger lifts */
            f->long_done = true;
        }
    }
    else if (!touching && f->down)
    {
        report(f->long_done ? 2 : 0, f->col, f->row, true);
        f->down = false;
    }
}

static void touch_task(void *arg)
{
    (void)arg;
    finger_t f = {0};
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
        uint8_t status = 0;
        if (gt911_read(GT911_STATUS, &status, 1) != ESP_OK)
            continue;
        if (!(status & 0x80))
        {
            /* No new data: keep the last state, but a finger that seemed
             * to lift goes on counting towards LIFT_POLLS. */
            if (f.down && f.gone > 0)
                finger(&f, false, 0, 0);
            continue;
        }
        int points = status & 0x0F;
        uint8_t p[4] = {0};
        if (points > 0)
            gt911_read(GT911_POINT1, p, sizeof(p));
        gt911_write8(GT911_STATUS, 0);
        finger(&f, points > 0, p[0] | (p[1] << 8), p[2] | (p[3] << 8));
    }
}

bool touch_init(touch_input_fn input)
{
    s_input = input;
    i2c_master_bus_config_t bc = {
        .i2c_port = I2C_NUM_1,              /* I2C0 stays free for scripts (machine.I2C(0)) */
        .sda_io_num = PIN_SDA,
        .scl_io_num = PIN_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    if (i2c_new_master_bus(&bc, &s_bus) != ESP_OK)
    {
        ESP_LOGE(TAG, "I2C bus failed");
        return false;
    }
    esp_gpio_reserve(BIT64(PIN_SDA) | BIT64(PIN_SCL));
    pca9557_reset_touch();

    uint16_t addr = 0;
    if (i2c_master_probe(s_bus, GT911_ADDR_A, 50) == ESP_OK)
        addr = GT911_ADDR_A;
    else if (i2c_master_probe(s_bus, GT911_ADDR_B, 50) == ESP_OK)
        addr = GT911_ADDR_B;
    if (!addr)
    {
        ESP_LOGW(TAG, "no GT911 at 0x5D or 0x14: touch is off");
        return false;
    }
    i2c_device_config_t dc = {.dev_addr_length = I2C_ADDR_BIT_LEN_7, .device_address = addr, .scl_speed_hz = I2C_HZ};
    if (i2c_master_bus_add_device(s_bus, &dc, &s_gt911) != ESP_OK)
        return false;
    uint8_t id[4] = {0};
    gt911_read(0x8140, id, sizeof(id));
    ESP_LOGI(TAG, "GT911 at 0x%02X, product id %.4s", addr, (const char *)id);
    return xTaskCreate(touch_task, "touch", TOUCH_STACK, NULL, TOUCH_PRIO, NULL) == pdPASS;
}
