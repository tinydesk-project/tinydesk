/*
 * bt_hid_bluedroid.c - Bluetooth Classic and LE keyboards and mice on the
 * classic ESP32 with PSRAM (ports/esp32, the WROVER), behind td_bt_ops_t
 * (td_sysinfo.h), like bt_hid.c (NimBLE, LE only) on the other boards.
 *
 * ESP-IDF's Bluedroid in dual mode with its HID host (esp_hidh), which
 * opens Classic (BR/EDR) and LE HID devices and hands over their input
 * reports; those go to hid_input.c, the decoder USB devices use too. A
 * pairing scan runs a Classic inquiry and an LE scan together. Pairing:
 * Secure Simple Pairing with a passkey the keyboard types (shown in the
 * app's message), a random PIN for older keyboards that ask for one, LE
 * with a passkey likewise. Paired Classic devices reconnect by themselves
 * (the board stays connectable); paired LE devices are found by a light
 * background scan. Slots and the stack's own bonds are kept in NVS.
 *
 * Board configuration key: bt.enable (as bt_hid.c). The radio starts on the
 * first scan, or at start when a slot has a device that reconnects.
 */
#include "bt_hid.h"

#include "sdkconfig.h"

#if CONFIG_BT_BLUEDROID_ENABLED && CONFIG_BT_CLASSIC_ENABLED

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_gap_bt_api.h"
#include "esp_heap_caps.h"
#include "esp_hidh.h"
#include "esp_hidh_gattc.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hid_input.h"
#include "nvs.h"
#include "tdsh_board.h"

static const char *TAG = "bt_hid";

#define SCAN_MS  8000
#define MIN_HEAP 40000   /* internal heap a start needs */
#define UUID_HID 0x1812

/* ------------------------------------------------------------ the slots */

typedef struct
{
    uint8_t used, reconnect, addr_type, classic;
    uint8_t addr[6];   /* most significant byte first, as Bluedroid has it */
    char name[TD_BT_NAME_LEN];
} saved_slot_t;

static saved_slot_t s_slots[TD_BT_SLOTS];

static void slots_load(void)
{
    nvs_handle_t h;
    size_t n = sizeof(s_slots);
    if (nvs_open("bt", NVS_READONLY, &h) == ESP_OK)
    {
        if (nvs_get_blob(h, "slots_bd", s_slots, &n) != ESP_OK || n != sizeof(s_slots))
            memset(s_slots, 0, sizeof(s_slots));
        nvs_close(h);
    }
}

static void slots_save(void)
{
    nvs_handle_t h;
    if (nvs_open("bt", NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_set_blob(h, "slots_bd", s_slots, sizeof(s_slots));
        nvs_commit(h);
        nvs_close(h);
    }
}

/* td_bt_ops_t's addresses are least significant byte first. */
static void flip(uint8_t out[6], const uint8_t in[6])
{
    for (int i = 0; i < 6; i++)
        out[i] = in[5 - i];
}

/* ------------------------------------------------------------ the state */

typedef struct
{
    bool used, ready, kbd, mouse;
    esp_hidh_dev_t *dev;
    const char *state;
    uint8_t addr[6];
    char name[TD_BT_NAME_LEN];
    hid_mouse_layout_t layout;
    uint32_t reports;
    char last[TD_BT_MESSAGE_LEN + 32];
} conn_t;

enum
{
    JOB_OPEN,
    JOB_SCAN
};

typedef struct
{
    int type, slot;
    uint8_t addr[6], addr_type, classic;
    char name[TD_BT_NAME_LEN];
} job_t;

static SemaphoreHandle_t s_lock;
static QueueHandle_t s_jobs;
static conn_t s_conn[TD_BT_SLOTS];
static td_bt_radio_t s_radio = TD_BT_RADIO_OFF;
static int s_enable_error;
static uint32_t s_stack_heap;
static bool s_busy, s_failed, s_scanning, s_classic_scanning, s_le_scanning, s_background;
static int64_t s_scan_end;
static char s_message[TD_BT_MESSAGE_LEN];
static td_bt_device_t s_found[TD_BT_MAX_DEVICES];
static int s_found_n;
static char s_pairings[48];
static int s_pairing_slot = -1;   /* the slot an open in progress is for */

static void lock(void)
{
    xSemaphoreTakeRecursive(s_lock, portMAX_DELAY);
}
static void unlock(void)
{
    xSemaphoreGiveRecursive(s_lock);
}

static void say(bool failed, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void say(bool failed, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s_message, sizeof(s_message), fmt, ap);
    va_end(ap);
    s_failed = failed;
    ESP_LOGI(TAG, "%s", s_message);
}

static int slot_of_addr(const uint8_t addr[6])
{
    for (int i = 0; i < TD_BT_SLOTS; i++)
        if (s_conn[i].used && !memcmp(s_conn[i].addr, addr, 6))
            return i;
    for (int i = 0; i < TD_BT_SLOTS; i++)
        if (s_slots[i].used && !memcmp(s_slots[i].addr, addr, 6))
            return i;
    return -1;
}

static void background(void);

/* ------------------------------------------------------------ scanning */

static td_bt_device_t *found(const uint8_t addr[6])
{
    uint8_t le[6];
    flip(le, addr);
    for (int i = 0; i < s_found_n; i++)
        if (!memcmp(s_found[i].addr, le, 6))
            return &s_found[i];
    if (s_found_n == TD_BT_MAX_DEVICES)
        return NULL;
    td_bt_device_t *d = &s_found[s_found_n++];
    memset(d, 0, sizeof(*d));
    memcpy(d->addr, le, 6);
    d->rssi = -127;
    return d;
}

static void name_into(td_bt_device_t *d, const uint8_t *name, int len)
{
    if (!name || len <= 0 || d->name[0])
        return;
    if (len > TD_BT_NAME_LEN - 1)
        len = TD_BT_NAME_LEN - 1;
    memcpy(d->name, name, (size_t)len);
    d->name[len] = 0;
}

static void scan_finished(void)
{
    if (!s_scanning || s_classic_scanning || s_le_scanning)
        return;
    s_scanning = false;
    s_busy = false;
    int kept = 0;   /* what could be paired: HID devices, and others with a name */
    for (int i = 0; i < s_found_n; i++)
        if (s_found[i].kind != TD_BT_KIND_OTHER || s_found[i].name[0])
            s_found[kept++] = s_found[i];
    s_found_n = kept;
    for (int i = 1; i < s_found_n; i++)   /* strongest first */
        for (int j = i; j > 0 && s_found[j].rssi > s_found[j - 1].rssi; j--)
        {
            td_bt_device_t t = s_found[j];
            s_found[j] = s_found[j - 1];
            s_found[j - 1] = t;
        }
    say(false, "%d device%s found", s_found_n, s_found_n == 1 ? "" : "s");
    background();
}

static void bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    lock();
    switch (event)
    {
    case ESP_BT_GAP_DISC_RES_EVT:
    {
        td_bt_device_t *d = found(param->disc_res.bda);
        if (!d)
            break;
        d->classic = true;
        for (int i = 0; i < param->disc_res.num_prop; i++)
        {
            esp_bt_gap_dev_prop_t *p = &param->disc_res.prop[i];
            if (p->type == ESP_BT_GAP_DEV_PROP_BDNAME)
                name_into(d, p->val, (int)strlen((const char *)p->val));
            else if (p->type == ESP_BT_GAP_DEV_PROP_RSSI)
                d->rssi = *(int8_t *)p->val;
            else if (p->type == ESP_BT_GAP_DEV_PROP_COD)
            {
                uint32_t cod;
                memcpy(&cod, p->val, sizeof(cod));
                /* Major class peripheral: minor bits keyboard 0x40, pointer 0x80. */
                if (((cod >> 8) & 0x1F) == 5)
                    d->kind = (cod & 0xC0) == 0x40   ? TD_BT_KIND_KEYBOARD
                              : (cod & 0xC0) == 0x80 ? TD_BT_KIND_MOUSE
                                                     : TD_BT_KIND_HID;
            }
            else if (p->type == ESP_BT_GAP_DEV_PROP_EIR)
            {
                uint8_t len = 0;
                uint8_t *n = esp_bt_gap_resolve_eir_data(p->val, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &len);
                if (!n)
                    n = esp_bt_gap_resolve_eir_data(p->val, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &len);
                name_into(d, n, len);
            }
        }
        break;
    }
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED && s_classic_scanning)
        {
            s_classic_scanning = false;
            scan_finished();
        }
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:   /* Secure Simple Pairing: the keyboard types it */
        say(false, "On the keyboard, type %06lu and Enter", (unsigned long)param->key_notif.passkey);
        break;
    case ESP_BT_GAP_CFM_REQ_EVT:
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_PIN_REQ_EVT:
    {
        /* An older keyboard: we choose the PIN, it is typed on the keyboard. */
        esp_bt_pin_code_t pin = {0};
        unsigned v = esp_random() % 10000;
        snprintf((char *)pin, sizeof(pin), "%04u", v);
        say(false, "On the keyboard, type %04u and Enter", v);
        esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin);
        break;
    }
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        if (param->auth_cmpl.stat != ESP_BT_STATUS_SUCCESS)
            say(true, "pairing failed (%d): put it in pairing mode and try again", param->auth_cmpl.stat);
        break;
    default:
        break;
    }
    unlock();
}

static bool has_hid_uuid(const uint8_t *adv, uint8_t type)
{
    uint8_t len = 0;
    uint8_t *u = esp_ble_resolve_adv_data((uint8_t *)adv, type, &len);
    for (int i = 0; u && i + 1 < len; i += 2)
        if ((u[i] | u[i + 1] << 8) == UUID_HID)
            return true;
    return false;
}

static void ble_result(const struct ble_scan_result_evt_param *r)
{
    if (s_background)
    {
        /* A paired LE device woke up: connect it (on the worker). */
        for (int i = 0; i < TD_BT_SLOTS; i++)
            if (s_slots[i].used && !s_slots[i].classic && s_slots[i].reconnect && !s_conn[i].used &&
                !memcmp(s_slots[i].addr, r->bda, 6))
            {
                esp_ble_gap_stop_scanning();
                s_background = false;
                job_t j = {.type = JOB_OPEN, .slot = i, .addr_type = r->ble_addr_type};
                memcpy(j.addr, r->bda, 6);
                snprintf(j.name, sizeof(j.name), "%s", s_slots[i].name);
                s_conn[i].used = true;   /* claimed; the worker fills it in */
                s_conn[i].state = "connecting";
                xQueueSend(s_jobs, &j, 0);
                return;
            }
        return;
    }
    td_bt_device_t *d = found(r->bda);
    if (!d)
        return;
    d->addr_type = r->ble_addr_type;
    if (r->rssi > d->rssi)
        d->rssi = (int8_t)r->rssi;
    uint8_t len = 0;
    uint8_t *n = esp_ble_resolve_adv_data((uint8_t *)r->ble_adv, ESP_BLE_AD_TYPE_NAME_CMPL, &len);
    if (!n)
        n = esp_ble_resolve_adv_data((uint8_t *)r->ble_adv, ESP_BLE_AD_TYPE_NAME_SHORT, &len);
    name_into(d, n, len);
    if (has_hid_uuid(r->ble_adv, ESP_BLE_AD_TYPE_16SRV_CMPL) || has_hid_uuid(r->ble_adv, ESP_BLE_AD_TYPE_16SRV_PART))
        d->kind = d->kind == TD_BT_KIND_OTHER ? TD_BT_KIND_HID : d->kind;
    uint8_t *a = esp_ble_resolve_adv_data((uint8_t *)r->ble_adv, ESP_BLE_AD_TYPE_APPEARANCE, &len);
    if (a && len == 2)
    {
        uint16_t app = (uint16_t)(a[0] | a[1] << 8);
        d->kind = app == 0x03C1 ? TD_BT_KIND_KEYBOARD : app == 0x03C2 ? TD_BT_KIND_MOUSE
                                                                      : d->kind;
    }
}

static esp_ble_scan_params_t s_scan_params;

static void ble_gap_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    lock();
    switch (event)
    {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
        esp_ble_gap_start_scanning(s_background ? 0 : SCAN_MS / 1000);
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
        if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_RES_EVT)
            ble_result(&param->scan_rst);
        else if (param->scan_rst.search_evt == ESP_GAP_SEARCH_INQ_CMPL_EVT && s_le_scanning)
        {
            s_le_scanning = false;
            scan_finished();
        }
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT:
        say(false, "On the keyboard, type %06lu and Enter", (unsigned long)param->ble_security.key_notif.passkey);
        break;
    case ESP_GAP_BLE_NC_REQ_EVT:
        esp_ble_confirm_reply(param->ble_security.ble_req.bd_addr, true);
        break;
    default:
        break;
    }
    unlock();
}

/* Look for paired LE devices that are not connected (Classic ones call in). */
static void background(void)
{
    if (s_radio != TD_BT_RADIO_UP || s_scanning || s_background)
        return;
    bool waiting = false;
    for (int i = 0; i < TD_BT_SLOTS; i++)
        waiting |= s_slots[i].used && !s_slots[i].classic && s_slots[i].reconnect && !s_conn[i].used;
    if (!waiting)
        return;
    s_background = true;
    s_scan_params = (esp_ble_scan_params_t){
        .scan_type = BLE_SCAN_TYPE_PASSIVE,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval = 0x0320,   /* 30 ms every 0.5 s */
        .scan_window = 0x0030,
        .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE,
    };
    esp_ble_gap_set_scan_params(&s_scan_params);
}

/* ------------------------------------------------------------ HID host */

static void attach(conn_t *c)
{
    char name[48];
    snprintf(name, sizeof(name), "Bluetooth: %s", c->name[0] ? c->name : "device");
    esp_hid_usage_t usage = esp_hidh_dev_usage_get(c->dev);
    c->kbd = (usage & ESP_HID_USAGE_KEYBOARD) != 0;
    c->mouse = false;
    size_t num = 0;
    esp_hid_raw_report_map_t *maps = NULL;
    if ((usage & ESP_HID_USAGE_MOUSE) && esp_hidh_dev_report_maps_get(c->dev, &num, &maps) == ESP_OK)
        for (size_t i = 0; i < num && !c->mouse; i++)
            c->mouse = hid_mouse_parse(&c->layout, maps[i].data, maps[i].len);
    if ((usage & ESP_HID_USAGE_MOUSE) && !c->mouse)
    {
        memset(&c->layout, 0, sizeof(c->layout));   /* boot reports */
        c->mouse = true;
    }
    if (c->kbd)
        hid_kbd_attach(name);
    if (c->mouse)
        hid_mouse_attach(name);
}

static void hidh_cb(void *args, esp_event_base_t base, int32_t id, void *data)
{
    (void)args;
    (void)base;
    esp_hidh_event_data_t *p = data;
    lock();
    switch ((esp_hidh_event_t)id)
    {
    case ESP_HIDH_OPEN_EVENT:
    {
        if (p->open.status != ESP_OK || !p->open.dev)
            break;
        const uint8_t *bda = esp_hidh_dev_bda_get(p->open.dev);
        int slot = s_pairing_slot >= 0 ? s_pairing_slot : bda ? slot_of_addr(bda)
                                                              : -1;
        if (slot < 0)
        {
            esp_hidh_dev_close(p->open.dev);   /* not one of ours */
            break;
        }
        conn_t *c = &s_conn[slot];
        c->used = true;
        c->dev = p->open.dev;
        if (bda)
            memcpy(c->addr, bda, 6);
        const char *n = esp_hidh_dev_name_get(p->open.dev);
        if (n && n[0])
            snprintf(c->name, sizeof(c->name), "%s", n);
        else if (!c->name[0])
            snprintf(c->name, sizeof(c->name), "%s", s_slots[slot].name);
        attach(c);
        c->ready = true;
        c->state = "ready";
        snprintf(c->last, sizeof(c->last), "connected: %s%s%s", c->kbd ? "keyboard" : "",
                 c->kbd && c->mouse ? " and " : "", c->mouse ? "mouse" : "");
        saved_slot_t *s = &s_slots[slot];
        bool classic = esp_hidh_dev_transport_get(p->open.dev) == ESP_HID_TRANSPORT_BT;
        if (!s->used || memcmp(s->addr, c->addr, 6) || strcmp(s->name, c->name))
        {
            s->used = 1;
            s->reconnect = 1;
            s->classic = classic;
            memcpy(s->addr, c->addr, 6);
            snprintf(s->name, sizeof(s->name), "%s", c->name);
            slots_save();
        }
        s_busy = false;
        say(false, "%s is ready", c->name);
        background();
        break;
    }
    case ESP_HIDH_INPUT_EVENT:
    {
        const uint8_t *bda = esp_hidh_dev_bda_get(p->input.dev);
        int slot = bda ? slot_of_addr(bda) : -1;
        if (slot < 0 || !s_conn[slot].ready)
            break;
        conn_t *c = &s_conn[slot];
        c->reports++;
        if (p->input.usage == ESP_HID_USAGE_KEYBOARD && c->kbd)
        {
            uint8_t k[8] = {0};
            if (p->input.length >= 8)
                memcpy(k, p->input.data, 8);
            else if (p->input.length == 7)
            {
                k[0] = p->input.data[0];
                memcpy(k + 2, p->input.data + 1, 6);
            }
            else
                break;
            hid_kbd_report(k, 8);
        }
        else if (p->input.usage == ESP_HID_USAGE_MOUSE && c->mouse)
        {
            uint8_t buf[66];
            uint16_t n = p->input.length < sizeof(buf) - 1 ? p->input.length : sizeof(buf) - 1;
            if (c->layout.ok && c->layout.ids)
            {
                buf[0] = (uint8_t)p->input.report_id;
                memcpy(buf + 1, p->input.data, n);
                hid_mouse_report(&c->layout, buf, (size_t)n + 1);
            }
            else
                hid_mouse_report(&c->layout, p->input.data, n);
        }
        break;
    }
    case ESP_HIDH_CLOSE_EVENT:
    {
        const uint8_t *bda = esp_hidh_dev_bda_get(p->close.dev);
        int slot = bda ? slot_of_addr(bda) : -1;
        if (slot >= 0 && s_conn[slot].dev == p->close.dev)
        {
            conn_t *c = &s_conn[slot];
            if (c->ready)
            {
                if (c->kbd)
                    hid_kbd_detach();
                if (c->mouse)
                    hid_mouse_detach();
            }
            char last[sizeof(c->last)];
            snprintf(last, sizeof(last), "disconnected");
            memset(c, 0, sizeof(*c));
            memcpy(c->last, last, sizeof(last));
        }
        esp_hidh_dev_free(p->close.dev);
        background();
        break;
    }
    default:
        break;
    }
    unlock();
}

/* The worker: opening a device blocks until it is connected and its HID
 * service read, so it runs here and not on the caller's task. */
static void worker(void *arg)
{
    (void)arg;
    for (;;)
    {
        job_t j;
        if (xQueueReceive(s_jobs, &j, portMAX_DELAY) != pdTRUE)
            continue;
        if (j.type == JOB_SCAN)
        {
            lock();
            s_found_n = 0;
            s_scanning = s_classic_scanning = s_le_scanning = true;
            if (s_background)
            {
                esp_ble_gap_stop_scanning();
                s_background = false;
            }
            s_scan_end = esp_timer_get_time() + SCAN_MS * 1000LL;
            say(false, "listening for devices in pairing mode");
            esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 6, 0);   /* 6 x 1.28 s */
            s_scan_params = (esp_ble_scan_params_t){
                .scan_type = BLE_SCAN_TYPE_ACTIVE,
                .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
                .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
                .scan_interval = 0x50,
                .scan_window = 0x30,
                .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE,
            };
            esp_ble_gap_set_scan_params(&s_scan_params);
            unlock();
            continue;
        }
        lock();
        s_pairing_slot = j.slot;
        conn_t *c = &s_conn[j.slot];
        c->used = true;
        c->state = s_slots[j.slot].used && !memcmp(s_slots[j.slot].addr, j.addr, 6) ? "connecting" : "pairing";
        memcpy(c->addr, j.addr, 6);
        snprintf(c->name, sizeof(c->name), "%s", j.name);
        unlock();
        esp_hidh_dev_t *dev =
            esp_hidh_dev_open(j.addr, j.classic ? ESP_HID_TRANSPORT_BT : ESP_HID_TRANSPORT_BLE, j.addr_type);
        lock();
        s_pairing_slot = -1;
        if (!dev && !c->ready)
        {
            say(true, "%s did not connect: put it in pairing mode (or wake it) and try again",
                j.name[0] ? j.name : "the device");
            snprintf(c->last, sizeof(c->last), "did not connect");
            char last[sizeof(c->last)];
            memcpy(last, c->last, sizeof(last));
            memset(c, 0, sizeof(*c));
            memcpy(c->last, last, sizeof(last));
            s_busy = false;
            background();
        }
        unlock();
    }
}

/* ------------------------------------------------------------ the stack */

/* Starts the stack. Never called with the state lock held: while Bluedroid
 * and the HID host start, their tasks call back into code that takes it,
 * and the start waits for them. Its own lock keeps two starts apart. */
static SemaphoreHandle_t s_start_lock;

static bool radio_start_locked(void);

static bool radio_start(void)
{
    if (s_radio == TD_BT_RADIO_UP)
        return true;
    xSemaphoreTake(s_start_lock, portMAX_DELAY);
    bool ok = radio_start_locked();
    xSemaphoreGive(s_start_lock);
    return ok;
}

static bool radio_start_locked(void)
{
    if (s_radio == TD_BT_RADIO_UP)
        return true;
    if (s_radio == TD_BT_RADIO_FAILED)
    {
        say(true, "the radio failed to start (%d); restart the board", s_enable_error);
        return false;
    }
    size_t before = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    if (before < MIN_HEAP)
    {
        say(true, "starting the radio needs %d KB free; %u KB is", MIN_HEAP / 1024, (unsigned)(before / 1024));
        return false;
    }
    s_radio = TD_BT_RADIO_STARTING;
    esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    esp_err_t err = esp_bt_controller_init(&cfg);
    if (err == ESP_OK)
        err = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (err == ESP_OK)
        err = esp_bluedroid_init();
    if (err == ESP_OK)
        err = esp_bluedroid_enable();
    if (err == ESP_OK)
        err = esp_bt_gap_register_callback(bt_gap_cb);
    if (err == ESP_OK)
        err = esp_ble_gap_register_callback(ble_gap_cb);
    if (err != ESP_OK)
    {
        s_radio = TD_BT_RADIO_FAILED;
        s_enable_error = err;
        say(true, "the radio failed to start (%s)", esp_err_to_name(err));
        return false;
    }
    /* Classic: Secure Simple Pairing, the board shows codes (the keyboard
     * types them); a PIN when an older device asks; reachable for paired
     * devices that call in, not discoverable. */
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_OUT;
    esp_bt_gap_set_security_param(ESP_BT_SP_IOCAP_MODE, &iocap, sizeof(iocap));
    esp_bt_pin_code_t pin = {0};
    esp_bt_gap_set_pin(ESP_BT_PIN_TYPE_VARIABLE, 0, pin);
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
    /* LE: bonding with Secure Connections, the board shows the passkey. */
    esp_ble_auth_req_t auth = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t le_io = ESP_IO_CAP_OUT;
    uint8_t keys = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK, size = 16;
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &le_io, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &keys, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &keys, 1);
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &size, 1);
    const esp_hidh_config_t hc = {.callback = hidh_cb, .event_stack_size = 4096, .callback_arg = NULL};
    /* LE HID goes through the GATT client: its events must reach the HID
     * host (esp_hidh_init waits for the client's registration). */
    err = esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler);
    if (err == ESP_OK)
        err = esp_hidh_init(&hc);
    if (err != ESP_OK)
    {
        s_radio = TD_BT_RADIO_FAILED;
        s_enable_error = err;
        say(true, "the HID host failed to start (%s)", esp_err_to_name(err));
        return false;
    }
    size_t after = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    s_stack_heap = before > after ? (uint32_t)(before - after) : 0;
    s_radio = TD_BT_RADIO_UP;
    ESP_LOGI(TAG, "Bluetooth Classic and LE are up");
    /* Classic devices usually call in; try those that are on already. */
    for (int i = 0; i < TD_BT_SLOTS; i++)
        if (s_slots[i].used && s_slots[i].classic && s_slots[i].reconnect)
        {
            job_t j = {.type = JOB_OPEN, .slot = i, .classic = 1};
            memcpy(j.addr, s_slots[i].addr, 6);
            snprintf(j.name, sizeof(j.name), "%s", s_slots[i].name);
            xQueueSend(s_jobs, &j, 0);
        }
    background();
    return true;
}

/* ------------------------------------------------------- td_bt_ops_t */

static void ops_status(td_bt_status_t *out)
{
    memset(out, 0, sizeof(*out));
    lock();
    out->radio = s_radio;
    out->radio_up = s_radio == TD_BT_RADIO_UP;
    out->enable_error = s_enable_error;
    out->stack_heap = s_stack_heap;
    out->busy = s_busy;
    out->job_failed = s_failed;
    snprintf(out->message, sizeof(out->message), "%s", s_message);
    if (s_scanning)
    {
        int64_t left = (s_scan_end - esp_timer_get_time()) / 1000;
        out->scan_ms_left = left > 0 ? (uint32_t)left : 0;
    }
    for (int i = 0; i < TD_BT_SLOTS; i++)
    {
        td_bt_slot_t *o = &out->slot[i];
        const saved_slot_t *s = &s_slots[i];
        const conn_t *c = &s_conn[i];
        o->paired = s->used;
        o->reconnects = s->used && s->reconnect;
        if (s->used)
        {
            flip(o->paired_addr, s->addr);
            snprintf(o->paired_name, sizeof(o->paired_name), "%s", s->name);
            out->paired++;
        }
        if (c->used)
        {
            o->state = c->state ? c->state : "connecting";
            o->active = true;
            o->ready = o->connected = c->ready;
            flip(o->addr, c->addr);
            snprintf(o->name, sizeof(o->name), "%s", c->name);
            o->reports = c->reports;
            out->connected += c->ready;
        }
        else
            o->state = s->used && s->reconnect && s_radio == TD_BT_RADIO_UP ? "waiting" : "idle";
        snprintf(o->last, sizeof(o->last), "%s", c->last);
    }
    snprintf(s_pairings, sizeof(s_pairings), "%d stored in the board's NVS", out->paired);
    out->pairings = s_pairings;
    unlock();
}

static bool ops_scan(void)
{
    if (!radio_start())
        return false;
    lock();
    bool ok = false;
    if (s_scanning || s_busy)
        say(true, "busy: wait for the scan or connection to finish");
    else
    {
        job_t j = {.type = JOB_SCAN};
        s_busy = true;
        s_failed = false;
        s_scanning = true;   /* scan_results() says -1 from now on */
        s_scan_end = esp_timer_get_time() + SCAN_MS * 1000LL;
        ok = xQueueSend(s_jobs, &j, 0) == pdTRUE;
        if (!ok)
            s_scanning = s_busy = false;
    }
    unlock();
    return ok;
}

static int ops_scan_results(td_bt_device_t *out, int max)
{
    lock();
    int n = s_scanning ? -1 : s_found_n < max ? s_found_n
                                              : max;
    if (out && n > 0)
        memcpy(out, s_found, sizeof(*out) * (size_t)n);
    unlock();
    return n;
}

static void forget_bond(const saved_slot_t *s)
{
    if (s_radio != TD_BT_RADIO_UP)
        return;
    uint8_t a[6];
    memcpy(a, s->addr, 6);
    if (s->classic)
        esp_bt_gap_remove_bond_device(a);
    else
        esp_ble_remove_bond_device(a);
}

static bool ops_pair(int slot, const td_bt_device_t *dev)
{
    if (slot < 0 || slot >= TD_BT_SLOTS || !dev)
        return false;
    if (!radio_start())
        return false;
    lock();
    bool ok = false;
    if (s_scanning || s_busy)
        say(true, "busy: try again in a moment");
    else
    {
        uint8_t addr[6];
        flip(addr, dev->addr);
        conn_t *old = &s_conn[slot];
        if (old->used && old->dev)
            esp_hidh_dev_close(old->dev);
        if (s_slots[slot].used && memcmp(s_slots[slot].addr, addr, 6))
        {
            forget_bond(&s_slots[slot]);
            memset(&s_slots[slot], 0, sizeof(s_slots[slot]));
            slots_save();
        }
        job_t j = {.type = JOB_OPEN, .slot = slot, .addr_type = dev->addr_type, .classic = dev->classic};
        memcpy(j.addr, addr, 6);
        snprintf(j.name, sizeof(j.name), "%s", dev->name);
        s_busy = true;
        s_failed = false;
        say(false, "connecting to %s...", dev->name[0] ? dev->name : "the device");
        ok = xQueueSend(s_jobs, &j, 0) == pdTRUE;
    }
    unlock();
    return ok;
}

static bool ops_disconnect(int slot)
{
    if (slot < 0 || slot >= TD_BT_SLOTS)
        return false;
    lock();
    s_slots[slot].reconnect = 0;
    slots_save();
    if (s_conn[slot].used && s_conn[slot].dev)
        esp_hidh_dev_close(s_conn[slot].dev);
    say(false, "not reconnecting; pairing kept");
    unlock();
    return true;
}

static int ops_paired(td_bt_paired_t *out, int max)
{
    lock();
    int n = 0;
    for (int i = 0; i < TD_BT_SLOTS && n < max; i++)
    {
        if (!s_slots[i].used)
            continue;
        memset(&out[n], 0, sizeof(out[n]));
        out[n].slot = i;
        flip(out[n].addr, s_slots[i].addr);
        snprintf(out[n].name, sizeof(out[n].name), "%s", s_slots[i].name);
        out[n].connected = s_conn[i].ready;
        out[n].reconnects = s_slots[i].reconnect;
        n++;
    }
    unlock();
    return n;
}

static bool ops_forget(int slot)
{
    if (slot < 0 || slot >= TD_BT_SLOTS)
        return false;
    lock();
    if (s_conn[slot].used && s_conn[slot].dev)
        esp_hidh_dev_close(s_conn[slot].dev);
    if (s_slots[slot].used)
        forget_bond(&s_slots[slot]);
    memset(&s_slots[slot], 0, sizeof(s_slots[slot]));
    slots_save();
    say(false, "pairing deleted");
    unlock();
    return true;
}

static bool ops_set_reconnect(int slot, bool on)
{
    if (slot < 0 || slot >= TD_BT_SLOTS || !s_slots[slot].used)
        return false;
    if (!on)
        return ops_disconnect(slot);
    bool ok = radio_start();
    lock();
    if (ok)
    {
        s_slots[slot].reconnect = 1;
        slots_save();
        say(false, "reconnects by itself: wake the device up");
        if (s_slots[slot].classic && !s_conn[slot].used)
        {
            job_t j = {.type = JOB_OPEN, .slot = slot, .classic = 1};
            memcpy(j.addr, s_slots[slot].addr, 6);
            snprintf(j.name, sizeof(j.name), "%s", s_slots[slot].name);
            xQueueSend(s_jobs, &j, 0);
        }
        background();
    }
    unlock();
    return ok;
}

static const td_bt_ops_t s_ops = {
    .status = ops_status,
    .scan = ops_scan,
    .scan_results = ops_scan_results,
    .pair = ops_pair,
    .disconnect = ops_disconnect,
    .paired = ops_paired,
    .forget = ops_forget,
    .set_reconnect = ops_set_reconnect,
};

const td_bt_ops_t *bt_hid_start(void)
{
    if (tdsh_board_int("bt.enable", 0) != 1)
    {
        esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);   /* off: its memory goes back to the heap */
        return NULL;
    }
    s_lock = xSemaphoreCreateRecursiveMutex();
    s_start_lock = xSemaphoreCreateMutex();
    s_jobs = xQueueCreate(4, sizeof(job_t));
    if (!s_lock || !s_start_lock || !s_jobs || xTaskCreate(worker, "bt_hid", 4096, NULL, 4, NULL) != pdPASS)
        return NULL;
    hid_input_init();
    slots_load();
    bool any = false;
    for (int i = 0; i < TD_BT_SLOTS; i++)
        any |= s_slots[i].used && s_slots[i].reconnect;
    if (any)
        radio_start();
    ESP_LOGI(TAG, "Bluetooth Classic and LE keyboards and mice: %s",
             any ? "radio on for paired devices" : "radio off until a scan");
    return &s_ops;
}

#endif /* CONFIG_BT_BLUEDROID_ENABLED && CONFIG_BT_CLASSIC_ENABLED */
