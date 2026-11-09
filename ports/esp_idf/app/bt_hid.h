/*
 * bt_hid.h - Bluetooth LE keyboards and mice (bt_hid.c), on every ESP32
 * board, behind td_bt_ops_t (td_sysinfo.h).
 */
#ifndef BT_HID_H
#define BT_HID_H

#include "tinydesk/td_sysinfo.h"

/* Board key bt.enable = 1: set up Bluetooth and return its table (the radio
 * starts at once only when a paired device reconnects; else on a scan).
 * Otherwise NULL, and the controller's memory goes back to the heap. */
const td_bt_ops_t *bt_hid_start(void);

#endif
