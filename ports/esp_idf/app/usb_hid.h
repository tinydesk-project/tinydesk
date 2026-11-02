/*
 * usb_hid.h - a USB keyboard and mouse on the ESP32-S3's own USB port
 * (USB host, GPIO19/20), for ports that build usb_hid.c (TD_USB_HID).
 *
 * Keys arrive as the bytes a terminal sends (PuTTY's), and the mouse as
 * SGR mouse reports, so the desktop reads them like the serial link's own
 * input; link_read() mixes them in. While a mouse is attached the desktop
 * draws the pointer itself (td_wm_set_pointer).
 */
#ifndef USB_HID_H
#define USB_HID_H

#include <stdint.h>

#include "tinydesk/td_sysinfo.h"

/* Start the USB host when board key usb.host is 1. */
void usb_hid_start(void);

/* Bytes from the keyboard and mouse (0 if none; never blocks). */
int usb_hid_read(uint8_t *buf, int cap);

/* The mouse settings for the Mouse app (td_sysinfo_t.mouse), or NULL when
 * the USB host is off. */
const td_mouse_ops_t *usb_hid_mouse_ops(void);

/* The keyboard settings for the Keyboard app (td_sysinfo_t.keyboard), or
 * NULL when the USB host is off. */
const td_keyboard_ops_t *usb_hid_keyboard_ops(void);

#endif
