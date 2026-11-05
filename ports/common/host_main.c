/*
 * host_main.c - the desktop build: tinydesk plus all demo apps, with TinyDesk Shell
 * running in the Terminal app. Files live in ./tinydesk_fs (shared by the
 * shell and the Files app); settings in ./tinydesk_settings.bin.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "td_apps.h"
#include "host_fetch.h"
#include "td_fs_stdio.h"
#include "td_host_hal.h"

#ifdef TD_WITH_TDSH
#include "tdsh_bridge.h"
#ifdef TD_WITH_TDSH
#include "tdsh.h"   /* TDSH_VERSION */
#endif
#endif

#define SETTINGS_FILE "tinydesk_settings.bin"

static bool settings_load(void *data, int len)
{
    FILE *f = fopen(SETTINGS_FILE, "rb");
    if (!f)
        return false;
    bool ok = fread(data, 1, (size_t)len, f) == (size_t)len;
    fclose(f);
    return ok;
}

static bool settings_save(const void *data, int len)
{
    FILE *f = fopen(SETTINGS_FILE, "wb");
    if (!f)
        return false;
    bool ok = fwrite(data, 1, (size_t)len, f) == (size_t)len;
    return fclose(f) == 0 && ok;
}

/* The PC's clock. */
static bool time_now(int64_t *utc)
{
    *utc = (int64_t)time(NULL);
    return true;
}

/* The PC's own offset from UTC (the default time zone). */
static long pc_offset(void)
{
    time_t now = time(NULL);
    struct tm g = *gmtime(&now);
    g.tm_isdst = -1;
    return (long)difftime(now, mktime(&g));
}

/* Each user's time zone, in ~/.tdsh_tz like TinyDesk Shell's `tz` (seconds east of
 * UTC); without that file, the PC's. */
static char s_tz_user[64];
static long s_tz;
static uint32_t s_tz_read_ms;
static bool s_tz_valid;

static void tz_path(char *out, size_t cap)
{
    snprintf(out, cap, "%.400s/.tdsh_tz", td_session_home());
}

static bool get_tz(long *seconds)
{
    uint32_t now = td_millis();
    if (!s_tz_valid || strcmp(s_tz_user, td_session_user()) != 0 || now - s_tz_read_ms > 5000)
    {
        char path[420];
        tz_path(path, sizeof(path));
        snprintf(s_tz_user, sizeof(s_tz_user), "%s", td_session_user());
        s_tz = pc_offset();
        FILE *f = fopen(path, "r");
        if (f)
        {
            if (fscanf(f, "%ld", &s_tz) != 1)
                s_tz = pc_offset();
            fclose(f);
        }
        s_tz_read_ms = now;
        s_tz_valid = true;
    }
    *seconds = s_tz;
    return true;
}

static bool set_tz(long seconds)
{
    char path[420];
    tz_path(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (!f)
        return false;
    fprintf(f, "%ld\n", seconds);
    s_tz_valid = false;
    return fclose(f) == 0;
}

static td_sysinfo_t s_info;

void td_host_use_net(const td_net_ops_t *net)
{
    s_info.net = net;
}

void td_host_use_users(bool (*exists)(const char *), bool (*auth)(const char *, const char *))
{
    s_info.user_exists = exists;
    s_info.authenticate = auth;
}

static void welcome_done(int button, void *user)
{
    (void)user;
    if (button == 0)
        td_app_launch("Terminal");
}

void td_host_setup(const td_hal_t *hal, const char *platform_name)
{
    static char fs_root[512];
    if (!getcwd(fs_root, sizeof(fs_root) - 16))
        snprintf(fs_root, sizeof(fs_root), ".");
    for (char *p = fs_root; *p; p++)
        if (*p == '\\')
            *p = '/';
    strcat(fs_root, "/tinydesk_fs");

    s_info.platform = platform_name;
    s_info.chip = "host CPU";
    s_info.sdk_version = "n/a";
    s_info.settings_load = settings_load;
    s_info.settings_save = settings_save;
    s_info.fs = td_fs_stdio(fs_root);
    td_fs_stdio_guard(td_session_path_allowed);
    s_info.time_now = time_now;
    s_info.get_tz = get_tz;
    s_info.set_tz = set_tz;
#ifdef TD_WITH_TDSH
    s_info.extra = "Shell:     TinyDesk Shell " TDSH_VERSION;
#endif
    s_info.fetch = host_fetch_ops();
    s_info.store_url = getenv("TINYDESK_STORE");   /* another catalog: a URL or a file path */
    s_info.kiosk_app = getenv("TINYDESK_KIOSK");   /* kiosk mode: a Python app's shell path */
    s_info.kiosk_splash = getenv("TINYDESK_KIOSK_SPLASH");
    s_info.kiosk_hide_status = getenv("TINYDESK_KIOSK_HIDE_STATUS") != NULL;
    td_set_sysinfo(&s_info);

    td_init(hal);
    td_logf('I', "TinyDesk %s on %s", TD_VERSION, platform_name);
    td_logf('I', "terminal size %dx%d", td_stats()->cols, td_stats()->rows);

    td_apps_register_all();
#ifdef TD_WITH_TDSH
    td_terminal_set_backend(td_tdsh_host_backend(fs_root, "tinydesk"));
    td_logf('I', "TinyDesk Shell sandbox: %s", fs_root);
#endif
    if (!td_kiosk_active())
        td_msgbox("Welcome to TinyDesk",
                  "Press F10 or click [Start] for apps.\nF6 switches windows, F11 is full screen.\nOpen the Terminal now?",
                  "Terminal|Later", welcome_done, NULL);
}

int td_host_main(const char *platform_name)
{
    const td_hal_t *hal = td_host_hal_open();
    if (!hal)
    {
        fprintf(stderr, "TinyDesk needs an interactive terminal (Windows Terminal, xterm, ...).\n");
        return 1;
    }
    td_host_setup(hal, platform_name);
    td_run();
    td_shutdown();
    td_host_hal_close();
    return 0;
}
