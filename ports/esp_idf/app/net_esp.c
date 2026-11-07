/*
 * net_esp.c - td_net_ops_t for the ESP-IDF boards on top of TinyDesk Shell's Wi-Fi and
 * Ethernet modules, so the Network app shares TinyDesk Shell's saved networks.
 *
 * Scans and connects can take seconds, so each runs in a short-lived worker
 * task; the UI only reads the results. Server start/stop runs on the
 * caller's task instead: the SSH server wants 96 KiB of free internal RAM
 * when it starts, and a worker's stack would eat the margin. Status is cached for a second
 * because the taskbar asks for it on every frame.
 */
#include "net_esp.h"

#include <stdio.h>
#include <string.h>

#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "td_apps.h"
#include "telnet.h"
#include "tdsh_espidf.h"

#define AP_MAX       24
#define WORKER_STACK 6144

typedef enum
{
    JOB_SCAN,
    JOB_CONNECT,
    JOB_DISCONNECT
} job_t;

static SemaphoreHandle_t s_lock;
static volatile bool s_busy;
static job_t s_job;
static char s_ssid[33];
static char s_pass[65];
static bool s_use_saved;
static char s_user[32];          /* who asked (saved networks are per user) */
static td_wifi_ap_t s_aps[AP_MAX];
static int s_ap_count;
static bool s_scan_done;
static char s_msg[64];
static td_net_status_t s_cache;
static int64_t s_cache_us;

static void set_msg(const char *fmt, const char *arg)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    snprintf(s_msg, sizeof(s_msg), fmt, arg);
    xSemaphoreGive(s_lock);
}

static void worker(void *arg)
{
    (void)arg;
    switch (s_job)
    {
    case JOB_SCAN:
    {
        tdsh_wifi_ap_t found[AP_MAX];
        int n = tdsh_wifi_scan_list(found, AP_MAX, s_user);
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_ap_count = n < 0 ? 0 : n;
        for (int i = 0; i < s_ap_count; i++)
        {
            memcpy(s_aps[i].ssid, found[i].ssid, sizeof(s_aps[i].ssid));
            s_aps[i].rssi = found[i].rssi;
            s_aps[i].secure = found[i].secure;
            s_aps[i].saved = found[i].saved;
        }
        s_scan_done = true;
        snprintf(s_msg, sizeof(s_msg), "%s", n < 0 ? "Scan failed" : "");
        xSemaphoreGive(s_lock);
        break;
    }
    case JOB_CONNECT:
        if (!s_use_saved && tdsh_wifi_save(s_ssid, s_pass, s_user) != 0)
        {
            set_msg("Could not save %.32s", s_ssid);
        }
        else if (tdsh_wifi_connect_saved(s_ssid, s_user) == 0)
        {
            set_msg("Connected to %.32s", s_ssid);
        }
        else
        {
            set_msg("Could not connect to %.32s", s_ssid);
        }
        memset(s_pass, 0, sizeof(s_pass));
        break;
    case JOB_DISCONNECT:
        set_msg("%s", tdsh_wifi_disconnect_now() == 0 ? "Disconnected" : "Disconnect failed");
        break;
    }
    s_cache_us = 0;          /* refresh the status right away */
    s_busy = false;
    vTaskDelete(NULL);
}

static bool start_job(job_t job)
{
    if (s_busy)
        return false;
    s_busy = true;
    s_job = job;
    snprintf(s_user, sizeof(s_user), "%s", td_session_user());
    if (xTaskCreate(worker, "td_net", WORKER_STACK, NULL, 3, NULL) != pdPASS)
    {
        s_busy = false;
        set_msg("%s", "Not enough memory");
        return false;
    }
    return true;
}

/* ------------------------------------------------------------- ops */

static void ip_text(uint32_t addr, char *out, size_t cap)
{
    snprintf(out, cap, "%u.%u.%u.%u", (unsigned)(addr & 0xFF), (unsigned)((addr >> 8) & 0xFF),
             (unsigned)((addr >> 16) & 0xFF), (unsigned)(addr >> 24));
}

static void net_status(td_net_status_t *out)
{
    int64_t now = esp_timer_get_time();
    if (s_cache_us == 0 || now - s_cache_us > 1000000)
    {
        td_net_status_t st;
        memset(&st, 0, sizeof(st));
        tdsh_wifi_info_t wi;
        if (tdsh_wifi_get_info(&wi) == 0 && wi.connected)
        {
            st.wifi_up = true;
            snprintf(st.ssid, sizeof(st.ssid), "%s", wi.ssid);
            st.rssi = wi.rssi;
            ip_text(wi.ip.addr, st.wifi_ip, sizeof(st.wifi_ip));
        }
        tdsh_eth_info_t ei;
        if (tdsh_eth_get_info(&ei) == 0)
        {
            st.eth_present = ei.enabled;
            st.eth_up = ei.connected;
            if (ei.connected)
                ip_text(ei.ip.addr, st.eth_ip, sizeof(st.eth_ip));
        }
        s_cache = st;
        s_cache_us = now;
    }
    *out = s_cache;
    out->busy = s_busy;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    snprintf(out->message, sizeof(out->message), "%s", s_msg);
    xSemaphoreGive(s_lock);
}

static bool net_scan(void)
{
    s_scan_done = false;
    return start_job(JOB_SCAN);
}

static int net_scan_results(td_wifi_ap_t *out, int max)
{
    if (!s_scan_done)
        return s_busy ? -1 : 0;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    int n = s_ap_count < max ? s_ap_count : max;
    memcpy(out, s_aps, sizeof(td_wifi_ap_t) * (size_t)n);
    xSemaphoreGive(s_lock);
    return n;
}

static bool net_connect(const char *ssid, const char *password)
{
    if (!td_session_has_right(TD_RIGHT_NETWORK))
        return false;
    if (s_busy)
        return false;
    snprintf(s_ssid, sizeof(s_ssid), "%s", ssid);
    s_use_saved = password == NULL;
    snprintf(s_pass, sizeof(s_pass), "%s", password ? password : "");
    return start_job(JOB_CONNECT);
}

static bool net_disconnect(void)
{
    if (!td_session_has_right(TD_RIGHT_NETWORK))
        return false;
    return start_job(JOB_DISCONNECT);
}

static bool net_forget(const char *ssid)
{
    if (!td_session_has_right(TD_RIGHT_NETWORK))
        return false;
    int rc = tdsh_wifi_forget(ssid, td_session_user());
    if (rc == -2)
        set_msg("%s", "Shared network: only root can forget it");
    else if (rc != 0)
        set_msg("%s", "Could not forget that network");
    else
        set_msg("Forgot %.32s", ssid);
    return rc == 0;
}

static bool net_server_status(int which, int *port, int *clients)
{
    uint16_t p = 0;
    int c = 0;
    bool on = which == TD_SERVER_SSH ? tdsh_ssh_is_running(&p, &c) : tdsh_ftp_is_running(&p);
    if (port)
        *port = p;
    if (clients)
        *clients = c;
    return on;
}

static bool net_server_set(int which, bool on)
{
    if (!td_session_has_right(TD_RIGHT_NETWORK))
        return false;
    if (on && !tdsh_remote_access_ready())
    {
        set_msg("%s", "First run passwd in Terminal (old: " TDSH_FACTORY_ROOT_PASSWORD ")");
        return false;
    }
    if (s_busy)
        return false;
    const char *name = which == TD_SERVER_SSH ? "SSH/SFTP" : "FTP";
    int rc = which == TD_SERVER_SSH ? tdsh_ssh_set_running(on) : tdsh_ftp_set_running(on);
    char msg[64];
    if (rc == 0)
        snprintf(msg, sizeof(msg), "%s server %s", name, on ? "started" : "stopped");
    else if (on && !tdsh_network_is_online())
        snprintf(msg, sizeof(msg), "%s: not started, no network", name);
    else if (on)
        snprintf(msg, sizeof(msg), "%s: not started (%u KB internal RAM free)", name,
                 (unsigned)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) / 1024u));
    else
        snprintf(msg, sizeof(msg), "%s server could not stop", name);
    set_msg("%s", msg);
    s_cache_us = 0;
    return true;
}

static void net_telnet_enable(bool on)
{
    if (!td_session_has_right(TD_RIGHT_NETWORK))
        return;
    telnet_set_enabled(on);
    set_msg("%s", on && !telnet_enabled()
                      ? "Telnet: run passwd in Terminal (old: " TDSH_FACTORY_ROOT_PASSWORD ")"
                      : "Telnet is unencrypted; root login only");
}

static const td_net_ops_t s_ops = {
    .status = net_status,
    .scan = net_scan,
    .scan_results = net_scan_results,
    .connect = net_connect,
    .disconnect = net_disconnect,
    .forget = net_forget,
    .server_status = net_server_status,
    .server_set = net_server_set,
    .telnet_enabled = telnet_enabled,
    .telnet_enable = net_telnet_enable,
    .telnet_peer = telnet_peer,
};

const td_net_ops_t *net_esp_ops(void)
{
    if (!s_lock)
        s_lock = xSemaphoreCreateMutex();
    return &s_ops;
}
