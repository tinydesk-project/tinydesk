/*
 * main.c - tinydesk on the ESP-IDF boards (ESP32-C6, classic ESP32).
 *
 * Boot order:
 *   1. NVS, then TinyDesk Shell (mounts LittleFS at /fs, users, network manager).
 *   2. esp_log is hooked so every log line lands in the Log Viewer (the
 *      ESP-IDF console is off: the UARTs are the desktop or RS-485 lines).
 *   3. The tinydesk task takes over the board's link (link.h) and runs the
 *      desktop. TinyDesk Shell starts in its own task when the Terminal opens.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "esp_chip_info.h"
#include "esp_event.h"
#include "esp_heap_caps.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_netif.h"
#include "esp_rom_sys.h"
#include "esp_sleep.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sdkconfig.h"

#include "hal_mux.h"
#include "link.h"
#include "net_esp.h"
#include "ota_esp.h"
#include "td_modbus.h"
#include "td_proto_cmds.h"
#include "td_apps.h"
#include "telnet.h"
#include "td_fs_stdio.h"
#include "tdsh_bridge_esp.h"
#include "tdsh_espidf.h"

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3
#define UI_TASK_STACK 8192   /* Xtensa frames are bigger; RAM is plentiful (PSRAM) */
#else
#define UI_TASK_STACK 5120   /* measured on the C6: about 1.8 KB used */
#endif
#define TZ_NAME          "/.tdsh_tz"   /* TinyDesk Shell's per-user `tz` setting, in the home */
#define UI_TASK_PRIORITY 5

static const char *TAG = "tinydesk";

/* ------------------------------------------------------------ sysinfo */

/* Internal RAM; PSRAM (classic ESP32 port) is reported on its own. */
#define INTERNAL (MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)
static uint32_t free_heap(void)
{
    return (uint32_t)heap_caps_get_free_size(INTERNAL);
}
static uint32_t min_free_heap(void)
{
    return (uint32_t)heap_caps_get_minimum_free_size(INTERNAL);
}
static uint32_t total_heap(void)
{
    return (uint32_t)heap_caps_get_total_size(INTERNAL);
}
static uint32_t psram_free(void)
{
    return (uint32_t)heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
}
static uint32_t psram_total(void)
{
    return (uint32_t)heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
}
static int task_count(void)
{
    return (int)uxTaskGetNumberOfTasks();
}

/* The board configuration built into the firmware (the port's board.conf,
 * or board.example.conf; see main/CMakeLists.txt). */
extern const char s_board_builtin[] asm("_binary_board_builtin_conf_start");

#if CONFIG_FREERTOS_USE_TRACE_FACILITY && CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
/* The previous sample, to turn run-time counters into CPU shares. The
 * buffers are allocated on the first call (when the Task Manager opens) and
 * kept, growing only when there are more tasks: a refresh then needs no
 * memory, so the list does not go blank when RAM is short. */
typedef struct
{
    TaskHandle_t handle;
    configRUN_TIME_COUNTER_TYPE run;
} task_sample_t;
static task_sample_t *s_prev, *s_now;
static TaskStatus_t *s_status;
static UBaseType_t s_task_cap;
static int s_prev_count;
static configRUN_TIME_COUNTER_TYPE s_prev_total;

static bool grow_task_buffers(UBaseType_t cap)
{
    if (cap <= s_task_cap)
        return true;
    TaskStatus_t *st = realloc(s_status, cap * sizeof(*st));
    if (!st)
        return false;
    s_status = st;
    task_sample_t *prev = realloc(s_prev, cap * sizeof(*prev));
    if (!prev)
        return false;
    s_prev = prev;
    task_sample_t *now = realloc(s_now, cap * sizeof(*now));
    if (!now)
        return false;
    s_now = now;
    s_task_cap = cap;
    return true;
}

static int list_tasks(td_task_info_t *out, int max)
{
    if (!grow_task_buffers(uxTaskGetNumberOfTasks() + 4))
        return -1;
    TaskStatus_t *st = s_status;
    task_sample_t *now = s_now;
    configRUN_TIME_COUNTER_TYPE total = 0;
    UBaseType_t n = uxTaskGetSystemState(st, s_task_cap, &total);
    configRUN_TIME_COUNTER_TYPE elapsed = total - s_prev_total;   /* wraps fine */
    bool have_prev = s_prev_count > 0 && elapsed > 0;
    int k = 0;
    for (UBaseType_t i = 0; i < n; i++)
    {
        now[i].handle = st[i].xHandle;
        now[i].run = st[i].ulRunTimeCounter;
        if (k >= max)
            continue;
        td_task_info_t *t = &out[k++];
        snprintf(t->name, sizeof(t->name), "%s", st[i].pcTaskName);
        switch (st[i].eCurrentState)
        {
        case eRunning:
            t->state = 'R';
            break;
        case eReady:
            t->state = 'r';
            break;
        case eBlocked:
            t->state = 'B';
            break;
        case eSuspended:
            t->state = 'S';
            break;
        default:
            t->state = 'D';
            break;
        }
        t->priority = (uint8_t)st[i].uxCurrentPriority;
        BaseType_t core = xTaskGetCoreID(st[i].xHandle);
        t->core = (int8_t)(core == tskNO_AFFINITY || core < 0 ? -1 : core);
        t->stack_free = (uint32_t)st[i].usStackHighWaterMark;    /* bytes on ESP-IDF */
        t->cpu_tenths = -1;
        for (int j = 0; have_prev && j < s_prev_count; j++)
        {
            if (s_prev[j].handle != st[i].xHandle)
                continue;
            uint64_t ran = (uint64_t)(configRUN_TIME_COUNTER_TYPE)(st[i].ulRunTimeCounter - s_prev[j].run);
            uint64_t tenths = ran * 1000u / ((uint64_t)elapsed * portNUM_PROCESSORS);
            t->cpu_tenths = (int16_t)(tenths > 1000 ? 1000 : tenths);
            break;
        }
    }
    s_now = s_prev;                 /* swap: this sample becomes the previous one */
    s_prev = now;
    s_prev_count = (int)n;
    s_prev_total = total;
    return k;
}
#endif
static int cpu_mhz(void)
{
    return CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;
}

/* Time zone: the same offset file TinyDesk Shell's `tz` command uses, so the
 * taskbar clock and `date` agree. Re-read now and then (tz may change it). */
static long s_tz;
static int64_t s_tz_read_us = -1;
static char s_tz_user[32];

/* The logged-in user's own tz file. */
static void tz_path(char *out, size_t cap)
{
    snprintf(out, cap, "%.140s" TZ_NAME, td_session_home());
}

static bool get_tz(long *seconds)
{
    int64_t now = esp_timer_get_time();
    bool other_user = strcmp(s_tz_user, td_session_user()) != 0;
    if (s_tz_read_us < 0 || other_user || now - s_tz_read_us > 30000000)
    {
        char path[160];
        tz_path(path, sizeof(path));
        snprintf(s_tz_user, sizeof(s_tz_user), "%s", td_session_user());
        s_tz = 0;
        FILE *f = fopen(path, "r");
        if (f)
        {
            if (fscanf(f, "%ld", &s_tz) != 1)
                s_tz = 0;
            fclose(f);
        }
        s_tz_read_us = now;
    }
    *seconds = s_tz;
    return true;
}

static bool set_tz(long seconds)
{
    char path[160];
    tz_path(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (!f)
        return false;
    fprintf(f, "%ld\n", seconds);
    bool ok = fclose(f) == 0;
    s_tz_read_us = -1;
    return ok;
}

/* The system clock, once SNTP (or Date & time) has set it. */
static bool time_now(int64_t *utc)
{
    time_t now = time(NULL);
    if (now < 1700000000)
        return false;   /* not set yet */
    *utc = (int64_t)now;
    return true;
}

static bool time_set(int64_t utc)
{
    struct timeval tv = {.tv_sec = (time_t)utc, .tv_usec = 0};
    return settimeofday(&tv, NULL) == 0;
}

static bool time_auto_get(void)
{
    return tdsh_time_auto();
}
static bool time_auto_set(bool on)
{
    return tdsh_time_set_auto(on) == 0;
}

/* "Sync now" waits for SNTP for a few seconds, so it runs in a short-lived
 * task. */
static volatile bool s_syncing;

static void sync_task(void *arg)
{
    (void)arg;
    (void)tdsh_time_sync_now();
    s_syncing = false;
    vTaskDelete(NULL);
}

static bool time_sync(void)
{
    if (s_syncing || !tdsh_network_is_online())
        return false;
    s_syncing = true;
    if (xTaskCreate(sync_task, "td_sntp", 4096, NULL, 3, NULL) != pdPASS)
    {
        s_syncing = false;
        return false;
    }
    return true;
}

static bool settings_load(void *data, int len)
{
    nvs_handle_t h;
    if (nvs_open("tinydesk", NVS_READONLY, &h) != ESP_OK)
        return false;
    size_t size = (size_t)len;
    esp_err_t err = nvs_get_blob(h, "settings", data, &size);
    nvs_close(h);
    return err == ESP_OK && size == (size_t)len;
}

static bool settings_save(const void *data, int len)
{
    nvs_handle_t h;
    if (nvs_open("tinydesk", NVS_READWRITE, &h) != ESP_OK)
        return false;
    esp_err_t err = nvs_set_blob(h, "settings", data, (size_t)len);
    if (err == ESP_OK)
        err = nvs_commit(h);
    nvs_close(h);
    return err == ESP_OK;
}

static void confirm_firmware(void *arg)
{
    (void)arg;
    ota_esp_boot_ok();
}

static char s_chip[48];
static char s_sdk[32];
static td_sysinfo_t s_info;

static void fill_sysinfo(void)
{
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    const char *model = chip.model == CHIP_ESP32     ? "ESP32"
                        : chip.model == CHIP_ESP32C6 ? "ESP32-C6"
                        : chip.model == CHIP_ESP32S3 ? "ESP32-S3"
                                                     : CONFIG_IDF_TARGET;
    snprintf(s_chip, sizeof(s_chip), "%s rev %d.%d, %d core%s", model, chip.revision / 100, chip.revision % 100,
             chip.cores, chip.cores == 1 ? "" : "s");
    snprintf(s_sdk, sizeof(s_sdk), "ESP-IDF %s", esp_get_idf_version());

    s_info.platform = board_platform();
    s_info.chip = s_chip;
    s_info.sdk_version = s_sdk;
    s_info.free_heap = free_heap;
    s_info.min_free_heap = min_free_heap;
    s_info.total_heap = total_heap;
    s_info.psram_free = psram_free;
    s_info.psram_total = psram_total;
    s_info.task_count = task_count;
#if CONFIG_FREERTOS_USE_TRACE_FACILITY && CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS
    s_info.tasks = list_tasks;
#endif
    s_info.cpu_mhz = cpu_mhz;
    s_info.time_now = time_now;
    s_info.time_set = time_set;
    s_info.time_auto_get = time_auto_get;
    s_info.time_auto_set = time_auto_set;
    s_info.time_sync = time_sync;
    s_info.get_tz = get_tz;
    s_info.set_tz = set_tz;
    s_info.net = net_esp_ops();
    /* Updates need a second app slot (not on the 4 MB ESP32 layout). */
    if (esp_ota_get_next_update_partition(NULL))
    {
        s_info.ota = ota_esp_ops();
    }
    else
    {
        s_info.no_ota_text =
            "This board cannot update itself: its flash has one app slot\n"
            "(4 MB layout), and an update needs a second one to download into.\n"
            "\n"
            "Install the new release with the web installer instead:\n"
            "  https://tinydesk-project.github.io/install/\n"
            "Leave \"Erase device\" off: your files stay; users, Wi-Fi networks\n"
            "and passwords start fresh.";
    }
    s_info.user_exists = tdsh_user_exists;
    s_info.authenticate = tdsh_user_authenticate;
    s_info.settings_load = settings_load;
    s_info.settings_save = settings_save;
    s_info.fs = td_fs_stdio(TDSH_MOUNT_POINT);
    /* A mounted SD card (sd mount) shows up in Files as the folder "sd". */
    td_fs_stdio_redirect(TDSH_MOUNT_POINT "/sd", "/sd", tdsh_sdcard_mounted);
    s_info.extra = "Shell:     TinyDesk Shell " TDSH_VERSION;
    td_set_sysinfo(&s_info);
}

/* ----------------------------------------------------- log capture */

static vprintf_like_t s_prev_vprintf;
static portMUX_TYPE s_log_mux = portMUX_INITIALIZER_UNLOCKED;

static void log_lock(void *ctx)
{
    (void)ctx;
    taskENTER_CRITICAL(&s_log_mux);
}
static void log_unlock(void *ctx)
{
    (void)ctx;
    taskEXIT_CRITICAL(&s_log_mux);
}

/* Every esp_log line: copy into the Log Viewer ring, then print it as
 * before (to the Terminal window for the shell task; nowhere otherwise). */
static int log_hook(const char *fmt, va_list ap)
{
    char line[128];
    va_list copy;
    va_copy(copy, ap);
    vsnprintf(line, sizeof(line), fmt, copy);
    va_end(copy);
    td_log_append(0, line);
    return s_prev_vprintf ? s_prev_vprintf(fmt, ap) : 0;
}

/* ----------------------------------------------------------- UI task */

static void ui_task(void *arg)
{
    (void)arg;
    const td_hal_t *hal = hal_mux_init();
    if (!hal)
    {
        vTaskDelete(NULL);
        return;
    }
    td_init(hal);
    td_apps_register_all();
    td_terminal_set_backend(tdsh_bridge_esp_backend());
    ESP_LOGI(TAG, "desktop %dx%d, UI stack free %u bytes", td_stats()->cols, td_stats()->rows,
             (unsigned)uxTaskGetStackHighWaterMark(NULL));

    uint32_t last_report = 0;
    while (td_step())
    {
        hal->sleep_ms(hal->ctx, TD_LOOP_SLEEP_MS);
        uint32_t now = td_millis();
        if (now - last_report > 60000)
        {
            last_report = now;
            ESP_LOGI(TAG, "UI stack min free %u B, heap free %u B (min %u B)",
                     (unsigned)uxTaskGetStackHighWaterMark(NULL), (unsigned)free_heap(),
                     (unsigned)min_free_heap());
        }
    }
    /* Start menu > Exit: restart the board. */
    td_shutdown();
    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
}

/* Keep ROM / early-boot output off the USB port, which belongs to the
 * desktop (see sdkconfig.defaults). */
static void quiet_usb_port(void)
{
    esp_rom_install_channel_putc(2, NULL);   /* ROM printf: not on USB... */
    esp_rom_install_channel_putc(1, NULL);   /* ...nor on UART0 (RS-485 on the C6, the desktop on the ESP32) */
    esp_deep_sleep_disable_rom_logging();    /* no ROM banner on the next warm reset */
    esp_log_level_set("*", ESP_LOG_INFO);    /* boot ran at WARN; normal logging again */
}

void app_main(void)
{
    quiet_usb_port();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* The TCP/IP stack and the default event loop, up front: the desktop's
     * Telnet listener (and MQTT, Modbus TCP...) open sockets before any
     * network interface is started, and a board with no Ethernet only
     * starts Wi-Fi later, if at all. Wi-Fi and Ethernet accept both
     * already existing. */
    ESP_ERROR_CHECK(esp_netif_init());
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        ESP_ERROR_CHECK(err);

    td_log_set_lock(log_lock, log_unlock, NULL);
    s_prev_vprintf = esp_log_set_vprintf(log_hook);
    fill_sysinfo();

    tdsh_espidf_config_t cfg = TDSH_ESP_IDF_CONFIG_DEFAULT();
    cfg.hostname = board_hostname();
    cfg.board_config = s_board_builtin;   /* board.conf (or the example) built in */
    cfg.default_user = "root";
    err = tdsh_espidf_init(&cfg);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "TinyDesk Shell init failed: %s (Terminal will not work)", esp_err_to_name(err));
    else
    {
        td_proto_register_shell_commands();   /* mqtt, modbus */
        td_proto_set_break_check(tdsh_bridge_break_requested);   /* Ctrl+C stops modbus read -i */
        if (esp_ota_get_next_update_partition(NULL))
            ota_esp_register_command();   /* ota (root only) */
    }
    td_mb_set_serial(board_rtu_lines());        /* Modbus RTU lines, if the board has any */

    xTaskCreate(ui_task, "tinydesk", UI_TASK_STACK, NULL, UI_TASK_PRIORITY, NULL);

    /* A new firmware runs on trial: confirm it once it has run for 30 s,
     * otherwise a crash or a reset makes the bootloader go back. */
    static esp_timer_handle_t s_ok_timer;
    const esp_timer_create_args_t ok = {.callback = confirm_firmware, .name = "ota_ok"};
    if (esp_timer_create(&ok, &s_ok_timer) == ESP_OK)
        esp_timer_start_once(s_ok_timer, 30 * 1000000LL);
    telnet_start();   /* remote desktop on port 23 (TinyDesk Shell login) */
}
