/*
 * hid_input.h - keyboards and mice the board reads itself, from any
 * transport: USB on the ESP32-S3 (usb_hid.c) and Bluetooth LE on every
 * board (bt_hid.c). Their HID reports become the bytes a terminal sends
 * (PuTTY's keys, SGR mouse reports), which the desktop reads with the
 * board link's own input (hal_mux.c); while a mouse is attached the
 * desktop draws the pointer itself (td_wm_set_pointer).
 */
#ifndef HID_INPUT_H
#define HID_INPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "tinydesk/td_sysinfo.h"

/* Read the usb.mouse_* and usb.kbd_* settings and start key repeat. Safe to
 * call more than once. */
void hid_input_init(void);

/* Bytes from the keyboards and mice (0 if none; never blocks). */
int hid_input_read(uint8_t *buf, int cap);

/* A keyboard came or went; its boot report (modifiers, a reserved byte,
 * six key usages). name: what the Keyboard app shows ("USB keyboard"). */
void hid_kbd_attach(const char *name);
void hid_kbd_detach(void);
void hid_kbd_report(const uint8_t *r, size_t len);

/* Where a mouse puts its buttons, movement and wheel in its reports, read
 * from its report descriptor; one per mouse. */
typedef struct
{
    uint16_t pos;   /* bit offset after the report ID */
    uint8_t size;   /* bits */
    uint8_t id;     /* report ID (0: none) */
    bool sign;
} hid_field_t;

typedef struct
{
    bool ok, ids;   /* fields found; reports start with an ID */
    hid_field_t button[3], x, y, wheel;
} hid_mouse_layout_t;

/* Fill layout from a report descriptor; false when it has no X and Y (then
 * the mouse's reports are read as boot reports: buttons, X, Y). */
bool hid_mouse_parse(hid_mouse_layout_t *layout, const uint8_t *desc, size_t len);

/* A mouse came or went (name: "USB mouse, with its wheel"); one of its
 * input reports, with its report ID first when the descriptor has IDs.
 * layout NULL or not ok: a boot report. */
void hid_mouse_attach(const char *name);
void hid_mouse_detach(void);
void hid_mouse_report(const hid_mouse_layout_t *layout, const uint8_t *r, size_t len);

/* The Mouse and Keyboard apps' settings (td_sysinfo_t.mouse, .keyboard), or
 * NULL before hid_input_init(). */
const td_mouse_ops_t *hid_input_mouse_ops(void);
const td_keyboard_ops_t *hid_input_keyboard_ops(void);

#endif
