/*
 * bt_hid.c - Bluetooth LE keyboards and mice on the ESP32 boards (not the 4 MB ESP32), behind
 * TinyDesk's td_bt_ops_t (td_sysinfo.h; the Bluetooth app and the bt
 * command).
 *
 * NimBLE as a central only (it connects to devices; it never advertises),
 * with a small HID-over-GATT client of its own: on a connection it encrypts
 * the link (the stored keys of a paired device, or pairs a new one; a
 * keyboard that asks for a passkey gets one to type, shown in the app's
 * message), finds the HID service, reads the report map, reads which report
 * each Report characteristic is and subscribes to the input reports. Their
 * data goes to hid_input.c, the same decoder as USB devices: keyboards in
 * boot format (modifiers, reserved, six keys), mice in their own format from
 * the report map, so the wheel works.
 *
 * Pairings live in three slots (keyboard, mouse, combo): the slot list in
 * NVS ("bt"/"slots"), the keys in NimBLE's own store (NVS). The radio starts
 * on the first scan, or at start when a slot has a device that reconnects;
 * then a light background scan (30 ms every 0.5 s) waits for those devices
 * to wake up and connects them.
 *
 * Board configuration key (the Board configuration page of the docs):
 *   bt.enable  1: Bluetooth keyboards and mice (default 0: off, and on the
 *              classic ESP32 the controller's memory goes back to the heap)
 */
#include "bt_hid.h"

#include "sdkconfig.h"

#if !CONFIG_BT_NIMBLE_ENABLED
/* A board built without Bluetooth (the 4 MB ESP32: the classic ESP32's
 * controller reserves about 64 KB of internal RAM at link time). */
const td_bt_ops_t *bt_hid_start(void)
{
    return NULL;
}
#else

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_bt.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "hid_input.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "nvs.h"
#include "store/config/ble_store_config.h"

void ble_store_config_init(void);   /* NimBLE has no header for it */
#include "tdsh_board.h"

static const char *TAG = "bt_hid";

#define SCAN_MS     8000
#define CONNECT_MS  10000
#define MIN_HEAP    48000   /* internal heap a start needs, with some to spare */
#define MAX_CHRS    20
#define MAX_REPORTS 8
#define MAP_MAX     600

#define UUID_HID      0x1812
#define UUID_MAP      0x2A4B
#define UUID_REPORT   0x2A4D
#define UUID_PROTOCOL 0x2A4E
#define UUID_REF      0x2908
#define UUID_CCCD     0x2902

/* ------------------------------------------------------------ the slots */

typedef struct
{
    uint8_t used, reconnect, addr_type, pad;
    uint8_t addr[6];
    char name[TD_BT_NAME_LEN];
} saved_slot_t;

static saved_slot_t s_slots[TD_BT_SLOTS];

static void slots_load(void)
{
    nvs_handle_t h;
    size_t n = sizeof(s_slots);
    if (nvs_open("bt", NVS_READONLY, &h) == ESP_OK)
    {
        if (nvs_get_blob(h, "slots", s_slots, &n) != ESP_OK || n != sizeof(s_slots))
            memset(s_slots, 0, sizeof(s_slots));
        nvs_close(h);
    }
}

static void slots_save(void)
{
    nvs_handle_t h;
    if (nvs_open("bt", NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_set_blob(h, "slots", s_slots, sizeof(s_slots));
        nvs_commit(h);
        nvs_close(h);
    }
}

/* ------------------------------------------------------------ the state */

typedef struct
{
    uint16_t val, ref, cccd;
    uint8_t id, type;   /* report ID; 1 input, 2 output, 3 feature */
} report_t;

typedef struct
{
    bool used, ready, encrypted, kbd, mouse;
    int slot;
    uint16_t handle;
    ble_addr_t addr;
    char name[TD_BT_NAME_LEN];
    const char *state;
    uint16_t svc_start, svc_end;
    struct
    {
        uint16_t def, val, uuid;
        uint8_t props;
    } chr[MAX_CHRS];
    int nchr;
    report_t rep[MAX_REPORTS];
    int nrep, step;
    uint16_t map_handle, proto_handle, map_len;
    uint8_t map[MAP_MAX];
    uint8_t kbd_id;
    bool kbd_ids;
    hid_mouse_layout_t layout;
    uint32_t reports;
    char last[TD_BT_MESSAGE_LEN + 32];
} conn_t;

static SemaphoreHandle_t s_lock;
static conn_t *s_conn;   /* TD_BT_SLOTS of them, allocated when the radio starts */
static td_bt_radio_t s_radio = TD_BT_RADIO_OFF;
static int s_enable_error;
static uint32_t s_stack_heap;
static uint8_t s_own_addr_type;
static bool s_busy, s_failed, s_scanning, s_scan_wanted;
static int64_t s_scan_end;
static char s_message[TD_BT_MESSAGE_LEN];
static td_bt_device_t s_found[TD_BT_MAX_DEVICES];
static int s_found_n;
static char s_pairings[48];

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

static conn_t *conn_by_handle(uint16_t handle)
{
    for (int i = 0; s_conn && i < TD_BT_SLOTS; i++)
        if (s_conn[i].used && s_conn[i].handle == handle)
            return &s_conn[i];
    return NULL;
}

static conn_t *conn_by_slot(int slot)
{
    return s_conn && s_conn[slot].used ? &s_conn[slot] : NULL;
}

static void kick(void);
static int gap_event(struct ble_gap_event *event, void *arg);

/* ------------------------------------------------------- the report map */

/* The report ID of the keyboard's input report (a Keyboard application
 * collection), if the map has one. */
static bool keyboard_in_map(conn_t *c)
{
    uint32_t page = 0, usage = 0;
    uint8_t id = 0;
    int depth = 0, kbd_depth = -1;
    bool ids = false, found = false;
    const uint8_t *d = c->map;
    for (size_t i = 0; i < c->map_len;)
    {
        uint8_t b = d[i++];
        if (b == 0xFE)
        {
            if (i >= c->map_len)
                break;
            i += 2u + d[i];
            continue;
        }
        int n = (b & 3) == 3 ? 4 : (b & 3);
        if (i + (size_t)n > c->map_len)
            break;
        uint32_t v = 0;
        for (int k = 0; k < n; k++)
            v |= (uint32_t)d[i + k] << (8 * k);
        i += (size_t)n;
        int type = (b >> 2) & 3, tag = b >> 4;
        if (type == 1 && tag == 0)
            page = v;
        else if (type == 1 && tag == 8)
        {
            id = (uint8_t)v;
            ids = true;
        }
        else if (type == 2 && tag == 0)
            usage = n == 4 ? v : (page << 16 | v);
        else if (type == 0 && tag == 10)   /* Collection */
        {
            if (v == 1 && usage == 0x10006 && kbd_depth < 0)
                kbd_depth = depth;
            depth++;
        }
        else if (type == 0 && tag == 12)   /* End Collection */
        {
            depth--;
            if (depth == kbd_depth)
                kbd_depth = -1;
        }
        else if (type == 0 && tag == 8 && kbd_depth >= 0 && !found)   /* Input */
        {
            c->kbd_id = id;
            found = true;
        }
        if (type == 0)
            usage = 0;
    }
    c->kbd_ids = ids;
    return found;
}

/* --------------------------------------------- the HID-over-GATT client */

static void fail(conn_t *c, const char *what, int rc)
{
    say(true, "%s: %s (%d)", c->name[0] ? c->name : "the device", what, rc);
    snprintf(c->last, sizeof(c->last), "%s (%d)", what, rc);
    ble_gap_terminate(c->handle, BLE_ERR_REM_USER_CONN_TERM);
}

static void step(conn_t *c);

static int on_cccd(uint16_t handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
    (void)attr;
    (void)arg;
    lock();
    conn_t *c = conn_by_handle(handle);
    if (c)
    {
        if (error->status && error->status != BLE_HS_EDONE)
            ESP_LOGW(TAG, "%s: subscribing to report %d failed (%d)", c->name, c->rep[c->step].id, error->status);
        c->step++;
        step(c);
    }
    unlock();
    return 0;
}

static int on_ref(uint16_t handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
    (void)arg;
    lock();
    conn_t *c = conn_by_handle(handle);
    if (c && error->status == 0 && attr && OS_MBUF_PKTLEN(attr->om) >= 2)
    {
        uint8_t v[2];
        ble_hs_mbuf_to_flat(attr->om, v, sizeof(v), NULL);
        c->rep[c->step].id = v[0];
        c->rep[c->step].type = v[1];
    }
    if (c)
    {
        c->step++;
        step(c);
    }
    unlock();
    return 0;
}

static int on_map(uint16_t handle, const struct ble_gatt_error *error, struct ble_gatt_attr *attr, void *arg)
{
    (void)arg;
    lock();
    conn_t *c = conn_by_handle(handle);
    if (c && error->status == 0 && attr)
    {
        uint16_t n = OS_MBUF_PKTLEN(attr->om);
        if (attr->offset + n <= MAP_MAX)
        {
            ble_hs_mbuf_to_flat(attr->om, c->map + attr->offset, n, NULL);
            if (attr->offset + n > c->map_len)
                c->map_len = (uint16_t)(attr->offset + n);
        }
    }
    else if (c && error->status == BLE_HS_EDONE)
    {
        c->step = 0;
        c->state = "discovering";
        step(c);   /* on to the report references */
    }
    else if (c)
    {
        fail(c, "reading the report map failed", error->status);
    }
    unlock();
    return 0;
}

static int on_dsc(uint16_t handle, const struct ble_gatt_error *error, uint16_t chr_val, const struct ble_gatt_dsc *dsc,
                  void *arg)
{
    (void)chr_val;
    (void)arg;
    lock();
    conn_t *c = conn_by_handle(handle);
    if (c && error->status == 0 && dsc && dsc->uuid.u.type == BLE_UUID_TYPE_16)
    {
        uint16_t u = ble_uuid_u16(&dsc->uuid.u);
        if (u == UUID_REF)
            c->rep[c->step].ref = dsc->handle;
        else if (u == UUID_CCCD)
            c->rep[c->step].cccd = dsc->handle;
    }
    else if (c && (error->status == BLE_HS_EDONE || error->status))
    {
        c->step++;
        step(c);
    }
    unlock();
    return 0;
}

static int on_chr(uint16_t handle, const struct ble_gatt_error *error, const struct ble_gatt_chr *chr, void *arg)
{
    (void)arg;
    lock();
    conn_t *c = conn_by_handle(handle);
    if (c && error->status == 0 && chr && c->nchr < MAX_CHRS)
    {
        c->chr[c->nchr].def = chr->def_handle;
        c->chr[c->nchr].val = chr->val_handle;
        c->chr[c->nchr].props = chr->properties;
        c->chr[c->nchr].uuid = chr->uuid.u.type == BLE_UUID_TYPE_16 ? ble_uuid_u16(&chr->uuid.u) : 0;
        c->nchr++;
    }
    else if (c && error->status == BLE_HS_EDONE)
    {
        for (int i = 0; i < c->nchr; i++)
        {
            if (c->chr[i].uuid == UUID_MAP)
                c->map_handle = c->chr[i].val;
            else if (c->chr[i].uuid == UUID_PROTOCOL)
                c->proto_handle = c->chr[i].val;
            else if (c->chr[i].uuid == UUID_REPORT && (c->chr[i].props & BLE_GATT_CHR_PROP_NOTIFY) &&
                     c->nrep < MAX_REPORTS)
                c->rep[c->nrep++] = (report_t){.val = c->chr[i].val, .type = 1};
        }
        if (!c->map_handle || !c->nrep)
            fail(c, "it has no HID reports", 0);
        else
        {
            c->step = 0;
            step(c);   /* on to the descriptors */
        }
    }
    else if (c)
    {
        fail(c, "finding its characteristics failed", error->status);
    }
    unlock();
    return 0;
}

static int on_svc(uint16_t handle, const struct ble_gatt_error *error, const struct ble_gatt_svc *svc, void *arg)
{
    (void)arg;
    lock();
    conn_t *c = conn_by_handle(handle);
    if (c && error->status == 0 && svc)
    {
        c->svc_start = svc->start_handle;
        c->svc_end = svc->end_handle;
    }
    else if (c && error->status == BLE_HS_EDONE)
    {
        if (!c->svc_start)
            fail(c, "it is not a keyboard or mouse (no HID service)", 0);
        else
        {
            int rc = ble_gattc_disc_all_chrs(c->handle, c->svc_start, c->svc_end, on_chr, NULL);
            if (rc)
                fail(c, "finding its characteristics failed", rc);
        }
    }
    else if (c)
    {
        fail(c, "finding its HID service failed", error->status);
    }
    unlock();
    return 0;
}

/* The end of a characteristic's descriptors: before the next one. */
static uint16_t chr_end(const conn_t *c, uint16_t val)
{
    uint16_t end = c->svc_end;
    for (int i = 0; i < c->nchr; i++)
        if (c->chr[i].def > val && c->chr[i].def - 1 < end)
            end = (uint16_t)(c->chr[i].def - 1);
    return end;
}

static void ready(conn_t *c);

/* The client's steps after the characteristics, one at a time: each
 * report's descriptors, the report map, each report's reference, then
 * subscribing to the input reports. c->step counts within a phase;
 * c->state says which phase. */
static void step(conn_t *c)
{
    int rc = 0;
    if (!strcmp(c->state, "connecting") || !strcmp(c->state, "pairing"))
        c->state = "reading";   /* descriptors */
    if (!strcmp(c->state, "reading"))
    {
        if (c->step < c->nrep)
        {
            uint16_t end = chr_end(c, c->rep[c->step].val);
            if (end <= c->rep[c->step].val)
            {
                c->step++;
                step(c);
                return;
            }
            rc = ble_gattc_disc_all_dscs(c->handle, c->rep[c->step].val, end, on_dsc, NULL);
            if (rc)
                fail(c, "finding its report descriptors failed", rc);
            return;
        }
        c->state = "mapping";
        c->map_len = 0;
        rc = ble_gattc_read_long(c->handle, c->map_handle, 0, on_map, NULL);
        if (rc)
            fail(c, "reading the report map failed", rc);
        return;
    }
    if (!strcmp(c->state, "discovering"))   /* references */
    {
        while (c->step < c->nrep && !c->rep[c->step].ref)
            c->step++;
        if (c->step < c->nrep)
        {
            rc = ble_gattc_read(c->handle, c->rep[c->step].ref, on_ref, NULL);
            if (rc)
                fail(c, "reading a report reference failed", rc);
            return;
        }
        c->state = "subscribing";
        c->step = 0;
    }
    if (!strcmp(c->state, "subscribing"))
    {
        while (c->step < c->nrep && (c->rep[c->step].type != 1 || !c->rep[c->step].cccd))
            c->step++;
        if (c->step < c->nrep)
        {
            static const uint8_t on[2] = {1, 0};
            rc = ble_gattc_write_flat(c->handle, c->rep[c->step].cccd, on, sizeof(on), on_cccd, NULL);
            if (rc)
                fail(c, "subscribing to its reports failed", rc);
            return;
        }
        ready(c);
    }
}

static void ready(conn_t *c)
{
    if (c->proto_handle)
    {
        static const uint8_t report_mode = 1;
        ble_gattc_write_no_rsp_flat(c->handle, c->proto_handle, &report_mode, 1);
    }
    c->kbd = keyboard_in_map(c);
    c->mouse = hid_mouse_parse(&c->layout, c->map, c->map_len);
    if (!c->kbd && !c->mouse)
    {
        fail(c, "its reports are not a keyboard's or a mouse's", 0);
        return;
    }
    char name[48];
    snprintf(name, sizeof(name), "Bluetooth: %s", c->name[0] ? c->name : "device");
    if (c->kbd)
        hid_kbd_attach(name);
    if (c->mouse)
        hid_mouse_attach(name);
    c->ready = true;
    c->state = "ready";
    s_busy = false;
    snprintf(c->last, sizeof(c->last), "connected: %s%s%s", c->kbd ? "keyboard" : "", c->kbd && c->mouse ? " and " : "",
             c->mouse ? (c->layout.wheel.size ? "mouse with its wheel" : "mouse") : "");
    say(false, "%s is ready", c->name[0] ? c->name : "the device");
    saved_slot_t *s = &s_slots[c->slot];
    if (!s->used || memcmp(s->addr, c->addr.val, 6) || strcmp(s->name, c->name))
    {
        s->used = 1;
        s->reconnect = 1;
        s->addr_type = c->addr.type;
        memcpy(s->addr, c->addr.val, 6);
        snprintf(s->name, sizeof(s->name), "%s", c->name);
        slots_save();
    }
    kick();
}

static void input(conn_t *c, uint16_t attr, struct os_mbuf *om)
{
    const report_t *r = NULL;
    for (int i = 0; i < c->nrep; i++)
        if (c->rep[i].val == attr)
            r = &c->rep[i];
    if (!r || !c->ready)
        return;
    uint8_t buf[66];
    uint16_t n = OS_MBUF_PKTLEN(om);
    if (n > sizeof(buf) - 1)
        n = sizeof(buf) - 1;
    ble_hs_mbuf_to_flat(om, buf + 1, n, NULL);
    c->reports++;
    if (c->kbd && (!c->kbd_ids || r->id == c->kbd_id))
    {
        uint8_t k[8] = {0};
        if (n >= 8)
            memcpy(k, buf + 1, 8);
        else if (n == 7)   /* no reserved byte */
        {
            k[0] = buf[1];
            memcpy(k + 2, buf + 2, 6);
        }
        else
            return;
        hid_kbd_report(k, 8);
        return;
    }
    if (c->mouse)
    {
        if (c->layout.ids)
        {
            buf[0] = r->id;
            hid_mouse_report(&c->layout, buf, (size_t)n + 1);
        }
        else
            hid_mouse_report(&c->layout, buf + 1, n);
    }
}

/* ------------------------------------------------------------ GAP events */

static void dropped(conn_t *c, int reason)
{
    if (c->ready)
    {
        if (c->kbd)
            hid_kbd_detach();
        if (c->mouse)
            hid_mouse_detach();
        snprintf(c->last, sizeof(c->last), "disconnected (%d)", reason);
    }
    int slot = c->slot;
    char last[sizeof(c->last)];
    memcpy(last, c->last, sizeof(last));
    memset(c, 0, sizeof(*c));
    c->slot = slot;
    memcpy(c->last, last, sizeof(last));
    s_busy = false;
}

static void add_found(const struct ble_gap_disc_desc *d)
{
    struct ble_hs_adv_fields f;
    if (ble_hs_adv_parse_fields(&f, d->data, d->length_data) != 0)
        return;
    td_bt_kind_t kind = TD_BT_KIND_OTHER;
    for (int i = 0; i < f.num_uuids16; i++)
        if (ble_uuid_u16(&f.uuids16[i].u) == UUID_HID)
            kind = TD_BT_KIND_HID;
    if (f.appearance_is_present)
    {
        if (f.appearance == 0x03C1)
            kind = TD_BT_KIND_KEYBOARD;
        else if (f.appearance == 0x03C2)
            kind = TD_BT_KIND_MOUSE;
        else if ((f.appearance & 0xFFC0) == 0x03C0 && kind == TD_BT_KIND_OTHER)
            kind = TD_BT_KIND_HID;
    }
    int i = 0;
    while (i < s_found_n && memcmp(s_found[i].addr, d->addr.val, 6))
        i++;
    if (i == s_found_n)
    {
        if (s_found_n == TD_BT_MAX_DEVICES)
            return;
        memset(&s_found[i], 0, sizeof(s_found[i]));
        memcpy(s_found[i].addr, d->addr.val, 6);
        s_found[i].addr_type = d->addr.type;
        s_found[i].rssi = d->rssi;
        s_found_n++;
    }
    td_bt_device_t *e = &s_found[i];
    if (d->rssi > e->rssi)
        e->rssi = d->rssi;
    if (kind != TD_BT_KIND_OTHER)
        e->kind = kind;
    if (f.name && f.name_len && !e->name[0])
    {
        int n = f.name_len < TD_BT_NAME_LEN - 1 ? f.name_len : TD_BT_NAME_LEN - 1;
        memcpy(e->name, f.name, (size_t)n);
        e->name[n] = 0;
    }
}

static void connect_to(int slot, const ble_addr_t *addr, const char *name)
{
    if (ble_gap_disc_active())
        ble_gap_disc_cancel();
    conn_t *c = &s_conn[slot];
    memset(c, 0, sizeof(*c));
    c->slot = slot;
    c->used = true;
    c->addr = *addr;
    c->state = "connecting";
    c->handle = BLE_HS_CONN_HANDLE_NONE;
    snprintf(c->name, sizeof(c->name), "%s", name ? name : "");
    s_busy = true;
    int rc = ble_gap_connect(s_own_addr_type, addr, CONNECT_MS, NULL, gap_event, (void *)(intptr_t)slot);
    if (rc)
    {
        say(true, "connecting to %s failed (%d)", c->name[0] ? c->name : "the device", rc);
        dropped(c, rc);
    }
}

static int gap_event(struct ble_gap_event *event, void *arg)
{
    lock();
    switch (event->type)
    {
    case BLE_GAP_EVENT_DISC:
    {
        const struct ble_gap_disc_desc *d = &event->disc;
        if (s_scanning)
            add_found(d);
        else
            for (int i = 0; i < TD_BT_SLOTS; i++)   /* a paired device woke up */
                if (s_slots[i].used && s_slots[i].reconnect && !s_conn[i].used &&
                    !memcmp(s_slots[i].addr, d->addr.val, 6) && !ble_gap_conn_active())
                {
                    ble_addr_t a = d->addr;
                    connect_to(i, &a, s_slots[i].name);
                    break;
                }
        break;
    }
    case BLE_GAP_EVENT_DISC_COMPLETE:
        if (s_scanning)
        {
            s_scanning = false;
            s_busy = false;
            /* Only what could be paired: HID devices, and others with a name
             * (some keyboards do not advertise their kind). */
            int kept = 0;
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
        }
        kick();
        break;
    case BLE_GAP_EVENT_CONNECT:
    {
        int slot = (int)(intptr_t)arg;
        conn_t *c = &s_conn[slot];
        if (event->connect.status != 0)
        {
            say(true, "%s did not answer (%d)", c->name[0] ? c->name : "the device", event->connect.status);
            snprintf(c->last, sizeof(c->last), "did not answer");
            dropped(c, event->connect.status);
            kick();
            break;
        }
        c->handle = event->connect.conn_handle;
        c->state = s_slots[slot].used && !memcmp(s_slots[slot].addr, c->addr.val, 6) ? "connecting" : "pairing";
        ble_gattc_exchange_mtu(c->handle, NULL, NULL);
        int rc = ble_gap_security_initiate(c->handle);   /* stored keys, or pairing */
        if (rc && rc != BLE_HS_EALREADY)
            fail(c, "starting encryption failed", rc);
        break;
    }
    case BLE_GAP_EVENT_ENC_CHANGE:
    {
        conn_t *c = conn_by_handle(event->enc_change.conn_handle);
        if (!c)
            break;
        if (event->enc_change.status != 0)
        {
            /* The device forgot us (paired with something else, reset):
             * the stored keys are useless; pair it again. */
            struct ble_gap_conn_desc desc;
            if (ble_gap_conn_find(c->handle, &desc) == 0)
                ble_store_util_delete_peer(&desc.peer_id_addr);
            fail(c, "encryption failed; put it in pairing mode and pair it again", event->enc_change.status);
            break;
        }
        c->encrypted = true;
        int rc = ble_gattc_disc_svc_by_uuid(c->handle, BLE_UUID16_DECLARE(UUID_HID), on_svc, NULL);
        if (rc)
            fail(c, "finding its HID service failed", rc);
        break;
    }
    case BLE_GAP_EVENT_PASSKEY_ACTION:
    {
        conn_t *c = conn_by_handle(event->passkey.conn_handle);
        struct ble_sm_io io = {.action = event->passkey.params.action};
        if (io.action == BLE_SM_IOACT_DISP)
        {
            io.passkey = esp_random() % 1000000;
            say(false, "On the keyboard, type %06lu and Enter", (unsigned long)io.passkey);
            if (c)
                snprintf(c->last, sizeof(c->last), "passkey %06lu", (unsigned long)io.passkey);
        }
        else if (io.action == BLE_SM_IOACT_NUMCMP)
            io.numcmp_accept = 1;
        ble_sm_inject_io(event->passkey.conn_handle, &io);
        break;
    }
    case BLE_GAP_EVENT_REPEAT_PAIRING:
    {
        /* Pairing again with a device we know: forget the old keys. */
        struct ble_gap_conn_desc desc;
        if (ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc) == 0)
            ble_store_util_delete_peer(&desc.peer_id_addr);
        unlock();
        return BLE_GAP_REPEAT_PAIRING_RETRY;
    }
    case BLE_GAP_EVENT_NOTIFY_RX:
    {
        conn_t *c = conn_by_handle(event->notify_rx.conn_handle);
        if (c)
            input(c, event->notify_rx.attr_handle, event->notify_rx.om);
        break;
    }
    case BLE_GAP_EVENT_DISCONNECT:
    {
        conn_t *c = conn_by_handle(event->disconnect.conn.conn_handle);
        if (c)
        {
            if (!c->ready && !s_failed)
                say(true, "%s went away while connecting (%d)", c->name[0] ? c->name : "the device",
                    event->disconnect.reason);
            dropped(c, event->disconnect.reason);
        }
        kick();
        break;
    }
    default:
        break;
    }
    unlock();
    return 0;
}

/* Keep looking for paired devices that reconnect and are not connected:
 * a light passive scan, unless a pairing scan or a connection attempt runs. */
static void kick(void)
{
    if (s_radio != TD_BT_RADIO_UP || s_scanning || ble_gap_conn_active())
        return;
    if (s_scan_wanted)
    {
        s_scan_wanted = false;
        if (ble_gap_disc_active())
            ble_gap_disc_cancel();
        s_found_n = 0;
        struct ble_gap_disc_params p = {.filter_duplicates = 1, .passive = 0};
        int rc = ble_gap_disc(s_own_addr_type, SCAN_MS, &p, gap_event, NULL);
        if (rc)
        {
            s_busy = false;
            say(true, "the scan did not start (%d)", rc);
        }
        else
        {
            s_scanning = true;
            s_scan_end = esp_timer_get_time() + SCAN_MS * 1000LL;
            say(false, "listening for devices in pairing mode");
        }
        return;
    }
    bool waiting = false;
    for (int i = 0; i < TD_BT_SLOTS; i++)
        waiting |= s_slots[i].used && s_slots[i].reconnect && !s_conn[i].used;
    if (waiting && !ble_gap_disc_active())
    {
        struct ble_gap_disc_params p = {.itvl = 0x0320, .window = 0x0030, .passive = 1};
        ble_gap_disc(s_own_addr_type, BLE_HS_FOREVER, &p, gap_event, NULL);
    }
    else if (!waiting && ble_gap_disc_active())
        ble_gap_disc_cancel();
}

/* ------------------------------------------------------------ the stack */

static void on_sync(void)
{
    lock();
    ble_hs_util_ensure_addr(0);
    ble_hs_id_infer_auto(0, &s_own_addr_type);
    s_radio = TD_BT_RADIO_UP;
    unlock();
    ESP_LOGI(TAG, "Bluetooth LE is up");
    lock();
    kick();
    unlock();
}

static void on_reset(int reason)
{
    ESP_LOGW(TAG, "NimBLE reset (%d)", reason);
}

static void host_task(void *arg)
{
    (void)arg;
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* Start NimBLE (once); false with a message when it cannot. */
static bool radio_start(void)
{
    if (s_radio == TD_BT_RADIO_UP || s_radio == TD_BT_RADIO_STARTING)
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
    s_conn = calloc(TD_BT_SLOTS, sizeof(conn_t));
    if (!s_conn)
    {
        say(true, "out of memory");
        return false;
    }
    for (int i = 0; i < TD_BT_SLOTS; i++)
        s_conn[i].slot = i;
    s_radio = TD_BT_RADIO_STARTING;
    esp_err_t err = nimble_port_init();
    if (err != ESP_OK)
    {
        s_radio = TD_BT_RADIO_FAILED;
        s_enable_error = err;
        say(true, "the radio failed to start (%d)", err);
        return false;
    }
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    ble_hs_cfg.sm_io_cap = BLE_SM_IO_CAP_DISP_ONLY;   /* keyboards: we show the passkey */
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_mitm = 1;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_store_config_init();
    nimble_port_freertos_init(host_task);
    size_t after = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    s_stack_heap = before > after ? (uint32_t)(before - after) : 0;
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
        const conn_t *c = conn_by_slot(i);
        o->paired = s->used;
        o->reconnects = s->used && s->reconnect;
        if (s->used)
        {
            memcpy(o->paired_addr, s->addr, 6);
            snprintf(o->paired_name, sizeof(o->paired_name), "%s", s->name);
            out->paired++;
        }
        if (c)
        {
            o->state = c->state;
            o->active = true;
            o->ready = o->connected = c->ready;
            memcpy(o->addr, c->addr.val, 6);
            snprintf(o->name, sizeof(o->name), "%s", c->name);
            o->reports = c->reports;
            out->connected += c->ready;
        }
        else
            o->state = s->used && s->reconnect && s_radio == TD_BT_RADIO_UP ? "waiting" : "idle";
        if (s_conn)
            snprintf(o->last, sizeof(o->last), "%s", s_conn[i].last);
    }
    int n = out->paired;
    snprintf(s_pairings, sizeof(s_pairings), "%d stored in the board's NVS", n);
    out->pairings = s_pairings;
    unlock();
}

static bool ops_scan(void)
{
    lock();
    bool ok = false;
    if (s_scanning || ble_gap_conn_active())
        say(true, "busy: wait for the scan or connection to finish");
    else if (radio_start())
    {
        s_scan_wanted = true;
        s_busy = true;
        s_failed = false;
        s_found_n = 0;
        s_scan_end = esp_timer_get_time() + SCAN_MS * 1000LL;
        say(false, "starting the radio...");
        kick();
        ok = true;
    }
    unlock();
    return ok;
}

static int ops_scan_results(td_bt_device_t *out, int max)
{
    lock();
    int n = s_scanning || s_scan_wanted ? -1 : s_found_n < max ? s_found_n
                                                               : max;
    if (out && n > 0)
        memcpy(out, s_found, sizeof(*out) * (size_t)n);
    unlock();
    return n;
}

static bool ops_pair(int slot, const td_bt_device_t *dev)
{
    if (slot < 0 || slot >= TD_BT_SLOTS || !dev)
        return false;
    lock();
    bool ok = false;
    if (!radio_start())
        ;
    else if (s_radio != TD_BT_RADIO_UP || ble_gap_conn_active() || s_scanning)
        say(true, "busy: try again in a moment");
    else
    {
        conn_t *old = conn_by_slot(slot);
        if (old && old->handle != BLE_HS_CONN_HANDLE_NONE)
            ble_gap_terminate(old->handle, BLE_ERR_REM_USER_CONN_TERM);
        if (s_slots[slot].used && memcmp(s_slots[slot].addr, dev->addr, 6))
        {
            ble_addr_t a = {.type = s_slots[slot].addr_type};
            memcpy(a.val, s_slots[slot].addr, 6);
            ble_store_util_delete_peer(&a);
            memset(&s_slots[slot], 0, sizeof(s_slots[slot]));
            slots_save();
        }
        ble_addr_t a = {.type = dev->addr_type};
        memcpy(a.val, dev->addr, 6);
        s_failed = false;
        say(false, "connecting to %s...", dev->name[0] ? dev->name : "the device");
        connect_to(slot, &a, dev->name);
        ok = !s_failed;
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
    conn_t *c = conn_by_slot(slot);
    if (c && c->handle != BLE_HS_CONN_HANDLE_NONE)
        ble_gap_terminate(c->handle, BLE_ERR_REM_USER_CONN_TERM);
    say(false, "not reconnecting; pairing kept");
    kick();
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
        memcpy(out[n].addr, s_slots[i].addr, 6);
        snprintf(out[n].name, sizeof(out[n].name), "%s", s_slots[i].name);
        out[n].connected = s_conn && s_conn[i].used && s_conn[i].ready;
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
    conn_t *c = conn_by_slot(slot);
    if (c && c->handle != BLE_HS_CONN_HANDLE_NONE)
        ble_gap_terminate(c->handle, BLE_ERR_REM_USER_CONN_TERM);
    if (s_slots[slot].used && s_radio == TD_BT_RADIO_UP)
    {
        ble_addr_t a = {.type = s_slots[slot].addr_type};
        memcpy(a.val, s_slots[slot].addr, 6);
        ble_store_util_delete_peer(&a);
    }
    memset(&s_slots[slot], 0, sizeof(s_slots[slot]));
    slots_save();
    say(false, "pairing deleted");
    kick();
    unlock();
    return true;
}

static bool ops_set_reconnect(int slot, bool on)
{
    if (slot < 0 || slot >= TD_BT_SLOTS || !s_slots[slot].used)
        return false;
    if (!on)
        return ops_disconnect(slot);
    lock();
    bool ok = radio_start();
    if (ok)
    {
        s_slots[slot].reconnect = 1;
        slots_save();
        say(false, "reconnects by itself: wake the device up");
        kick();
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
        /* Off: the controller's memory goes back to the heap (on the
         * classic ESP32 it is reserved otherwise). */
        esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
        return NULL;
    }
#if CONFIG_IDF_TARGET_ESP32
    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);   /* LE only */
#endif
    s_lock = xSemaphoreCreateRecursiveMutex();
    if (!s_lock)
        return NULL;
    hid_input_init();
    slots_load();
    bool any = false;
    for (int i = 0; i < TD_BT_SLOTS; i++)
        any |= s_slots[i].used && s_slots[i].reconnect;
    if (any)
    {
        lock();
        radio_start();   /* paired devices come back by themselves */
        unlock();
    }
    ESP_LOGI(TAG, "Bluetooth keyboards and mice: %s", any ? "radio on for paired devices" : "radio off until a scan");
    return &s_ops;
}

#endif /* CONFIG_BT_NIMBLE_ENABLED */
