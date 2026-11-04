/*
 * network.c - Wi-Fi and Ethernet status, Wi-Fi scan / connect / forget,
 * the Telnet remote-desktop switch and SSH/SFTP and FTP server start/stop.
 * Opened from the start menu, the taskbar network indicator or Settings.
 * Any user may use it: saved networks belong to the user who added them
 * (root's are shared), which the port enforces.
 *
 * Everything goes through td_sysinfo()->net, supplied by the port; slow
 * operations run in the background there, so this window only polls.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "td_apps.h"

#define AP_MAX 24

static td_window_t *s_win;
static td_widget_t *s_wifi, *s_addr, *s_eth, *s_telnet, *s_peer, *s_list, *s_msg;
static td_widget_t *s_srv_label[2], *s_srv_button[2];
static td_wifi_ap_t s_aps[AP_MAX];
static int s_ap_count;
static bool s_scanning;
static char s_item[128]; /* SSID column: up to 32 bytes + padding, then bars and state */
static char s_target[33];

static const td_net_ops_t *net(void)
{
    return td_sysinfo()->net;
}

static const char *bars(int rssi)
{
    if (rssi >= -60)
        return "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x88";                 /* full */
    if (rssi >= -72)
        return "\xE2\x96\x88\xE2\x96\x88\xE2\x96\x91";
    return "\xE2\x96\x88\xE2\x96\x91\xE2\x96\x91";
}

static const char *get_item(td_widget_t *w, int index, int *fg, void *user)
{
    (void)w;
    (void)user;
    const td_wifi_ap_t *ap = &s_aps[index];
    td_net_status_t st;
    memset(&st, 0, sizeof(st));
    net()->status(&st);
    bool current = st.wifi_up && strcmp(st.ssid, ap->ssid) == 0;
    if (current)
        *fg = td_theme()->accent;
    /* The SSID is arbitrary bytes; pad it by cells, as the list draws it. */
    char ssid[80];
    td_utf8_pad(ssid, sizeof(ssid), ap->ssid, 32);
    snprintf(s_item, sizeof(s_item), "%s %s %-7s %s", ssid, bars(ap->rssi),
             ap->secure ? "secured" : "open", current ? "connected" : ap->saved ? "saved"
                                                                                : "");
    return s_item;
}

static void update_status(void)
{
    td_net_status_t st;
    memset(&st, 0, sizeof(st));
    net()->status(&st);
    if (st.wifi_up)
        td_widget_printf(s_wifi, "Wi-Fi:     %.32s (%d dBm)", st.ssid, st.rssi);
    else
        td_widget_printf(s_wifi, "Wi-Fi:     not connected");
    td_widget_printf(s_addr, "Address:   %s", st.wifi_up ? st.wifi_ip : "-");
    if (!st.eth_present)
        td_widget_printf(s_eth, "Ethernet:  not enabled");
    else
        td_widget_printf(s_eth, "Ethernet:  %s %s", st.eth_up ? "connected" : "no link", st.eth_up ? st.eth_ip : "");
    td_widget_printf(s_msg, "%s", st.busy ? "Working..." : st.message);

    static const char *const names[2] = {"SSH/SFTP:", "FTP:"};
    for (int i = 0; i < 2 && net()->server_status; i++)
    {
        int port = 0, clients = 0;
        bool on = net()->server_status(i, &port, &clients);
        if (on && i == TD_SERVER_SSH)
            td_widget_printf(s_srv_label[i], "%-10s running on port %d, %d client%s", names[i], port, clients,
                             clients == 1 ? "" : "s");
        else if (on)
            td_widget_printf(s_srv_label[i], "%-10s running on port %d", names[i], port);
        else
            td_widget_printf(s_srv_label[i], "%-10s stopped", names[i]);
        td_widget_set_text(s_srv_button[i], on ? "Stop" : "Start");
    }

    if (net()->telnet_peer)
    {
        const char *peer = net()->telnet_peer();
        td_widget_printf(s_peer, peer ? "in use from %s" : "", peer ? peer : "");
    }
    if (net()->telnet_enabled)
        td_checkbox_set(s_telnet, net()->telnet_enabled());
}

static void start_scan(void)
{
    if (net()->scan && net()->scan())
    {
        s_scanning = true;
        td_widget_set_text(s_msg, "Scanning...");
    }
}

static void on_tick(td_window_t *win)
{
    (void)win;
    update_status();
    if (s_scanning && net()->scan_results)
    {
        int n = net()->scan_results(s_aps, AP_MAX);
        if (n >= 0)
        {
            s_scanning = false;
            s_ap_count = n;
            td_list_set_count(s_list, n);
            if (n == 0)
                td_widget_set_text(s_msg, "No networks found.");
        }
    }
}

/* ---------------------------------------------------------- actions */

static const td_wifi_ap_t *selected(void)
{
    int i = td_list_selected(s_list);
    return i >= 0 && i < s_ap_count ? &s_aps[i] : NULL;
}

static void password_answer(const char *password, void *user)
{
    (void)user;
    if (net()->connect(s_target, password))
        td_widget_printf(s_msg, "Connecting to %.32s...", s_target);
}

static void do_connect(void)
{
    const td_wifi_ap_t *ap = selected();
    if (!ap || !net()->connect)
        return;
    snprintf(s_target, sizeof(s_target), "%s", ap->ssid);
    if (ap->saved)
    {
        password_answer(NULL, NULL);
    }
    else if (ap->secure)
    {
        char prompt[64];
        snprintf(prompt, sizeof(prompt), "Password for %.32s:", ap->ssid);
        td_passwordbox("Wi-Fi password", prompt, password_answer, NULL);
    }
    else
    {
        password_answer("", NULL);
    }
}

static void on_scan(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    start_scan();
}
static void on_connect(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    do_connect();
}

static void on_disconnect(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    if (net()->disconnect)
        net()->disconnect();
}

static void forget_answer(int button, void *user)
{
    (void)user;
    if (button != 0 || !net()->forget)
        return;
    if (net()->forget(s_target))
    {
        for (int i = 0; i < s_ap_count; i++)
            if (strcmp(s_aps[i].ssid, s_target) == 0)
                s_aps[i].saved = false;
        td_wm_invalidate();
    }
}

static void on_forget(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    const td_wifi_ap_t *ap = selected();
    if (!ap || !ap->saved)
    {
        td_widget_set_text(s_msg, "That network is not saved.");
        return;
    }
    snprintf(s_target, sizeof(s_target), "%s", ap->ssid);
    char text[64];
    snprintf(text, sizeof(text), "Forget %.32s?", ap->ssid);
    td_msgbox("Network", text, "Forget|Cancel", forget_answer, NULL);
}

static void on_telnet(td_widget_t *w, void *user)
{
    (void)user;
    if (net()->telnet_enable)
        net()->telnet_enable(td_checkbox_get(w));
}

static void on_activate(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    do_connect();
}

/* Start / Stop for a server; user = TD_SERVER_SSH or TD_SERVER_FTP. */
static void on_server(td_widget_t *w, void *user)
{
    (void)w;
    int which = (int)(intptr_t)user;
    if (!net()->server_status || !net()->server_set)
        return;
    bool on = net()->server_status(which, NULL, NULL);
    if (net()->server_set(which, !on))
        td_widget_set_text(s_msg, on ? "Stopping..." : "Starting...");
}

static void on_close(td_window_t *win)
{
    (void)win;
    s_win = NULL;
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    if (!net() || !net()->status)
    {
        td_msgbox("Network", "Network settings are not available\non this platform.", "OK", NULL, NULL);
        return;
    }
    td_window_desc_t d = {
        .title = "Network",
        .rect = td_rect(-1, -1, 68, 23),
        .flags = TD_WIN_DEFAULT,
        .min_w = 64,
        .min_h = 14,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = 1000,
    };
    s_win = td_win_create(&d);
    if (!s_win)
        return;

    s_wifi = td_label(s_win, 1, 0, 0, "");
    s_addr = td_label(s_win, 1, 1, 0, "");
    s_eth = td_label(s_win, 1, 2, 0, "");
    if (net()->telnet_enable)
    {
        s_telnet = td_checkbox(s_win, 1, 3, "Remote desktop over Telnet (port 23)", false, on_telnet, NULL);
        s_peer = td_label(s_win, 43, 3, 0, "");
    }
    if (net()->server_status)
    {
        for (int i = 0; i < 2; i++)
        {
            s_srv_label[i] = td_label(s_win, 1, 4 + i, 50, "");
            s_srv_button[i] = td_button(s_win, 52, 4 + i, "Start", on_server, (void *)(intptr_t)i);
        }
    }
    td_label(s_win, 1, 7, 0, "Wi-Fi networks (double-click to connect)");
    s_list = td_list(s_win, td_rect(0, 8, -1, -2), get_item, on_activate, NULL);
    td_scrollbar(s_win, -1, 8, -2, s_list);
    s_msg = td_label(s_win, 1, -2, 0, "");
    td_button(s_win, 0, -1, "Scan", on_scan, NULL);
    td_button(s_win, 9, -1, "Connect", on_connect, NULL);
    td_button(s_win, 21, -1, "Disconnect", on_disconnect, NULL);
    td_button(s_win, 36, -1, "Forget", on_forget, NULL);
    td_widget_focus(s_list);
    update_status();
    start_scan();
}

static const td_app_t s_app = {"Network", launch, "(("};

void td_network_register(void)
{
    td_app_register(&s_app);
}
