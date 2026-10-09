/*
 * link.h - what a board port provides to the shared ESP-IDF code: the local
 * serial link the desktop is served on, and a few facts about the board.
 *
 *   ESP32-C6, ESP32-S3: link_usj.c in this folder  (built-in USB Serial/JTAG;
 *             ports/esp32c6 and ports/esp32s3)
 *   ESP32:    link_uart.c in this folder  (UART0 through the USB-UART chip;
 *             ports/esp32 and ports/esp32-4mb)
 *
 * hal_mux.c builds the tinydesk HAL on top of it (and hands the desktop to
 * a Telnet client while one is logged in).
 */
#ifndef LINK_H
#define LINK_H

#include <stdbool.h>
#include <stdint.h>

#include "td_modbus.h"

/* Set up the link; false on error. */
bool link_init(void);

/* Bytes received (0 if none; never blocks). */
int link_read(uint8_t *buf, int cap);

/* Bytes accepted (may be fewer than len when nobody is reading). */
int link_write(const uint8_t *buf, int len);

/* Board facts. */
const char *board_platform(void);          /* "ESP32-C6 (USB Serial/JTAG)" */
const char *board_hostname(void);          /* the shell's host name */
const td_mb_serial_t *board_rtu_lines(void);   /* Modbus RTU lines, or NULL */

#endif
