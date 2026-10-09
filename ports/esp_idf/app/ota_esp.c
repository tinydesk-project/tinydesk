/*
 * ota_esp.c - firmware updates for the ESP-IDF boards (td_ota_ops_t and the `ota`
 * shell command).
 *
 * The flash has two app slots (ota_0, ota_1). An update is written to the
 * slot that is not running, checked (image format, chip, SHA-256) and made
 * the boot slot; the next restart runs it "on trial". If it crashes before
 * ota_esp_boot_ok() confirms it (30 s after start-up), the bootloader goes
 * back to the previous version on its own.
 *
 * Sources: an http:// or https:// URL (HTTPS is checked against the ESP-IDF
 * certificate bundle), or a .bin file on the device (copied with SFTP, FTP
 * or SMB). The work runs in a short-lived task; the Software Update app and
 * the shell poll its status.
 *
 * Official releases: every release puts a small update feed next to the web
 * installer (update-desktop-<board>.json, from tools/make_release.py) with
 * the version, date, size and the app image's path. check_official() reads
 * it; the board key update.url points at another feed (self-hosting,
 * tests). Whether to check daily, and the version the user was last told
 * about, are kept in NVS (namespace td_update).
 */
#include "ota_esp.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_image_format.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "cJSON.h"
#include "nvs.h"
#include "tdsh_board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "tdsh.h"

static const char *TAG = "ota";

#define CHUNK           4096
#define URL_TASK_STACK  8192     /* HTTPS handshake */
#define FILE_TASK_STACK 4096

static SemaphoreHandle_t s_lock;
static td_ota_status_t s_st;
static char s_source[256];
static bool s_check_only;
static volatile bool s_cancel;
static volatile bool s_busy;
static int64_t s_start_us;

/* The official update feed. */
#define UPDATE_SITE "https://tinydesk-project.github.io/install/"
#if CONFIG_IDF_TARGET_ESP32C6
#define UPDATE_BOARD "esp32c6"
#elif CONFIG_IDF_TARGET_ESP32S3
#define UPDATE_BOARD "esp32s3"
#else
#define UPDATE_BOARD "esp32"          /* the classic ESP32 with PSRAM; the 4 MB one has no OTA */
#endif
#define FEED_MAX 2048
enum
{
    JOB_IMAGE,
    JOB_FEED
};
static int s_job;
static bool s_quiet;                  /* a background feed check: status() stays */
static td_ota_release_t s_rel;

static void lock(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
}
static void unlock(void)
{
    xSemaphoreGive(s_lock);
}

static void set_state(int state, const char *fmt, ...)
{
    lock();
    s_st.state = state;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s_st.message, sizeof(s_st.message), fmt, ap);
    va_end(ap);
    unlock();
    ESP_LOGI(TAG, "%s", s_st.message);
}

static void set_progress(uint32_t done, uint32_t total)
{
    int64_t us = esp_timer_get_time() - s_start_us;
    lock();
    s_st.done = done;
    s_st.total = total;
    s_st.percent = total ? (int)((uint64_t)done * 100 / total) : -1;
    s_st.bytes_per_s = us > 200000 ? (uint32_t)((uint64_t)done * 1000000 / (uint64_t)us) : 0;
    unlock();
}

static void set_new_version(const esp_app_desc_t *d)
{
    lock();
    snprintf(s_st.new_version, sizeof(s_st.new_version), "%s", d->version);
    snprintf(s_st.new_built, sizeof(s_st.new_built), "%s %s", d->date, d->time);
    unlock();
}

/* After a check: say whether it is newer, older or the same. */
static void report_check(void)
{
    const esp_app_desc_t *me = esp_app_get_description();
    lock();
    char v[32];
    snprintf(v, sizeof(v), "%s", s_st.new_version);
    unlock();
    if (!strcmp(v, me->version))
        set_state(TD_OTA_IDLE, "That is version %s, the one installed.", v);
    else
        set_state(TD_OTA_IDLE, "Version %s is available (installed: %s).", v, me->version);
}

/* ------------------------------------------------------------ from a URL */

static void from_url(void)
{
    esp_http_client_config_t http = {
        .url = s_source,
        .timeout_ms = 15000,
        .keep_alive_enable = true,
        .buffer_size = 2048,
        .buffer_size_tx = 1024,
    };
    if (!strncmp(s_source, "https://", 8))
        http.crt_bundle_attach = esp_crt_bundle_attach;
    esp_https_ota_config_t cfg = {.http_config = &http};
    esp_https_ota_handle_t h = NULL;
    esp_err_t err = esp_https_ota_begin(&cfg, &h);
    if (err != ESP_OK)
    {
        set_state(TD_OTA_FAILED, "Cannot download it: %s", esp_err_to_name(err));
        return;
    }
    esp_app_desc_t d;
    if (esp_https_ota_get_img_desc(h, &d) == ESP_OK)
        set_new_version(&d);
    if (s_check_only)
    {
        esp_https_ota_abort(h);
        report_check();
        return;
    }
    int size = esp_https_ota_get_image_size(h);
    set_state(TD_OTA_INSTALLING, "Downloading and installing version %s...", d.version);
    for (;;)
    {
        err = esp_https_ota_perform(h);
        set_progress((uint32_t)esp_https_ota_get_image_len_read(h), size > 0 ? (uint32_t)size : 0);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS)
            break;
        if (s_cancel)
        {
            esp_https_ota_abort(h);
            set_state(TD_OTA_FAILED, "Cancelled. The installed version is unchanged.");
            return;
        }
    }
    if (err != ESP_OK || !esp_https_ota_is_complete_data_received(h))
    {
        esp_https_ota_abort(h);
        set_state(TD_OTA_FAILED, "Download failed: %s", esp_err_to_name(err));
        return;
    }
    err = esp_https_ota_finish(h);     /* checks the image, sets the boot slot */
    if (err != ESP_OK)
    {
        set_state(TD_OTA_FAILED, err == ESP_ERR_OTA_VALIDATE_FAILED ? "The image is damaged or not for this chip." : "Could not finish: %s",
                  esp_err_to_name(err));
        return;
    }
    set_state(TD_OTA_DONE, "Version %s is installed. Restart to use it.", d.version);
}

/* ---------------------------------------------------------- from a file */

static void from_file(void)
{
    FILE *f = fopen(s_source, "rb");
    if (!f)
    {
        set_state(TD_OTA_FAILED, "Cannot open the file.");
        return;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(CHUNK);
    if (!buf)
    {
        fclose(f);
        set_state(TD_OTA_FAILED, "Not enough memory.");
        return;
    }
    size_t n = fread(buf, 1, CHUNK, f);
    const size_t desc_at = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t);
    esp_app_desc_t d;
    if (n < desc_at + sizeof(d) || buf[0] != ESP_IMAGE_HEADER_MAGIC)
    {
        set_state(TD_OTA_FAILED, "That is not an ESP32 firmware image (.bin).");
        goto out;
    }
    memcpy(&d, buf + desc_at, sizeof(d));
    if (d.magic_word != ESP_APP_DESC_MAGIC_WORD)
    {
        set_state(TD_OTA_FAILED, "That is not an application image.");
        goto out;
    }
    set_new_version(&d);
    if (s_check_only)
    {
        report_check();
        goto out;
    }
    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
    if (!next || (uint32_t)size > next->size)
    {
        set_state(TD_OTA_FAILED, "The image (%ld KB) does not fit the update slot.", size / 1024);
        goto out;
    }
    esp_ota_handle_t h;
    esp_err_t err = esp_ota_begin(next, OTA_WITH_SEQUENTIAL_WRITES, &h);
    if (err != ESP_OK)
    {
        set_state(TD_OTA_FAILED, "Cannot start: %s", esp_err_to_name(err));
        goto out;
    }
    set_state(TD_OTA_INSTALLING, "Installing version %s...", d.version);
    uint32_t done = 0;
    while (n > 0)
    {
        err = esp_ota_write(h, buf, n);
        if (err != ESP_OK)
            break;
        done += (uint32_t)n;
        set_progress(done, (uint32_t)size);
        if (s_cancel)
        {
            esp_ota_abort(h);
            set_state(TD_OTA_FAILED, "Cancelled. The installed version is unchanged.");
            goto out;
        }
        n = fread(buf, 1, CHUNK, f);
    }
    if (err != ESP_OK)
    {
        esp_ota_abort(h);
        set_state(TD_OTA_FAILED, "Writing failed: %s", esp_err_to_name(err));
        goto out;
    }
    err = esp_ota_end(h);
    if (err == ESP_OK)
        err = esp_ota_set_boot_partition(next);
    if (err != ESP_OK)
    {
        set_state(TD_OTA_FAILED, err == ESP_ERR_OTA_VALIDATE_FAILED ? "The image is damaged or not for this chip." : "Could not finish: %s",
                  esp_err_to_name(err));
        goto out;
    }
    set_state(TD_OTA_DONE, "Version %s is installed. Restart to use it.", d.version);
out:
    free(buf);
    fclose(f);
}

static bool is_url(const char *s)
{
    return !strncmp(s, "http://", 7) || !strncmp(s, "https://", 8);
}

/* ---------------------------------------------------- official releases */

/* Update information can move: a feed that says "moved": "<https URL of the
 * feed at its new address>" makes the board keep that address in NVS
 * (td_update/feed) and read it from then on, so later firmware keeps
 * getting updates even if the site's address changes again. */
static void moved_feed(char *out, size_t cap)
{
    nvs_handle_t h;
    out[0] = '\0';
    if (nvs_open("td_update", NVS_READONLY, &h) != ESP_OK)
        return;
    size_t n = cap;
    if (nvs_get_str(h, "feed", out, &n) != ESP_OK)
        out[0] = '\0';
    nvs_close(h);
}

static void set_moved_feed(const char *url)
{
    nvs_handle_t h;
    if (nvs_open("td_update", NVS_READWRITE, &h) != ESP_OK)
        return;
    if (url && url[0])
        nvs_set_str(h, "feed", url);
    else
        nvs_erase_key(h, "feed");
    nvs_commit(h);
    nvs_close(h);
}

/* Where the update information is read, first that applies: the board key
 * update.url (the user's own choice), an address a feed moved to, the
 * official site. `origin` (may be NULL) says which. */
static void feed_url_from(char *out, size_t cap, const char **origin)
{
    const char *u = tdsh_board_get("update.url");
    char moved[200];
    moved_feed(moved, sizeof(moved));
    const char *why;
    if (u && is_url(u))
    {
        snprintf(out, cap, "%s", u);
        why = "board key update.url";
    }
    else if (is_url(moved))
    {
        snprintf(out, cap, "%s", moved);
        why = "moved there by the update information";
    }
    else
    {
        snprintf(out, cap, "%supdate-desktop-%s.json", UPDATE_SITE, UPDATE_BOARD);
        why = "built in";
    }
    if (origin)
        *origin = why;
}

static void feed_url(char *out, size_t cap)
{
    feed_url_from(out, cap, NULL);
}

/* "0.1.10" > "0.1.9": the first three numbers, anything after them ignored. */
static bool version_newer(const char *a, const char *b)
{
    for (int i = 0; i < 3; i++)
    {
        unsigned long x = strtoul(a, (char **)&a, 10), y = strtoul(b, (char **)&b, 10);
        if (x != y)
            return x > y;
        if (*a == '.')
            a++;
        if (*b == '.')
            b++;
    }
    return false;
}

/* GET a small file into buf. Returns its length, or -1 with err set. */
static int http_get_small(const char *url, char *buf, int cap, char *err, size_t err_cap)
{
    esp_http_client_config_t http = {
        .url = url,
        .timeout_ms = 15000,
        .buffer_size = 2048,
        .buffer_size_tx = 1024,
    };
    if (!strncmp(url, "https://", 8))
        http.crt_bundle_attach = esp_crt_bundle_attach;
    esp_http_client_handle_t c = esp_http_client_init(&http);
    if (!c)
    {
        snprintf(err, err_cap, "Not enough memory to connect.");
        return -1;
    }
    int n = -1;
    esp_err_t e = esp_http_client_open(c, 0);
    if (e != ESP_OK)
    {
        snprintf(err, err_cap, "Cannot reach the update server: %s", esp_err_to_name(e));
        goto out;
    }
    esp_http_client_fetch_headers(c);
    int code = esp_http_client_get_status_code(c);
    if (code == 404)
    {
        snprintf(err, err_cap, "No update information for this board on the server (404).");
        goto out;
    }
    if (code != 200)
    {
        snprintf(err, err_cap, "The update server answered HTTP %d.", code);
        goto out;
    }
    n = 0;
    for (;;)
    {
        int r = esp_http_client_read(c, buf + n, cap - 1 - n);
        if (r <= 0)
            break;
        n += r;
        if (n >= cap - 1)
        {
            snprintf(err, err_cap, "The update information is too large.");
            n = -1;
            goto out;
        }
    }
    buf[n] = '\0';
out:
    esp_http_client_close(c);
    esp_http_client_cleanup(c);
    return n;
}

static const char *json_str(const cJSON *o, const char *key)
{
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, key);
    return cJSON_IsString(v) && v->valuestring ? v->valuestring : "";
}

/* A feed's "moved" address is followed only from a feed read over HTTPS, to
 * another HTTPS address, and not when the user set update.url. Returns true
 * (and the new address in url) when the board should read it now. */
static bool follow_move(const cJSON *j, char *url, size_t cap)
{
    const char *moved = j ? json_str(j, "moved") : "";
    const char *own = tdsh_board_get("update.url");
    if (!moved[0] || (own && is_url(own)))
        return false;
    if (strncmp(url, "https://", 8) != 0 || strncmp(moved, "https://", 8) != 0 || strlen(moved) >= cap ||
        strcmp(moved, url) == 0)
        return false;
    set_moved_feed(moved);
    ESP_LOGI(TAG, "update information moved to %s", moved);
    snprintf(url, cap, "%s", moved);
    return true;
}

static void from_feed(void)
{
    char url[200], err[96] = "";
    feed_url(url, sizeof(url));
    td_ota_release_t r = {0};
    char *buf = malloc(FEED_MAX);
    cJSON *j = NULL;
    for (int hop = 0; hop < 2; hop++)
    {
        err[0] = '\0';
        int n = buf ? http_get_small(url, buf, FEED_MAX, err, sizeof(err)) : -1;
        if (!buf)
            snprintf(err, sizeof(err), "Not enough memory to check.");
        j = n > 0 ? cJSON_Parse(buf) : NULL;
        if (n > 0 && !j)
            snprintf(err, sizeof(err), "The update information is not valid JSON.");
        if (hop == 0 && follow_move(j, url, sizeof(url)))
        {
            cJSON_Delete(j); /* read it at its new address now */
            j = NULL;
            continue;
        }
        break;
    }
    const char *ver = j ? json_str(j, "version") : "";
    const char *image = j ? json_str(j, "image") : "";
    if (j && (!ver[0] || !image[0]))
        snprintf(err, sizeof(err), "The update information has no version or image.");
    if (!err[0])
    {
        r.valid = true;
        snprintf(r.version, sizeof(r.version), "%s", ver);
        snprintf(r.date, sizeof(r.date), "%s", json_str(j, "date"));
        snprintf(r.notes, sizeof(r.notes), "%s", json_str(j, "notes"));
        const cJSON *size = cJSON_GetObjectItemCaseSensitive(j, "size");
        r.size = cJSON_IsNumber(size) && size->valuedouble > 0 ? (uint32_t)size->valuedouble : 0;
        if (is_url(image))
        {
            snprintf(r.url, sizeof(r.url), "%s", image);
        }
        else
        {                               /* relative to the feed */
            const char *slash = strrchr(url, '/');
            int base = slash ? (int)(slash - url) + 1 : 0;
            snprintf(r.url, sizeof(r.url), "%.*s%s", base, url, image);
        }
        r.newer = version_newer(r.version, esp_app_get_description()->version);
    }
    cJSON_Delete(j);
    free(buf);

    const char *mine = esp_app_get_description()->version;
    lock();
    r.checks = s_rel.checks + 1;
    snprintf(r.error, sizeof(r.error), "%s", err);
    if (!r.valid && s_rel.valid)
    {          /* keep what was known; note the failure */
        td_ota_release_t keep = s_rel;
        snprintf(keep.error, sizeof(keep.error), "%s", err);
        keep.checks = r.checks;
        r = keep;
    }
    s_rel = r;
    unlock();
    if (s_quiet)
    {
        ESP_LOGI(TAG, "official release check: %s", err[0] ? err : r.version);
        return;
    }
    if (err[0])
        set_state(TD_OTA_FAILED, "%s", err);
    else if (r.newer)
        set_state(TD_OTA_IDLE, "TinyDesk %s is available (installed: %s). Install it?", r.version, mine);
    else if (!strcmp(r.version, mine))
        set_state(TD_OTA_IDLE, "TinyDesk %s, the newest release, is installed.", mine);
    else
        set_state(TD_OTA_IDLE, "Installed %s is newer than the newest release (%s).", mine, r.version);
}

static void ota_task(void *arg)
{
    (void)arg;
    s_start_us = esp_timer_get_time();
    if (s_job == JOB_FEED)
        from_feed();
    else if (is_url(s_source))
        from_url();
    else
        from_file();
    s_busy = false;
    vTaskDelete(NULL);
}

/* ---------------------------------------------------------------- ops */

static void ota_info(td_ota_info_t *out)
{
    memset(out, 0, sizeof(*out));
    const esp_app_desc_t *d = esp_app_get_description();
    snprintf(out->version, sizeof(out->version), "%s", d->version);
    snprintf(out->built, sizeof(out->built), "%s %s", d->date, d->time);
    snprintf(out->sdk, sizeof(out->sdk), "%s", d->idf_ver);
    const esp_partition_t *run = esp_ota_get_running_partition();
    const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
    const esp_partition_t *boot = esp_ota_get_boot_partition();
    if (run)
        snprintf(out->running, sizeof(out->running), "%s", run->label);
    if (next)
        snprintf(out->next, sizeof(out->next), "%s", next->label);
    esp_ota_img_states_t st;
    if (run && esp_ota_get_state_partition(run, &st) == ESP_OK)
        out->on_trial = st == ESP_OTA_IMG_PENDING_VERIFY;
    esp_app_desc_t od;
    if (next && esp_ota_get_partition_description(next, &od) == ESP_OK)
    {
        snprintf(out->other_version, sizeof(out->other_version), "%s", od.version);
        esp_ota_img_states_t ost = ESP_OTA_IMG_UNDEFINED;
        esp_ota_get_state_partition(next, &ost);
        /* Not when an installed update is only waiting for the restart. */
        out->can_roll_back = ost != ESP_OTA_IMG_INVALID && ost != ESP_OTA_IMG_ABORTED && boot == run;
    }
}

static bool ota_start(const char *source, bool check_only)
{
    if (s_busy || !source || !source[0])
        return false;
    s_job = JOB_IMAGE;
    snprintf(s_source, sizeof(s_source), "%s", source);
    s_check_only = check_only;
    s_cancel = false;
    lock();
    memset(&s_st, 0, sizeof(s_st));
    s_st.state = check_only ? TD_OTA_CHECKING : TD_OTA_INSTALLING;
    s_st.percent = -1;
    snprintf(s_st.message, sizeof(s_st.message), "%s", check_only ? "Checking..." : "Starting...");
    unlock();
    s_busy = true;
    if (xTaskCreate(ota_task, "td_ota", is_url(source) ? URL_TASK_STACK : FILE_TASK_STACK, NULL, 4, NULL) != pdPASS)
    {
        s_busy = false;
        set_state(TD_OTA_FAILED, "Not enough memory to start.");
        return false;
    }
    return true;
}

static void ota_status(td_ota_status_t *out)
{
    lock();
    *out = s_st;
    unlock();
}

static void ota_cancel(void)
{
    s_cancel = true;
}

static void ota_restart(void)
{
    ESP_LOGW(TAG, "restarting for the new firmware");
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_restart();
}

static bool ota_roll_back(void)
{
    const esp_partition_t *run = esp_ota_get_running_partition();
    esp_ota_img_states_t st;
    if (run && esp_ota_get_state_partition(run, &st) == ESP_OK && st == ESP_OTA_IMG_PENDING_VERIFY)
    {
        esp_ota_mark_app_invalid_rollback_and_reboot();   /* does not return on success */
        return false;
    }
    const esp_partition_t *other = esp_ota_get_next_update_partition(NULL);
    esp_app_desc_t d;
    if (!other || esp_ota_get_partition_description(other, &d) != ESP_OK)
        return false;
    if (esp_ota_set_boot_partition(other) != ESP_OK)
        return false;
    ota_restart();
    return true;
}

static bool ota_check_official(bool quiet)
{
    if (s_busy)
        return false;
    s_job = JOB_FEED;
    s_quiet = quiet;
    if (!quiet)
    {
        lock();
        memset(&s_st, 0, sizeof(s_st));
        s_st.state = TD_OTA_CHECKING;
        s_st.percent = -1;
        snprintf(s_st.message, sizeof(s_st.message), "Looking up the newest official release...");
        unlock();
    }
    s_busy = true;
    if (xTaskCreate(ota_task, "td_ota", URL_TASK_STACK, NULL, 4, NULL) != pdPASS)
    {
        s_busy = false;
        if (!quiet)
            set_state(TD_OTA_FAILED, "Not enough memory to check.");
        return false;
    }
    return true;
}

static void ota_official(td_ota_release_t *out)
{
    lock();
    *out = s_rel;
    unlock();
}

/* NVS namespace td_update: auto (u8, default 1), told (the last version notified). */
static bool ota_auto_check(void)
{
    nvs_handle_t h;
    uint8_t v = 1;
    if (nvs_open("td_update", NVS_READONLY, &h) == ESP_OK)
    {
        nvs_get_u8(h, "auto", &v);
        nvs_close(h);
    }
    return v != 0;
}

static void ota_set_auto_check(bool on)
{
    nvs_handle_t h;
    if (nvs_open("td_update", NVS_READWRITE, &h) != ESP_OK)
        return;
    nvs_set_u8(h, "auto", on ? 1 : 0);
    nvs_commit(h);
    nvs_close(h);
}

static void ota_notified(char *out, int cap)
{
    nvs_handle_t h;
    out[0] = '\0';
    if (nvs_open("td_update", NVS_READONLY, &h) != ESP_OK)
        return;
    size_t n = (size_t)cap;
    if (nvs_get_str(h, "told", out, &n) != ESP_OK)
        out[0] = '\0';
    nvs_close(h);
}

static void ota_set_notified(const char *version)
{
    nvs_handle_t h;
    if (nvs_open("td_update", NVS_READWRITE, &h) != ESP_OK)
        return;
    nvs_set_str(h, "told", version ? version : "");
    nvs_commit(h);
    nvs_close(h);
}

/* Board settings that only the running firmware has (built in from a
 * board.conf). Official images are built without any, so installing one
 * would start without them: Software Update and `ota install` offer to save
 * them on the board first (`board save`). */
static int ota_unsaved_settings(void)
{
    return tdsh_board_unsaved();
}

static bool ota_save_settings(char *msg, int cap)
{
    int n = tdsh_board_save_builtin();
    if (n < 0)
    {
        snprintf(msg, (size_t)cap, "Cannot save the board settings: %s", strerror(-n));
        return false;
    }
    snprintf(msg, (size_t)cap, "Saved %d board setting%s in /etc/board.conf.", n, n == 1 ? "" : "s");
    return true;
}

static const td_ota_ops_t s_ops = {
    .info = ota_info,
    .start = ota_start,
    .status = ota_status,
    .cancel = ota_cancel,
    .restart = ota_restart,
    .roll_back = ota_roll_back,
    .check_official = ota_check_official,
    .official = ota_official,
    .auto_check = ota_auto_check,
    .set_auto_check = ota_set_auto_check,
    .notified = ota_notified,
    .set_notified = ota_set_notified,
    .unsaved_settings = ota_unsaved_settings,
    .save_settings = ota_save_settings,
};

/* A restart we were asked for (Start > Exit, `reboot`, Restart now) means
 * the new firmware ran well enough to take commands: confirm it, so it is
 * not rolled back. Crashes, watchdogs and power loss do not come through
 * here (no shutdown handlers run), so they still roll back. */
static void on_shutdown(void)
{
    if (esp_timer_get_time() >= 5 * 1000000LL)
        ota_esp_boot_ok();
}

const td_ota_ops_t *ota_esp_ops(void)
{
    if (!s_lock)
    {
        s_lock = xSemaphoreCreateMutex();
        esp_register_shutdown_handler(on_shutdown);
    }
    return &s_ops;
}

void ota_esp_boot_ok(void)
{
    const esp_partition_t *run = esp_ota_get_running_partition();
    esp_ota_img_states_t st;
    if (run && esp_ota_get_state_partition(run, &st) == ESP_OK && st == ESP_OTA_IMG_PENDING_VERIFY)
    {
        esp_ota_mark_app_valid_cancel_rollback();
        ESP_LOGI(TAG, "this firmware (%s) is confirmed", esp_app_get_description()->version);
    }
}

/* ------------------------------------------------------ `ota` command */

static void print_info(void)
{
    td_ota_info_t i;
    ota_info(&i);
    printf("Firmware:  TinyDesk %s, built %s, ESP-IDF %s\n", i.version, i.built, i.sdk);
    printf("Running:   %s%s\n", i.running, i.on_trial ? " (on trial: confirms itself 30 s after start-up)" : "");
    if (i.other_version[0])
        printf("Other slot: %s has version %s%s\n", i.next, i.other_version, i.can_roll_back ? " (ota rollback)" : "");
    else
        printf("Other slot: %s is empty\n", i.next);
}

static int wait_job(void)
{
    int last = -10;
    td_ota_status_t st;
    for (;;)
    {
        ota_status(&st);
        if (st.state == TD_OTA_INSTALLING && st.percent >= last + 10)
        {
            printf("  %3d%%  %u of %u KB  %u KB/s\n", st.percent, (unsigned)(st.done / 1024),
                   (unsigned)(st.total / 1024), (unsigned)(st.bytes_per_s / 1024));
            fflush(stdout);
            last = st.percent;
        }
        if (!s_busy)
            break;
        vTaskDelay(pdMS_TO_TICKS(250));
    }
    ota_status(&st);
    printf("%s\n", st.message);
    return st.state == TD_OTA_FAILED ? 1 : 0;
}

static int cmd_ota(tdsh_session_t *session, int argc, char **argv)
{
    const char *op = argc > 1 ? argv[1] : "status";
    if (!strcmp(op, "status"))
    {
        print_info();
        td_ota_status_t st;
        ota_status(&st);
        if (st.message[0])
            printf("Last:      %s\n", st.message);
        return 0;
    }
    bool force = !strcmp(op, "install") && argc == 4 && !strcmp(argv[2], "-f");
    if ((!strcmp(op, "check") || !strcmp(op, "install")) && (argc == 3 || force))
    {
        char real[200];
        const char *src = argv[argc - 1];
        int unsaved = ota_unsaved_settings();
        if (!strcmp(op, "install") && !force && unsaved > 0)
        {
            printf("ota: %d board setting%s (pins) %s built into this firmware only: firmware\n"
                   "built without them, such as an official release, starts without them.\n"
                   "Save them on the board first with `board save`, or install anyway with\n"
                   "`ota install -f %s`.\n",
                   unsaved, unsaved == 1 ? "" : "s", unsaved == 1 ? "is" : "are", src);
            return 1;
        }
        if (!is_url(src))
        {
            if (tdsh_path_to_real(session, src, real, sizeof(real), NULL, 0) != 0)
            {
                printf("ota: %s is not allowed\n", src);
                return 1;
            }
            src = real;
        }
        if (!ota_start(src, !strcmp(op, "check")))
        {
            printf("ota: an update is already running\n");
            return 1;
        }
        return wait_job();
    }
    if (!strcmp(op, "official") && argc == 2)
    {
        if (!ota_check_official(false))
        {
            printf("ota: an update is already running\n");
            return 1;
        }
        int rc = wait_job();
        td_ota_release_t r;
        ota_official(&r);
        if (rc == 0 && r.valid)
        {
            printf("Newest:    TinyDesk %s%s%s, %u KB\n", r.version, r.date[0] ? " of " : "", r.date,
                   (unsigned)(r.size / 1024));
            printf("Image:     %s\n", r.url);
            if (r.notes[0])
                printf("Notes:     %s\n", r.notes);
            if (r.newer)
                printf("Install:   ota install %s\n", r.url);
        }
        return rc;
    }
    if (!strcmp(op, "feed") && (argc == 2 || (argc == 3 && !strcmp(argv[2], "reset"))))
    {
        if (argc == 3)
            set_moved_feed(NULL); /* back to the built-in address */
        char url[200];
        const char *origin = "";
        feed_url_from(url, sizeof(url), &origin);
        printf("Update information: %s\n(%s)\n", url, origin);
        return 0;
    }
    if (!strcmp(op, "notify"))
    {
        if (argc == 3 && (!strcmp(argv[2], "on") || !strcmp(argv[2], "off")))
        {
            bool on = !strcmp(argv[2], "on");
            ota_set_auto_check(on);
            if (on)
                ota_set_notified("");      /* tell about the newest release again */
        }
        else if (argc != 2)
        {
            printf("usage: ota notify [on|off]\n");
            return 2;
        }
        printf("Daily check for official updates: %s\n", ota_auto_check() ? "on" : "off");
        return 0;
    }
    if (!strcmp(op, "cancel"))
    {
        ota_cancel();
        printf("Cancelling...\n");
        return 0;
    }
    if (!strcmp(op, "restart"))
    {
        ota_restart();
        return 0;
    }
    if (!strcmp(op, "rollback"))
    {
        td_ota_info_t i;
        ota_info(&i);
        if (!i.on_trial && !i.can_roll_back)
        {
            printf("ota: there is no previous version to go back to\n");
            return 1;
        }
        printf("Going back to %s and restarting...\n", i.on_trial ? "the previous version" : i.other_version);
        fflush(stdout);
        return ota_roll_back() ? 0 : 1;
    }
    printf("usage:\n"
           "  ota status                    installed version, slots, last result\n"
           "  ota official                  look up the newest official release\n"
           "  ota notify [on|off]           daily check and notice (on: tell again)\n"
           "  ota feed [reset]              where the update information is read (reset:\n"
           "                                forget an address it moved to)\n"
           "  ota check <url|file>          show the version of an update\n"
           "  ota install [-f] <url|file>   install it (then: ota restart); -f: even with\n"
           "                                board settings that only this firmware has\n"
           "  ota cancel | restart | rollback\n"
           "url: http://... or https://... to a TinyDesk .bin; file: a .bin on this device.\n");
    return 2;
}

static const tdsh_command_t s_cmd = {
    "ota",
    "ota <status|official|feed|notify|check|install|cancel|restart|rollback> ...",
    "Firmware update (OTA)",
    cmd_ota,
    TDSH_CMD_ROOT_ONLY,
};

int ota_esp_register_command(void)
{
    return tdsh_register_command(&s_cmd);
}
