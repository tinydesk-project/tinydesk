/*
 * usb_hid.h - a USB keyboard and mouse on the ESP32-S3's own USB port
 * (USB host, GPIO19/20), for ports that build usb_hid.c (TD_USB_HID). Their
 * input and settings are hid_input.h's, shared with Bluetooth devices.
 */
#ifndef USB_HID_H
#define USB_HID_H

/* Start the USB host when board key usb.host is 1. */
void usb_hid_start(void);

#endif
