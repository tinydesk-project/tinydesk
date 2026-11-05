/*
 * fetch_esp.c - downloads for the ESP-IDF boards (td_fetch_ops_t): the Store
 * app's catalog and app packages.
 *
 * One download at a time, in a short-lived task: an http:// or https:// URL
 * (HTTPS checked against the ESP-IDF certificate bundle, up to 4
 * redirects) or a file on the device is copied into a file, replacing it.
 * A failed or cancelled download removes what it wrote.
 */
#include "fetch_esp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define CHUNK      2048
#define TASK_STACK 8192      /* HTTPS handshake */
#define REDIRECTS  4

static SemaphoreHandle_t s_lock;
static td_fetch_status_t s_st;
static char s_source[256], s_dest[256];
static uint32_t s_max;
static volatile bool s_cancel, s_busy;

static void set(int state, uint32_t done, uint32_t total, const char *msg)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_st.state = state;
    s_st.done = done;
    s_st.total = total;
    if (msg)
        snprintf(s_st.message, sizeof(s_st.message), "%s", msg);
    xSemaphoreGive(s_lock);
}

/* Copies from read_fn into the open file f; returns NULL or an error. */
typedef int (*read_fn)(void *src, char *buf, int cap);
static const char *pump(read_fn rd, void *src, FILE *f, uint32_t total, char *buf)
{
    uint32_t done = 0;
    for (;;)
    {
        if (s_cancel)
            return "Cancelled.";
        int n = rd(src, buf, CHUNK);
        if (n < 0)
            return "The connection broke off.";
        if (n == 0)
            break;
        done += (uint32_t)n;
        if ((s_max && done > s_max) || (total && done > total))
            return "The download is larger than expected.";
        if (fwrite(buf, 1, (size_t)n, f) != (size_t)n)
            return "The file system is full.";
        set(TD_FETCH_RUNNING, done, total, NULL);
    }
    if (total && done != total)
        return "The download ended early.";
    return NULL;
}

static int read_http(void *src, char *buf, int cap)
{
    return esp_http_client_read((esp_http_client_handle_t)src, buf, cap);
}

static int read_file(void *src, char *buf, int cap)
{
    size_t n = fread(buf, 1, (size_t)cap, (FILE *)src);
    return ferror((FILE *)src) ? -1 : (int)n;
}

static const char *from_url(FILE *f, char *buf, char *msg, size_t cap)
{
    esp_http_client_config_t http = {
        .url = s_source,
        .timeout_ms = 15000,
        .buffer_size = 2048,
        .buffer_size_tx = 1024,
        .crt_bundle_attach = esp_crt_bundle_attach,   /* used for https:// only */
    };
    esp_http_client_handle_t c = esp_http_client_init(&http);
    if (!c)
        return "Not enough memory to connect.";
    const char *err = NULL;
    int code = 0;
    int64_t length = 0;
    for (int i = 0;; i++)
    {
        esp_err_t e = esp_http_client_open(c, 0);
        if (e != ESP_OK)
        {
            snprintf(msg, cap, "Cannot reach the server: %s", esp_err_to_name(e));
            err = msg;
            goto out;
        }
        length = esp_http_client_fetch_headers(c);
        code = esp_http_client_get_status_code(c);
        if (code < 300 || code > 308 || code == 304 || i == REDIRECTS)
            break;
        esp_http_client_flush_response(c, NULL);
        if (esp_http_client_set_redirection(c) != ESP_OK)
            break;
        esp_http_client_close(c);
    }
    if (code != 200)
    {
        snprintf(msg, cap, code == 404 ? "Not found on the server (404)." : "The server answered HTTP %d.", code);
        err = msg;
        goto out;
    }
    if (s_max && length > (int64_t)s_max)
    {
        err = "The download is larger than expected.";
        goto out;
    }
    err = pump(read_http, c, f, length > 0 ? (uint32_t)length : 0, buf);
out:
    esp_http_client_close(c);
    esp_http_client_cleanup(c);
    return err;
}

static void task(void *arg)
{
    (void)arg;
    static char msg[96];
    const char *err = NULL;
    char *buf = malloc(CHUNK);
    FILE *f = buf ? fopen(s_dest, "wb") : NULL;
    if (!buf)
        err = "Not enough memory.";
    else if (!f)
        err = "Cannot write the file.";
    else if (!strncmp(s_source, "http://", 7) || !strncmp(s_source, "https://", 8))
        err = from_url(f, buf, msg, sizeof(msg));
    else
    {
        const char *path = !strncmp(s_source, "file://", 7) ? s_source + 7 : s_source;
        FILE *in = fopen(path, "rb");
        if (!in)
            err = "The source file does not exist.";
        else
        {
            err = pump(read_file, in, f, 0, buf);
            fclose(in);
        }
    }
    if (f && fclose(f) != 0 && !err)
        err = "The file system is full.";
    if (err && f)
        remove(s_dest);
    free(buf);
    td_fetch_status_t st;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    st = s_st;
    xSemaphoreGive(s_lock);
    set(err ? TD_FETCH_FAILED : TD_FETCH_DONE, st.done, err ? st.total : st.done, err ? err : "");
    s_busy = false;
    vTaskDelete(NULL);
}

static bool fetch_start(const char *source, const char *dest, uint32_t max)
{
    if (s_busy || !source || !dest || strlen(source) >= sizeof(s_source) || strlen(dest) >= sizeof(s_dest))
        return false;
    snprintf(s_source, sizeof(s_source), "%s", source);
    snprintf(s_dest, sizeof(s_dest), "%s", dest);
    s_max = max;
    s_cancel = false;
    s_busy = true;
    set(TD_FETCH_RUNNING, 0, 0, "");
    if (xTaskCreate(task, "fetch", TASK_STACK, NULL, 4, NULL) != pdPASS)
    {
        s_busy = false;
        set(TD_FETCH_FAILED, 0, 0, "Not enough memory to start the download.");
        return false;
    }
    return true;
}

static void fetch_status(td_fetch_status_t *out)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_st;
    xSemaphoreGive(s_lock);
}

static void fetch_cancel(void)
{
    s_cancel = true;
}

static const td_fetch_ops_t s_ops = {fetch_start, fetch_status, fetch_cancel};

const td_fetch_ops_t *fetch_esp_ops(void)
{
    if (!s_lock)
        s_lock = xSemaphoreCreateMutex();
    return s_lock ? &s_ops : NULL;
}
