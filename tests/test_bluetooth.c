/*
 * test_bluetooth.c - the Bluetooth window (apps/bluetooth.c) inside the real
 * window manager, with the port's table (td_sysinfo()->bt, td_bt_ops_t)
 * replaced by a recorder: which requests the buttons make, which ask first,
 * how a scan's devices are offered, and how a refusal is shown. Written by
 * aquasock for TinyTang (tools/tests/tb_bluetooth_app.c) and taken into
 * TinyDesk with the window.
 *
 * Copyright (c) 2026 the TinyTang contributors. MIT License, as TinyDesk.
 */
#undef NDEBUG /* the checks are asserts: keep them in Release builds */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "td_apps.h"

/* ------------------------------------------------------- the fake engine */

static td_bt_status_t s_info;
static td_bt_device_t s_devs[TD_BT_MAX_DEVICES];
static int s_ndevs;
static const char *s_refuse;          /* the next request's refusal */
static int s_scans, s_pairs, s_forgets, s_offs, s_ons;
static int s_last_slot;
static td_bt_device_t s_last_dev;
static bool s_scanning;               /* scan_results() answers -1 while set */

static void fake_status(td_bt_status_t *out)
{
    *out = s_info;
}

static int fake_scan_results(td_bt_device_t *out, int max)
{
    if (s_scanning)
    {
        return -1;
    }
    const int n = s_ndevs < max ? s_ndevs : max;
    if (out != NULL && n > 0)
    {
        memcpy(out, s_devs, sizeof(s_devs[0]) * (size_t)n);
    }
    return n;
}

static bool take(bool *ok)
{
    const char *why = s_refuse;
    s_refuse = NULL;
    if (why != NULL)
    {
        snprintf(s_info.message, sizeof s_info.message, "%s", why);
        *ok = false;
        return false;
    }
    s_info.message[0] = '\0';
    *ok = true;
    return true;
}

static bool fake_scan(void)
{
    bool ok;
    const bool taken = take(&ok);
    if (taken)
    {
        s_scans++;
        s_info.busy = true;
        s_scanning = true;
    }
    s_info.job_failed = !ok;
    return taken;
}

static bool fake_pair(int slot, const td_bt_device_t *dev)
{
    bool ok;
    const bool taken = take(&ok);
    if (taken)
    {
        s_pairs++;
        s_last_slot = slot;
        s_last_dev = *dev;
    }
    s_info.job_failed = !ok;
    return taken;
}

static bool fake_disconnect(int slot)
{
    bool ok;
    const bool taken = take(&ok);
    if (taken)
    {
        s_offs++;
        s_last_slot = slot;
    }
    s_info.job_failed = !ok;
    return taken;
}

static bool fake_forget(int slot)
{
    bool ok;
    const bool taken = take(&ok);
    if (taken)
    {
        s_forgets++;
        s_last_slot = slot;
    }
    s_info.job_failed = !ok;
    return taken;
}

static bool fake_set_reconnect(int slot, bool on)
{
    bool ok;
    const bool taken = take(&ok);
    if (taken)
    {
        on ? s_ons++ : s_offs++;
        s_last_slot = slot;
    }
    s_info.job_failed = !ok;
    return taken;
}

static int fake_paired(td_bt_paired_t *out, int max)
{
    int n = 0;
    for (int i = 0; i < TD_BT_SLOTS && n < max; i++)
    {
        if (!s_info.slot[i].paired)
        {
            continue;
        }
        memset(&out[n], 0, sizeof out[n]);
        out[n].slot = i;
        memcpy(out[n].addr, s_info.slot[i].paired_addr, sizeof out[n].addr);
        snprintf(out[n].name, sizeof out[n].name, "%s", s_info.slot[i].paired_name);
        out[n].connected = s_info.slot[i].ready;
        out[n].reconnects = s_info.slot[i].reconnects;
        n++;
    }
    return n;
}

static const td_bt_ops_t s_fake = {
    .status = fake_status,
    .scan = fake_scan,
    .scan_results = fake_scan_results,
    .pair = fake_pair,
    .disconnect = fake_disconnect,
    .paired = fake_paired,
    .forget = fake_forget,
    .set_reconnect = fake_set_reconnect,
};


/* The engine's job finishing.  Through the interface a scan is over when
 * scan_results() stops answering -1; the message is what status() reports. */
static void job_done(const char *msg)
{
    s_info.busy = false;
    s_scanning = false;
    snprintf(s_info.message, sizeof s_info.message, "%s", msg);
}

/* ---------------------------------------------- TinyDesk on a fake terminal */

static uint32_t s_now = 10000;
static const char *s_reply = "\x1b[45;80R";   /* the size query's answer */

static int fake_read(void *ctx)
{
    (void)ctx;
    return *s_reply ? (uint8_t)*s_reply++ : -1;
}
static int fake_write(void *ctx, const uint8_t *b, int n)
{
    (void)ctx;
    (void)b;
    return n;
}
static uint32_t fake_millis(void *ctx)
{
    (void)ctx;
    return s_now;
}
static void fake_sleep(void *ctx, uint32_t ms)
{
    (void)ctx;
    s_now += ms;
}
static const td_hal_t s_hal = {fake_read, fake_write, fake_millis, fake_sleep, NULL};

static void key(uint32_t k)
{
    td_event_t ev = {0};
    ev.type = TD_EV_KEY;
    ev.key = k;
    ev.time_ms = s_now;
    td_wm_dispatch(&ev);
}

static void click(int x, int y)
{
    td_event_t ev = {0};
    ev.type = TD_EV_MOUSE;
    ev.button = TD_BUTTON_LEFT;
    ev.x = (int16_t)x;
    ev.y = (int16_t)y;
    ev.time_ms = s_now;
    ev.action = TD_MOUSE_PRESS;
    td_wm_dispatch(&ev);
    ev.action = TD_MOUSE_RELEASE;
    td_wm_dispatch(&ev);
    s_now += 500;                      /* never a double click */
}

static void tick(void)
{
    s_now += 250;
    td_timers_run(s_now);
}

static td_buffer_t s_screen;

/* Whether `text` is on the screen; its row in *row. */
static bool on_screen(const char *text, int *row)
{
    td_wm_compose(&s_screen);
    for (int y = 0; y < s_screen.rows; y++)
    {
        char line[128];
        int n = 0;
        for (int x = 0; x < s_screen.cols && n < (int)sizeof(line) - 1; x++)
        {
            const uint32_t ch = td_buffer_cell(&s_screen, x, y)->ch;
            line[n++] = ch < 128 && ch >= 32 ? (char)ch : ' ';
        }
        line[n] = '\0';
        if (strstr(line, text) != NULL)
        {
            if (row != NULL)
            {
                *row = y;
            }
            return true;
        }
    }
    return false;
}

static td_window_t *s_win;
static td_rect_t s_c;

/* The window's layout: slot buttons on row 5 + 5 * slot + 3, Pair...,
 * Off/On and Forget at columns 11, 23 and 31; the scan's Pair on row 30, which
 * is SCAN_Y + LIST_ROWS + 2 with three slots above it. */
static void press_slot(int slot, int column)
{
    click(s_c.x + column + 1, s_c.y + 8 + slot * 5);
}
#define SCAN_PAIR_ROW 30
#define PAIR_BTN      11
#define TOGGLE_BTN    23
#define FORGET_BTN    31

static void set_slot(int i, bool paired, bool ready, bool reconnects, uint8_t a0)
{
    td_bt_slot_t *s = &s_info.slot[i];
    memset(s, 0, sizeof(*s));
    s->state = ready ? "ready" : paired && reconnects ? "waiting"
                                                      : "idle";
    s->ready = ready;
    s->active = ready || (paired && reconnects);
    s->paired = paired;
    s->reconnects = reconnects;
    s->paired_addr[0] = a0;
    s->addr[0] = a0;
    snprintf(s->paired_name, sizeof(s->paired_name), "%s", i == 0 ? "Logi K950" : "Logi M750");
}

static td_bt_device_t dev(td_bt_kind_t kind, const char *name, int rssi, uint8_t a0)
{
    td_bt_device_t d;
    memset(&d, 0, sizeof(d));
    d.kind = kind;
    d.rssi = (int8_t)rssi;
    d.addr[0] = a0;
    snprintf(d.name, sizeof(d.name), "%s", name);
    return d;
}

int main(void)
{
    td_buffer_init(&s_screen, 80, 45);
    td_init(&s_hal);
    static td_sysinfo_t info;
    info.bt = &s_fake;
    td_set_sysinfo(&info);
    td_bluetooth_register();

    s_info.radio = TD_BT_RADIO_OFF;
    s_info.pairings = "loaded from the card";
    set_slot(0, true, true, true, 0xD9);     /* the K950, in use */
    set_slot(1, true, false, true, 0x5C);    /* the M750, waiting */

    /* The interface's "list paired devices" verb, which the window does not
     * use -- it reads status().slot[] -- but a caller that knows only the table
     * would.  Exercised here so it is not shipped untested. */
    {
        td_bt_paired_t list[TD_BT_SLOTS];
        const int n = td_sysinfo()->bt->paired(list, TD_BT_SLOTS);
        assert(n == 2);
        assert(list[0].slot == 0 && list[0].connected && list[0].reconnects);
        assert(list[0].addr[0] == 0xD9);
        assert(list[1].slot == 1 && !list[1].connected && list[1].reconnects);
        assert(strcmp(list[0].name, "Logi K950") == 0);
    }

    assert(td_app_launch("Bluetooth"));
    s_win = td_win_focused();
    assert(s_win != NULL);
    s_c = td_win_client(s_win);
    tick();
    assert(on_screen("off; a scan starts it", NULL));
    assert(on_screen("loaded from the card", NULL));

    /* Forget asks first; Esc keeps the pairing, Enter forgets. */
    press_slot(0, FORGET_BTN);
    assert(td_win_focused() != s_win);
    assert(on_screen("stops working at once", NULL));   /* it is connected */
    key(TD_KEY_ESC);
    assert(td_win_focused() == s_win && s_forgets == 0);
    press_slot(0, FORGET_BTN);
    key(TD_KEY_ENTER);
    assert(s_forgets == 1 && s_last_slot == 0);
    job_done("pairing deleted");
    tick();
    assert(on_screen("pairing deleted", NULL));

    /* Off on a connected device asks; on a waiting one it does not. */
    press_slot(0, TOGGLE_BTN);
    assert(td_win_focused() != s_win);
    key(TD_KEY_ENTER);
    assert(s_offs == 1 && s_last_slot == 0);
    job_done("not reconnecting; pairing kept");
    press_slot(1, TOGGLE_BTN);
    assert(td_win_focused() == s_win && s_offs == 2 && s_last_slot == 1);
    job_done("not reconnecting; pairing kept");

    /* On needs no confirmation; the button follows the slot. */
    s_info.slot[1].reconnects = false;
    tick();
    assert(on_screen("not reconnecting", NULL));
    press_slot(1, TOGGLE_BTN);
    assert(s_ons == 1 && s_last_slot == 1);
    job_done("reconnects by itself");

    /* A refused scan says why on the bottom line and shows no list. */
    s_refuse = "starting the radio needs 32 KB free; 20 KB is free";
    press_slot(1, PAIR_BTN);
    tick();
    assert(s_scans == 0);
    assert(on_screen("needs 32 KB free; 20 KB is free", NULL));
    assert(!on_screen("Pair a mouse", NULL));

    /* A scan for the mouse: its own kind first, then other HID devices, then
     * named ones; unnamed others are left out. */
    press_slot(1, PAIR_BTN);
    assert(s_scans == 1);
    s_info.scan_ms_left = 6000;
    tick();
    assert(on_screen("Pair a mouse", NULL));
    assert(on_screen("Listening... 6 s", NULL));
    s_devs[0] = dev(TD_BT_KIND_OTHER, "", -40, 1);
    s_devs[1] = dev(TD_BT_KIND_OTHER, "Living room TV", -50, 2);
    s_devs[2] = dev(TD_BT_KIND_KEYBOARD, "Some keyboard", -60, 3);
    s_devs[3] = dev(TD_BT_KIND_HID, "Gamepad", -65, 4);
    s_devs[4] = dev(TD_BT_KIND_MOUSE, "New mouse", -70, 5);
    s_ndevs = 5;
    s_info.scan_ms_left = 0;
    job_done("5 device(s) found");
    tick();
    int r_mouse, r_kbd, r_pad, r_tv;
    assert(on_screen("4 found", NULL));
    assert(on_screen("New mouse", &r_mouse) && on_screen("Some keyboard", &r_kbd));
    assert(on_screen("Gamepad", &r_pad) && on_screen("Living room TV", &r_tv));
    assert(r_mouse < r_kbd && r_kbd < r_pad && r_pad < r_tv);
    assert(!on_screen("(no name)", NULL));

    /* Pairing over a different paired device asks to replace it. */
    click(s_c.x + 2, s_c.y + SCAN_PAIR_ROW);
    assert(td_win_focused() != s_win && on_screen("Pairing this one", NULL));
    key(TD_KEY_ENTER);
    assert(s_pairs == 1 && s_last_slot == 1 && s_last_dev.addr[0] == 5);
    assert(!on_screen("Pair a mouse", NULL));   /* back to the slots */

    printf("test_bluetooth: PASS\n");
    return 0;
}
