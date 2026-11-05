/*
 * tdsh_bridge_esp.c - runs the TinyDesk Shell console in its own FreeRTOS task
 * and connects it to the tinydesk Terminal app.
 *
 *   tinydesk task                        "tdsh" task
 *   keys  --write()--> [in  stream buffer] --> stdin  (blocking reads)
 *   vterm <--read()--- [out stream buffer] <-- stdout / stderr / ESP_LOG
 *
 * The shell task's stdin/stdout/stderr are a funopen() stream over the two
 * buffers, so every printf in TinyDesk Shell (and the ESP_LOGx calls it makes) lands
 * in the window. The UI side never blocks.
 */
#include "tdsh_bridge_esp.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/stream_buffer.h"
#include "freertos/task.h"
#include "tdsh_espidf.h"

#define IN_BUFFER      256
#define OUT_BUFFER     2048
#define SHELL_PRIORITY 4        /* below the UI task, so typing stays smooth */

static const char *TAG = "tdsh_bridge";

static StreamBufferHandle_t s_in;
static StreamBufferHandle_t s_out;
static SemaphoreHandle_t s_out_lock;   /* several tasks may print */
static bool s_started;

/* Keys a long-running command's Ctrl+C check took from s_in that were not
 * Ctrl+C; the shell reads them next. Only the shell task touches these. */
static char s_typed[64];              /* typing during a repeat is rare */
static size_t s_typed_len;
static void flush_input(void)
{
    s_typed_len = 0;
    char discard[64];
    while (xStreamBufferReceive(s_in, discard, sizeof(discard), 0) > 0)
    {
    }
}

/* ---------------------------------------------- shell-side stream */

static int stream_read(void *cookie, char *buf, int len)
{
    (void)cookie;
    if (s_typed_len > 0)
    {
        size_t n = s_typed_len < (size_t)len ? s_typed_len : (size_t)len;
        memcpy(buf, s_typed, n);
        memmove(s_typed, s_typed + n, s_typed_len - n);
        s_typed_len -= n;
        return (int)n;
    }
    /* Block until the user types something. */
    size_t n;
    do
    {
        n = xStreamBufferReceive(s_in, buf, (size_t)len, portMAX_DELAY);
    } while (n == 0);
    return (int)n;
}

static int stream_write(void *cookie, const char *buf, int len)
{
    (void)cookie;
    xSemaphoreTake(s_out_lock, portMAX_DELAY);
    int done = 0;
    while (done < len)
    {
        /* Blocks while the window is behind; the UI drains it every tick. */
        done += (int)xStreamBufferSend(s_out, buf + done, (size_t)(len - done), portMAX_DELAY);
    }
    xSemaphoreGive(s_out_lock);
    return len;
}

static void shell_task(void *arg)
{
    (void)arg;
    FILE *io = funopen(NULL, stream_read, stream_write, NULL, NULL);
    if (!io)
    {
        ESP_LOGE(TAG, "funopen failed");
        vTaskDelete(NULL);
        return;
    }
    setvbuf(io, NULL, _IONBF, 0);
    stdin = io;
    stdout = io;
    stderr = io;
    tdsh_espidf_run_console();   /* never returns */
}

bool tdsh_bridge_break_requested(void)
{
    /* Only for a command on the desktop's console: an SSH session's
     * command must not eat the Terminal window's keys. */
    if (!s_started || !tdsh_is_local_console_task())
        return false;
    bool brk = false;
    char tmp[32];
    size_t n;
    while ((n = xStreamBufferReceive(s_in, tmp, sizeof(tmp), 0)) > 0)
    {
        for (size_t i = 0; i < n; i++)
        {
            if (tmp[i] == 0x03)
            {
                brk = true;
                s_typed_len = 0;              /* like a terminal: drop the type-ahead */
            }
            else if (s_typed_len < sizeof(s_typed))
            {
                s_typed[s_typed_len++] = tmp[i];
            }
        }
    }
    return brk;
}

/* ------------------------------------------------------- backend */

/* The Terminal window's width, for the shell's line editor (set by the UI
 * task from start() and resize(), read by the shell task). */
static volatile int s_cols;

static int console_columns(void)
{
    return s_cols;
}

static void backend_resize(void *ctx, int cols, int rows)
{
    (void)ctx;
    (void)rows;
    s_cols = cols;
}

static int backend_start(void *ctx, int cols, int rows)
{
    (void)ctx;
    (void)rows;
    s_cols = cols;
    if (s_started)
        return 0;
    tdsh_espidf_set_console_columns(console_columns);
    tdsh_espidf_set_console_flush(flush_input);
    s_in = xStreamBufferCreate(IN_BUFFER, 1);
    s_out = xStreamBufferCreate(OUT_BUFFER, 1);
    s_out_lock = xSemaphoreCreateMutex();
    if (!s_in || !s_out || !s_out_lock)
        return -1;
    if (xTaskCreate(shell_task, "tdsh", TDSH_SHELL_TASK_STACK, NULL, SHELL_PRIORITY, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "cannot create the shell task");
        return -1;
    }
    s_started = true;
    return 0;
}

static int backend_read(void *ctx, uint8_t *buf, int cap)
{
    (void)ctx;
    return s_started ? (int)xStreamBufferReceive(s_out, buf, (size_t)cap, 0) : 0;
}

static int backend_write(void *ctx, const uint8_t *buf, int len)
{
    (void)ctx;
    return s_started ? (int)xStreamBufferSend(s_in, buf, (size_t)len, 0) : 0;
}

/* The console session's user (boot user, or whoever logged in). */
static const char *backend_user(void *ctx)
{
    (void)ctx;
    return tdsh_espidf_console_user();
}

/* Continue the console as another user; Ctrl+C wakes the line editor so
 * the switch happens at once. */
static void backend_set_user(void *ctx, const char *user)
{
    (void)ctx;
    tdsh_espidf_console_set_user(user);
    if (s_started)
    {
        const uint8_t ctrl_c = 0x03;
        xStreamBufferSend(s_in, &ctrl_c, 1, 0);
    }
}

static const td_term_backend_t s_backend = {
    .name = "tdsh",
    .start = backend_start,
    .read = backend_read,
    .write = backend_write,
    .resize = backend_resize,
    .user = backend_user,
    .set_user = backend_set_user,
};

const td_term_backend_t *tdsh_bridge_esp_backend(void)
{
    return &s_backend;
}
