/*
 * usb_hid.c - a USB keyboard and mouse on the ESP32-S3's own USB port.
 *
 * ESP-IDF's USB host library and Espressif's HID host driver
 * (espressif/usb_host_hid) find keyboards and mice; their reports go to
 * hid_input.c, which turns them into the desktop's input (keys in the
 * chosen layout, the pointer) for USB and Bluetooth devices alike. Mice
 * report in their own format, read from their report descriptor so that the
 * wheel works, with boot reports as the fallback. ESP-IDF 5.3 has no hub
 * support: one device on the port, such as a wireless keyboard and mouse
 * set with one receiver.
 *
 * Board configuration key (the Board configuration page of the docs):
 *   usb.host   1: USB host on GPIO19/20 (default 0)
 * The usb.mouse_* and usb.kbd_* settings are hid_input.c's.
 */
#include "usb_hid.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "hid_input.h"
#include "tdsh_board.h"
#include "usb/hid_host.h"
#include "usb/usb_host.h"

#define TICK_MS 20

static const char *TAG = "usb_hid";
static hid_mouse_layout_t s_layout;   /* the mouse's reports */

/* ------------------------------------------------------ the HID driver */

typedef struct
{
    hid_host_device_handle_t dev;
    hid_host_driver_event_t event;
} dev_event_t;

static QueueHandle_t s_events;

static void interface_cb(hid_host_device_handle_t dev, const hid_host_interface_event_t event, void *arg)
{
    (void)arg;
    hid_host_dev_params_t p;
    if (hid_host_device_get_params(dev, &p) != ESP_OK)
        return;
    switch (event)
    {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
    {
        uint8_t data[64];
        size_t len = 0;
        if (hid_host_device_get_raw_input_report_data(dev, data, sizeof(data), &len) != ESP_OK ||
            p.sub_class != HID_SUBCLASS_BOOT_INTERFACE)
            return;
        if (p.proto == HID_PROTOCOL_KEYBOARD)
            hid_kbd_report(data, len);
        else if (p.proto == HID_PROTOCOL_MOUSE)
            hid_mouse_report(&s_layout, data, len);
        break;
    }
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "%s disconnected", p.proto == HID_PROTOCOL_MOUSE ? "mouse" : "keyboard");
        if (p.sub_class == HID_SUBCLASS_BOOT_INTERFACE && p.proto == HID_PROTOCOL_MOUSE)
            hid_mouse_detach();
        if (p.sub_class == HID_SUBCLASS_BOOT_INTERFACE && p.proto == HID_PROTOCOL_KEYBOARD)
            hid_kbd_detach();
        hid_host_device_close(dev);
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        ESP_LOGW(TAG, "transfer error");
        break;
    default:
        break;
    }
}

static void device_cb(hid_host_device_handle_t dev, const hid_host_driver_event_t event, void *arg)
{
    (void)arg;
    dev_event_t e = {.dev = dev, .event = event};
    xQueueSend(s_events, &e, 0);
}

/* Opens a new interface (from hid_task: the class requests block). */
static void open_interface(hid_host_device_handle_t dev)
{
    hid_host_dev_params_t p;
    if (hid_host_device_get_params(dev, &p) != ESP_OK)
        return;
    const hid_host_device_config_t cfg = {.callback = interface_cb, .callback_arg = NULL};
    if (hid_host_device_open(dev, &cfg) != ESP_OK)
        return;
    bool boot = p.sub_class == HID_SUBCLASS_BOOT_INTERFACE;
    bool wheel = false;
    if (boot && p.proto == HID_PROTOCOL_MOUSE)
    {
        /* Its own reports (with the wheel) when its descriptor describes
         * them, boot reports otherwise. */
        size_t len = 0;
        const uint8_t *desc = hid_host_get_report_descriptor(dev, &len);
        bool own = desc && hid_mouse_parse(&s_layout, desc, len);
        hid_class_request_set_protocol(dev, own ? HID_REPORT_PROTOCOL_REPORT : HID_REPORT_PROTOCOL_BOOT);
        wheel = own && s_layout.wheel.size;
        if (own)
            ESP_LOGI(TAG, "mouse reports: id %d, X %d+%d, Y %d+%d, wheel %d+%d", s_layout.x.id, s_layout.x.pos,
                     s_layout.x.size, s_layout.y.pos, s_layout.y.size, s_layout.wheel.pos, s_layout.wheel.size);
    }
    else if (boot)
    {
        hid_class_request_set_protocol(dev, HID_REPORT_PROTOCOL_BOOT);
        if (p.proto == HID_PROTOCOL_KEYBOARD)
            hid_class_request_set_idle(dev, 0, 0);
    }
    if (hid_host_device_start(dev) != ESP_OK)
    {
        hid_host_device_close(dev);
        return;
    }
    const char *what = !boot                              ? "other HID interface (not used)"
                       : p.proto == HID_PROTOCOL_KEYBOARD ? "keyboard"
                       : p.proto == HID_PROTOCOL_MOUSE    ? "mouse"
                                                          : "HID interface (not used)";
    ESP_LOGI(TAG, "%s connected (interface %d%s)", what, p.iface_num,
             !boot || p.proto != HID_PROTOCOL_MOUSE ? ""
             : wheel                                ? ", with its wheel"
             : s_layout.ok                          ? ", no wheel"
                                                    : ", boot reports");
    if (boot && p.proto == HID_PROTOCOL_KEYBOARD)
        hid_kbd_attach("USB keyboard");
    if (boot && p.proto == HID_PROTOCOL_MOUSE)
        hid_mouse_attach(wheel ? "USB mouse, with its wheel" : "USB mouse, no wheel");
}

static void hid_task(void *arg)
{
    (void)arg;
    for (;;)
    {
        dev_event_t e;
        if (xQueueReceive(s_events, &e, pdMS_TO_TICKS(TICK_MS)) == pdTRUE &&
            e.event == HID_HOST_DRIVER_EVENT_CONNECTED)
            open_interface(e.dev);
    }
}

static void usb_lib_task(void *arg)
{
    (void)arg;
    for (;;)
    {
        uint32_t flags = 0;
        usb_host_lib_handle_events(portMAX_DELAY, &flags);
        if (flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS)
            usb_host_device_free_all();
    }
}

/* A client of its own that logs every device on the port (its IDs and
 * interface classes), HID or not, so the Log Viewer tells "nothing there"
 * (no power on the port, say) from "there, but not a keyboard or mouse". */
static usb_host_client_handle_t s_client;
static volatile uint8_t s_new_addr;

static void client_cb(const usb_host_client_event_msg_t *msg, void *arg)
{
    (void)arg;
    if (msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV)
        s_new_addr = msg->new_dev.address;
    else if (msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE)
        ESP_LOGI(TAG, "USB device removed");
}

static void log_device(uint8_t addr)
{
    usb_device_handle_t dev;
    if (usb_host_device_open(s_client, addr, &dev) != ESP_OK)
        return;
    const usb_device_desc_t *d;
    const usb_config_desc_t *cfg;
    if (usb_host_get_device_descriptor(dev, &d) == ESP_OK)
        ESP_LOGI(TAG, "USB device %04x:%04x, class %d", d->idVendor, d->idProduct, d->bDeviceClass);
    if (usb_host_get_active_config_descriptor(dev, &cfg) == ESP_OK)
    {
        int offset = 0;
        const usb_standard_desc_t *h = (const usb_standard_desc_t *)cfg;
        while ((h = usb_parse_next_descriptor_of_type(h, cfg->wTotalLength, USB_B_DESCRIPTOR_TYPE_INTERFACE,
                                                      &offset)) != NULL)
        {
            const usb_intf_desc_t *i = (const usb_intf_desc_t *)h;
            ESP_LOGI(TAG, "  interface %d: class %d, subclass %d, protocol %d", i->bInterfaceNumber,
                     i->bInterfaceClass, i->bInterfaceSubClass, i->bInterfaceProtocol);
        }
    }
    usb_host_device_close(s_client, dev);
}

static void client_task(void *arg)
{
    (void)arg;
    const usb_host_client_config_t cfg = {
        .is_synchronous = false,
        .max_num_event_msg = 5,
        .async = {.client_event_callback = client_cb, .callback_arg = NULL},
    };
    if (usb_host_client_register(&cfg, &s_client) != ESP_OK)
    {
        vTaskDelete(NULL);
        return;
    }
    for (;;)
    {
        usb_host_client_handle_events(s_client, portMAX_DELAY);
        uint8_t addr = s_new_addr;
        if (addr)
        {
            s_new_addr = 0;
            log_device(addr);
        }
    }
}

void usb_hid_start(void)
{
    if (tdsh_board_int("usb.host", 0) != 1)
        return;
    hid_input_init();

    const usb_host_config_t host = {.skip_phy_setup = false, .intr_flags = ESP_INTR_FLAG_LEVEL1};
    esp_err_t err = usb_host_install(&host);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "USB host: %s", esp_err_to_name(err));
        return;
    }
    xTaskCreate(usb_lib_task, "usb_lib", 4096, NULL, 2, NULL);
    xTaskCreate(client_task, "usb_log", 3072, NULL, 3, NULL);
    s_events = xQueueCreate(8, sizeof(dev_event_t));
    const hid_host_driver_config_t drv = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 0,
        .callback = device_cb,
        .callback_arg = NULL,
    };
    err = hid_host_install(&drv);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "HID host: %s", esp_err_to_name(err));
        return;
    }
    xTaskCreate(hid_task, "usb_hid", 4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "USB host on GPIO19/20: keyboard and mouse");
}
