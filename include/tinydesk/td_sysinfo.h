/*
 * td_sysinfo.h - optional platform information and services supplied by the
 * port. Apps use these so they stay portable; any NULL member shows "n/a".
 */
#ifndef TD_SYSINFO_H
#define TD_SYSINFO_H

#include <stdbool.h>
#include <stdint.h>

#include "td_screen.h"
#define TD_FS_READ   0
#define TD_FS_WRITE  1
#define TD_FS_LIST   2
#define TD_FS_EXISTS 3
#define TD_FS_REMOVE 4

    /* Minimal filesystem access for the Files app. */
typedef struct
{
    const char *root;   /* starting directory, e.g. "/fs" */

        /* Call fn once per entry of dir (not "." or ".."). Returns the number of
     * entries or -1 on error. */
    int (*list)(const char *dir,
                void (*fn)(const char *name, bool is_dir, uint32_t size, void *user),
                void *user);

        /* Read up to cap bytes from the start of a file. Returns bytes read or -1. */
    int (*read)(const char *path, char *buf, int cap);

        /* Delete a file, or a directory with everything in it. Returns 0 on
     * success. */
    int (*remove)(const char *path);

        /* Create a directory. Returns 0 on success. */
    int (*mkdir)(const char *path);

        /* Create or replace a file with len bytes of data. Returns 0 on success. */
    int (*write)(const char *path, const char *data, int len);

        /* Rename or move a file or directory. Returns 0 on success. */
    int (*rename)(const char *from, const char *to);

        /* Returns 1 if the path exists (0 otherwise). */
    int (*exists)(const char *path);

        /* Optional: read up to cap bytes starting offset bytes into a file, for
     * files too large to read at once (the Image Viewer). Returns the bytes
     * read (0 at the end of the file) or -1. */
    int (*read_at)(const char *path, uint32_t offset, char *buf, int cap);

        /* Optional: add len bytes to the end of a file (made if missing), for
     * copying files too large to hold in memory. Returns 0 on success. */
    int (*append)(const char *path, const char *data, int len);
} td_fs_ops_t;

    /* ------------------------------------------------------------ network */

typedef struct
{
    bool wifi_up;              /* associated and has an IP address */
    char ssid[33];
    int rssi;                  /* dBm */
    char wifi_ip[16];
    bool eth_present;          /* Ethernet hardware enabled */
    bool eth_up;
    char eth_ip[16];
    bool busy;                 /* a scan / connect / disconnect is running */
    char message[64];          /* outcome of the last operation */
} td_net_status_t;

enum
{
    TD_SERVER_SSH = 0,
    TD_SERVER_FTP = 1
};

typedef struct
{
    char ssid[33];
    int rssi;
    bool secure;               /* needs a password */
    bool saved;                /* credentials are stored */
} td_wifi_ap_t;

    /* Network control for the Network app and the taskbar icon. Everything
 * returns at once; slow work runs in the background and its outcome shows
 * up in status(). */
typedef struct
{
    void (*status)(td_net_status_t *out);
    bool (*scan)(void);
      /* Results of the last scan; -1 while a scan is still running. */
    int (*scan_results)(td_wifi_ap_t *out, int max);
      /* Connect (password NULL: use the saved one). The network is saved. */
    bool (*connect)(const char *ssid, const char *password);
    bool (*disconnect)(void);
      /* Forget a saved network; false (with a status message) when the
     * current user may not. */
    bool (*forget)(const char *ssid);

        /* Servers (optional): which = TD_SERVER_SSH or TD_SERVER_FTP.
     * server_status returns true while running; server_set works in the
     * background and reports in status().message. */
    bool (*server_status)(int which, int *port, int *clients);
    bool (*server_set)(int which, bool on);

        /* Remote desktop over Telnet (optional). */
    bool (*telnet_enabled)(void);
    void (*telnet_enable)(bool on);
      /* Address of the connected Telnet client, or NULL. */
    const char *(*telnet_peer)(void);
} td_net_ops_t;

    /* ------------------------------------------------------------- displays */

    /* Where a display's terminal is (Settings -> Displays). Display 1 is the
 * HAL's terminal: the board's own link, or a Telnet client while one is
 * logged in. Display 2 (td_screen2_attach) is off, a Telnet client or a
 * terminal on a second serial port. Two Telnet displays take two
 * clients at once. */
enum
{
    TD_DISPLAY_OFF = 0,
    TD_DISPLAY_BOARD = 1,    /* the board's own link (USB serial, UART, LCD) */
    TD_DISPLAY_TELNET = 2,
    TD_DISPLAY_SERIAL = 3,   /* a second serial port (a USB-serial adapter on spare pins) */
    TD_DISPLAY_SOURCES = 4
};

typedef struct
{
      /* Can display (0 or 1) use src? (Not: no such port or pins.) */
    bool (*source_available)(int display, int src);
    int (*source)(int display);
    void (*set_source)(int display, int src);   /* saved; applies at once */
      /* A label for src ("USB serial", "LCD", "Serial 2: UART1, TX 17 / RX 18"). */
    const char *(*source_label)(int src);
      /* What display shows now ("192.168.1.5 (root)", "waiting for a
     * client", "no terminal answers"). */
    void (*status)(int display, char *buf, int cap);
      /* Keep the layout (td_screens_set_layout) for the next start. */
    void (*save_layout)(const td_screens_layout_t *layout);
      /* Orientation in degrees (0, 90, 180, 270), or -1 when the display's
     * terminal decides it (a PuTTY window). set_rotation saves it; it
     * applies at the next start. Optional. */
    int (*rotation)(int display);
    bool (*set_rotation)(int display, int degrees);
} td_display_ops_t;

    /* ---------------------------------------------------------------- mouse */

    /* Pointer settings of a mouse the board reads itself, in percent. */
typedef struct
{
    int speed;       /* 100: 8 mouse counts a column, 16 a row */
    int accel;       /* 0: none; 100: fast moves go up to 4 times as far */
    int precision;   /* 0: none; 100: slow moves go 0.4 times as far */
} td_mouse_settings_t;

typedef struct
{
      /* What is attached, e.g. "USB mouse 04ca:00bd, with its wheel"; false
     * when nothing is. */
    bool (*status)(char *buf, int cap);
    void (*get)(td_mouse_settings_t *s);
      /* Apply at once and save; false when saving failed. */
    bool (*set)(const td_mouse_settings_t *s);
} td_mouse_ops_t;

    /* -------------------------------------------------------------- keyboard */

    /* Settings of a keyboard the board reads itself. */
typedef struct
{
    int layout;      /* index of layout_name() */
    int delay_ms;    /* before a held key repeats */
    int rate;        /* repeats a second */
} td_keyboard_settings_t;

typedef struct
{
      /* What is attached, e.g. "USB keyboard"; false when nothing is. */
    bool (*status)(char *buf, int cap);
    void (*get)(td_keyboard_settings_t *s);
      /* Apply at once and save; false when saving failed. */
    bool (*set)(const td_keyboard_settings_t *s);
      /* The layouts' names ("US (English)", "German"...); NULL past the last. */
    const char *(*layout_name)(int i);
} td_keyboard_ops_t;

    /* ------------------------------------------------------ software update */

enum
{
    TD_OTA_IDLE,
    TD_OTA_CHECKING,
    TD_OTA_INSTALLING,
    TD_OTA_DONE,
    TD_OTA_FAILED
};

typedef struct
{
    char version[32];          /* running firmware, e.g. "0.1.0" */
    char built[32];            /* "Sep 24 2026 10:12:03" */
    char sdk[32];              /* "v5.3.1" */
    char running[17];          /* slot names, e.g. "ota_0" */
    char next[17];             /* where an update goes */
    bool on_trial;             /* new version not confirmed yet: a crash rolls back */
    bool can_roll_back;        /* the other slot holds a working version */
    char other_version[32];    /* its version */
} td_ota_info_t;

typedef struct
{
    int state;                 /* TD_OTA_* */
    int percent;               /* -1 while the size is unknown */
    uint32_t done, total;      /* bytes */
    uint32_t bytes_per_s;
    char new_version[32];      /* of the image being checked / installed */
    char new_built[32];
    char message[96];
} td_ota_status_t;

    /* The newest official release, as the port's update feed describes it. */
typedef struct
{
    bool valid;                /* a check succeeded */
    bool newer;                /* and it is newer than the installed version */
    char version[24];
    char date[12];             /* "2026-10-02" */
    char url[200];             /* its app image: give it to start() */
    uint32_t size;             /* bytes, 0 unknown */
    char notes[128];           /* the release page */
    char error[96];            /* why the last check failed, "" when it did not */
    uint32_t checks;           /* finished checks so far (to see a new result) */
} td_ota_release_t;

    /* Firmware updates (optional). Sources are an http(s):// URL or a real
 * file path; the work happens in the background. */
typedef struct
{
    void (*info)(td_ota_info_t *out);
    bool (*start)(const char *source, bool check_only);
    void (*status)(td_ota_status_t *out);
    void (*cancel)(void);
    void (*restart)(void);
    bool (*roll_back)(void);   /* boot the other slot's version */

        /* Official releases (optional, NULL: none). check_official looks up the
     * newest one in the background; quiet keeps status() as it is (a check
     * of its own, not one the user asked for). The result: official(). */
    bool (*check_official)(bool quiet);
    void (*official)(td_ota_release_t *out);
      /* "Check daily and notify me", kept by the port (default on). */
    bool (*auto_check)(void);
    void (*set_auto_check)(bool on);
      /* The version the user was last told about, so each is told once. */
    void (*notified)(char *out, int cap);
    void (*set_notified)(const char *version);

        /* Board settings (pins) that only the running firmware has, built in
     * from its board.conf (optional, NULL: none). Firmware built without
     * them, such as an official release, would start without them, so
     * Software Update offers to save them before it installs.
     * unsaved_settings counts them; save_settings writes them to the device
     * (false on failure) and describes the outcome in msg. */
    int (*unsaved_settings)(void);
    bool (*save_settings)(char *msg, int cap);
} td_ota_ops_t;

    /* ------------------------------------------------------------ downloads */

enum
{
    TD_FETCH_IDLE,
    TD_FETCH_RUNNING,
    TD_FETCH_DONE,
    TD_FETCH_FAILED
};

typedef struct
{
    int state;                 /* TD_FETCH_* */
    uint32_t done, total;      /* bytes; total 0 while unknown */
    char message[96];          /* why it failed */
} td_fetch_status_t;

    /* Downloads (optional; the Store uses them). start() copies source, an
 * http(s):// URL or a real file path (also file://), into the real file
 * path dest, replacing it, in the background; one at a time. HTTPS is
 * checked against the port's trusted certificates and redirects are
 * followed. A download larger than max bytes (0: no limit) fails. A failed
 * or cancelled download removes dest. */
typedef struct
{
    bool (*start)(const char *source, const char *dest, uint32_t max);
    void (*status)(td_fetch_status_t *out);
    void (*cancel)(void);
} td_fetch_ops_t;

    /* One system task (thread), for the Task Manager. */
typedef struct
{
    char name[16];
    char state;                /* 'R' running, 'r' ready, 'B' blocked, 'S' suspended, 'D' deleted */
    uint8_t priority;
    int8_t core;               /* pinned to this core, -1 any */
    uint32_t stack_free;       /* bytes never used; UINT32_MAX if not sampled */
    int16_t cpu_tenths;        /* share of all cores since the previous call, in 0.1 %; -1 unknown */
} td_task_info_t;

    /* A file a port puts on every new Desktop (td_sysinfo_t.desktop_files). */
typedef struct
{
    const char *name;          /* "Credits.txt"; NULL ends the list */
    const char *text;          /* the file's contents */
} td_desktop_file_t;

#define TD_USER_ROLE_USER  0
#define TD_USER_ROLE_ADMIN 1
#define TD_USER_ROLE_ROOT  2
#define TD_RIGHT_USERS     (1u << 0)
#define TD_RIGHT_SYSTEM    (1u << 1)
#define TD_RIGHT_NETWORK   (1u << 2)
#define TD_RIGHT_HARDWARE  (1u << 3)
#define TD_RIGHT_SHARED    (1u << 4)
#define TD_RIGHT_TIME      (1u << 5)
#define TD_RIGHT_APPS      (1u << 6)
#define TD_RIGHT_ALL       127u
typedef struct
{
    char name[32];
    int role;
    uint32_t rights;
} td_user_info_t;
typedef struct
{
    int (*list)(int index, td_user_info_t *out);
    int (*role)(const char *username);
    uint32_t (*rights)(const char *username);
    uint32_t (*generation)(const char *username);
    int (*create)(const char *actor, const char *name, const char *password, int role, uint32_t rights);
    int (*access_set)(const char *actor, const char *name, int role, uint32_t rights);
    int (*password_set)(const char *actor, const char *name, const char *old_password, const char *password);
    uint32_t (*remembered)(char *name, int cap);
    bool (*remember_allowed)(const char *name);
    uint32_t (*login)(const char *name, const char *password);
    int (*boot_user_set)(const char *actor, const char *name);
    int (*require_password)(const char *actor);
      /* Optional password hints, shown at Sign in to anyone at the device:
     * get fills out ("" none); set is allowed for the account itself and
     * whoever may manage it, "" removes it. 0 or a negative errno. */
    int (*hint_get)(const char *name, char *out, int cap);
    int (*hint_set)(const char *actor, const char *name, const char *hint);
} td_user_ops_t;
#define TD_HINT_MAX 64   /* bytes, with the ending 0 */
typedef struct
{
    const char *platform;     /* "ESP32-C6", "Windows host", ... */
    const char *chip;          /* e.g. "ESP32-C6 rev 0.1, 1 core" */
    const char *sdk_version;   /* e.g. ESP-IDF version */

        /* Heap in bytes. With external RAM (psram_total non-zero) these cover
     * the internal RAM only: it is the scarce part (task stacks, DMA, the
     * SSH server's start check), and the two are shown separately. */
    uint32_t (*free_heap)(void);
    uint32_t (*min_free_heap)(void);
    uint32_t (*total_heap)(void);
      /* External RAM (PSRAM) on the heap (optional; 0 or NULL: none). */
    uint32_t (*psram_free)(void);
    uint32_t (*psram_total)(void);
    int (*task_count)(void);
    int (*cpu_mhz)(void);
      /* Optional: the chip's own temperature (its die, not the room) in
     * tenths of a degree Celsius; false when it cannot be read. */
    bool (*chip_temp)(int *tenths_c);
      /* Optional: fill up to max tasks, return how many, or -1 when they
     * cannot be listed now (out of memory). CPU shares are measured between
     * calls (the first call has none). Tasks whose name starts with "IDLE"
     * are the idle time. */
    int (*tasks)(td_task_info_t *out, int max);

        /* The system clock (optional): seconds since 1970-01-01 UTC; false
     * while the clock is not set. */
    bool (*time_now)(int64_t *utc);
      /* Set the system clock (optional; the Date & time window allows it for
     * root only). */
    bool (*time_set)(int64_t utc);
      /* Automatic time from the network (optional): on/off (kept across
     * reboots) and "sync now", which works in the background. */
    bool (*time_auto_get)(void);
    bool (*time_auto_set)(bool on);
    bool (*time_sync)(void);

        /* The current desktop user's time zone as an offset from UTC in seconds
     * (optional). */
    bool (*get_tz)(long *seconds);
    bool (*set_tz)(long seconds);

    const td_net_ops_t *net;
    const td_display_ops_t *displays;   /* Settings -> Displays (optional) */
    const td_ota_ops_t *ota;   /* NULL: no updates on this platform */
      /* Downloads (optional, NULL: none) and the Store's catalog, an
     * http(s):// URL or a real file path of a catalog.txt (optional, NULL:
     * the official one). */
    const td_fetch_ops_t *fetch;
    const char *store_url;
      /* With ota NULL (optional): why, and how to update instead; lines
     * separated by '\n'. Software Update shows it. */
    const char *no_ota_text;

        /* User accounts (optional; without them the desktop is root's). */
    bool (*user_exists)(const char *user);
    bool (*authenticate)(const char *user, const char *password);
    const td_user_ops_t *users;

        /* Persist the Settings app blob (NVS on ESP32, a file on the host). */
    bool (*settings_load)(void *data, int len);
    bool (*settings_save)(const void *data, int len);

    const td_fs_ops_t *fs;

        /* Optional extra "About" line, e.g. the embedded shell version. */
    const char *extra;

        /* Optional files written next to Welcome.txt when a user's Desktop
     * folder is first created, e.g. a port's Credits.txt. An array ended by
     * an entry with name NULL; names are plain file names (no '/' or '\'),
     * others are skipped. */
    const td_desktop_file_t *desktop_files;

        /* Optional: a mouse and a keyboard the board reads itself (USB on the
     * ESP32-S3), for the Mouse and Keyboard apps. NULL: none, and no app. */
    const td_mouse_ops_t *mouse;
    const td_keyboard_ops_t *keyboard;

        /* Optional: kiosk mode. A Python script (a shell path, such as
     * "/root/hmi.py") the desktop runs full screen at start, with no
     * taskbar, Start menu or desktop (apps/kiosk.c). NULL or "": off. */
    const char *kiosk_app;
      /* Optional startup artwork; app-independent and never compiled into firmware. */
    const char *kiosk_splash;
    bool kiosk_hide_status;
} td_sysinfo_t;

    /* Install / read the port's sysinfo (never NULL; unset members are NULL). */
void td_set_sysinfo(const td_sysinfo_t *info);
const td_sysinfo_t *td_sysinfo(void);

#endif /* TD_SYSINFO_H */
