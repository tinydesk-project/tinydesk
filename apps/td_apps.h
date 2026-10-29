/*
 * td_apps.h - the demo apps that ship with tinydesk. They are optional:
 * link the ones you want and register them before td_run().
 */
#ifndef TD_APPS_H
#define TD_APPS_H

#include <stdbool.h>
#include <stdint.h>

#include "tinydesk/td.h"

/* Register every built-in app (and apply saved settings). */
void td_apps_register_all(void);

/* Individual apps. */
void td_about_register(void);
void td_sysmon_register(void);
void td_taskmgr_register(void);
void td_logview_register(void);
void td_settings_register(void);
void td_files_register(void);
void td_counter_register(void);
void td_terminal_register(void);
void td_editor_register(void);
void td_network_register(void);

/* ------------------------------------------------- files & desktop */

/* Open a text file in the Editor (NULL: a new, unnamed file). */
void td_editor_open(const char *path);

/* Open the Files app in a directory. */
void td_files_open(const char *dir);

/* Something changed on disk: let an open Files window re-read its folder. */
void td_files_changed(void);

/* The current user's home directory and Desktop folder (real paths, e.g.
 * "/fs/home/bob/Desktop" on the ESP32). The Desktop folder's files are
 * shown as desktop icons. */
const char *td_home_dir(void);
const char *td_desktop_dir(void);

/* Create the current user's Desktop folder if needed and show it (called
 * again after every user switch). */
void td_desktop_folder_init(void);

/* Re-read the Desktop folder now (it is also re-read every few seconds,
 * so changes made from the shell appear on their own). */
void td_desktop_refresh(void);

/* Move the file or folder at `path` into folder `dir`. Returns NULL on
 * success (or when it is already there), otherwise a message to show. */
const char *td_move_into(const char *path, const char *dir);

/* The path as the shell sees it (the filesystem root removed), for example
 * "/fs/root/Desktop/a.txt" -> "/root/Desktop/a.txt". */
const char *td_shell_path(const char *path);

/* True for a usable file or folder name: not empty, no '/' or '\', not
 * "." or "..". */
bool td_valid_name(const char *name);

/* ------------------------------------------------------------ log */

/* Append log text. Level is 'E', 'W', 'I', 'D' or 'V' (0 = detect from an
 * ESP-IDF style "I (123) tag: ..." prefix). Text may contain several lines
 * or end in the middle of one; lines are split on '\n'. Safe to call from
 * any task once a lock has been installed with td_log_set_lock(). */
void td_log_append(char level, const char *text);

/* printf-style convenience wrapper around td_log_append(). */
void td_logf(char level, const char *fmt, ...);

/* Serialise access to the log buffer when several threads write to it. */
void td_log_set_lock(void (*lock)(void *ctx), void (*unlock)(void *ctx), void *ctx);

/* ------------------------------------------------------- settings */

/* Load the current user's settings (theme, ASCII mode, desktop pattern,
 * icons, clock format) and apply them. Each user's settings live in
 * ~/.tinydesk_settings; a user without one gets the device defaults. Called
 * at start-up and after every user switch. */
void td_settings_apply_saved(void);

/* Save the current user's settings; false if saving is not possible. */
bool td_settings_save(void);

/* Taskbar clock preferences of the current user. */
enum
{
    TD_DATE_DMY = 0,
    TD_DATE_YMD = 1,
    TD_DATE_MDY = 2,
    TD_DATE_FORMATS = 3
};
typedef struct
{
    uint8_t clock_12h;      /* 0: 24-hour */
    uint8_t date_format;    /* TD_DATE_* */
    uint8_t hide_clock;     /* 1: no date and time in the taskbar */
} td_clock_prefs_t;
td_clock_prefs_t *td_clock_prefs(void);

/* Editor preferences (saved with the other settings: td_settings_save()). */
typedef struct
{
    uint8_t completion;     /* 1: suggestions pop up while typing Python */
} td_editor_prefs_t;
td_editor_prefs_t *td_editor_prefs(void);

/* ------------------------------------------------------ clipboard */

/* A text clipboard shared by the apps (the Editor's copy, cut and paste).
 * It is emptied when the desktop user changes. */
bool td_clipboard_set(const char *text, int len);
const char *td_clipboard_get(int *len);   /* NULL when empty */
void td_clipboard_clear(void);

/* ---------------------------------------------------- date & time */

/* Open the Date & time window (also opened by clicking the clock). */
void td_datetime_open(void);

/* Put the clock (with its click menu) in the taskbar. */
void td_datetime_install_clock(void);

/* Time of day of a UTC moment (seconds since 1970) in the current user's
 * time zone and clock format; "" for 0 (clock not set). */
void td_time_of_day(int64_t utc, char *buf, int cap);

/* A path the desktop user typed ("~/x", "/home/bob/x", or relative to
 * their home) as a real path; false if it leaves their home (non-root) or
 * contains "..". */
bool td_session_real_path(const char *path, char *real, size_t cap);

/* Register the Software Update app. */
void td_update_register(void);

/* ---------------------------------------------------- MQTT & Modbus */

void td_mqtt_register(void);
void td_modbus_register(void);

/* Drive the MQTT and Modbus connections from the desktop's main loop. */
void td_proto_service_start(void);

/* ------------------------------------------------------- terminal */

/* A program the Terminal app talks to (for example an embedded shell running
 * in its own task). All functions are called from the UI loop and must not
 * block. */
typedef struct
{
    const char *name;                                   /* shown in the title */
    /* Start the session if it is not running yet. */
    int (*start)(void *ctx, int cols, int rows);
    /* Program output waiting to be shown; returns bytes copied (0 if none). */
    int (*read)(void *ctx, uint8_t *buf, int cap);
    /* Keyboard input for the program; returns bytes accepted. */
    int (*write)(void *ctx, const uint8_t *buf, int len);
    /* The window's text area changed size (optional). */
    void (*resize)(void *ctx, int cols, int rows);
    /* Multi-user shells (optional): the user the program runs as, and a
     * request to continue as another (existing) user. */
    const char *(*user)(void *ctx);
    void (*set_user)(void *ctx, const char *user);
    void *ctx;
} td_term_backend_t;

/* Install the backend used by the Terminal app and start the program (call
 * after td_init(), before td_run()). */
void td_terminal_set_backend(const td_term_backend_t *backend);

/* The installed backend (or NULL). */
const td_term_backend_t *td_terminal_backend(void);

/* Forget everything on the Terminal screen and in its history. */
void td_terminal_reset(void);

/* Open (or focus) the Terminal window and type `command` followed by
 * Enter. A line the user had started typing at the prompt is cleared first
 * (Ctrl+U). Returns false (and shows a message) when the build has no
 * shell. */
bool td_terminal_run(const char *command);

/* The same session shown somewhere else (the Editor's output pane). */

/* Type `command` and Enter (after Ctrl+U) without opening the Terminal. */
bool td_terminal_command(const char *command);

/* Draw the session into the cells (x, y, w, h) of `win`'s client area.
 * While the Terminal window is closed the session takes that size;
 * otherwise the rows around the cursor are shown. scroll_back > 0 shows
 * older lines (clamped; the value used is returned). */
int td_terminal_draw_view(td_window_t *win, int x, int y, int w, int h, int scroll_back, bool cursor);

/* Send a key (as the Terminal window would) or text to the program. */
void td_terminal_key(const td_event_t *ev);
void td_terminal_send(const char *text, int len);

/* `win` is redrawn whenever the session prints, and `tap` (optional) sees
 * every byte of output; NULL stops both. One viewer at a time. */
void td_terminal_set_viewer(td_window_t *win, void (*tap)(const uint8_t *data, int len));

/* Read pending output now (the viewer's own tick, for a quick display). */
void td_terminal_pump(void);

/* Shell scripts: files ending in TD_SCRIPT_EXT. */
#define TD_SCRIPT_EXT ".tdsh"
bool td_is_script(const char *name);

/* Python scripts: files ending in .py. */
bool td_is_python(const char *name);

/* Files with a Run command: shell and Python scripts. */
bool td_is_runnable(const char *name);

/* Run the script at `path` (a real path, as the Files app and the Desktop
 * use) in the Terminal: `tdsh run "<shell path>"`, or `py "<shell path>"`
 * for a .py file. */
bool td_script_run(const char *path);

/* A colour for script names and icons that reads well on background `bg`
 * (a 256-colour palette index): green, bright on dark backgrounds. */
uint8_t td_script_colour(uint8_t bg);

/* ------------------------------------------------ windows for Python */

/* The tdui module puts windows on the desktop for a Python script. The
 * script runs on the shell's task and never touches the window manager: it
 * sends requests through a queue, the desktop carries them out every 10 ms
 * on its own loop, and sends back what the user did through another queue.
 * Each message is a td_pyui_msg_t followed by `len` bytes of text. */

/* Requests (script -> desktop). */
enum
{
    TD_PYUI_OPEN = 1,     /* window id: a, b client size, c, d position (-1: centred);
                           * flags 1 resizable; text the title */
    TD_PYUI_CLOSE,        /* window id */
    TD_PYUI_ADD,          /* widget id in window win; flags TD_PYUI_W_*; a, b position,
                           * c, d size, e initial value; text */
    TD_PYUI_TEXT,         /* id: new text (window: its title) */
    TD_PYUI_VALUE,        /* id: a is the checkbox state, progress percent or list selection */
    TD_PYUI_VISIBLE,      /* id: a 0 or 1 */
    TD_PYUI_COLOR,        /* id: a fg, b bg (palette index, -1 the theme's) */
    TD_PYUI_FOCUS,        /* window or widget id */
    TD_PYUI_LIST_CLEAR,   /* list id */
    TD_PYUI_LIST_ADD,     /* list id: text is items separated by '\n' */
    TD_PYUI_PUT,          /* canvas id: text at a, b in colours c, d */
    TD_PYUI_FILL,         /* canvas id: rectangle a, b, c, d of the first character
                           * of text (' ' if none) in colours e, f */
    TD_PYUI_MSGBOX,       /* request id: text is "title\0message\0buttons" */
    TD_PYUI_INPUT,        /* request id: text is "title\0prompt\0initial"; flags 1 secret */
    TD_PYUI_MENU,         /* request id: text is the items separated by '\0' */
    TD_PYUI_RESIZE,       /* window id: a, b client size; canvas id: a, b size */
    TD_PYUI_MOVE,         /* window id: a, b position on the screen; canvas id: in the window */
    TD_PYUI_SHOW,         /* window id: a is TD_PYUI_SHOW_* */
};

/* TD_PYUI_SHOW states. */
enum
{
    TD_PYUI_SHOW_RESTORE,     /* from maximised, full screen or minimised */
    TD_PYUI_SHOW_MAXIMIZE,
    TD_PYUI_SHOW_MINIMIZE,
    TD_PYUI_SHOW_FULLSCREEN,
};

/* Widget kinds (TD_PYUI_ADD flags). */
enum
{
    TD_PYUI_W_LABEL = 1,  /* c width (0: to the edge) */
    TD_PYUI_W_BUTTON,
    TD_PYUI_W_CHECKBOX,   /* e checked */
    TD_PYUI_W_TEXTBOX,    /* c width; e 1 secret */
    TD_PYUI_W_PROGRESS,   /* c width; e percent */
    TD_PYUI_W_LIST,       /* c, d size, the scrollbar included (<= 0: to the edge) */
    TD_PYUI_W_CANVAS,     /* c, d size */
};

/* Events (desktop -> script). */
enum
{
    TD_PYUI_EV_CLICK = 1, /* button pressed, Enter in a text box or list (a: item) */
    TD_PYUI_EV_CHANGE,    /* checkbox a, list selection a, text box text */
    TD_PYUI_EV_CLOSE,     /* window closed by the user or the desktop */
    TD_PYUI_EV_KEY,       /* window id: a, b low and high 16 bits of the key, c mods */
    TD_PYUI_EV_MOUSE,     /* canvas id: a, b cell, c button, d action */
    TD_PYUI_EV_RESULT,    /* request id: a button or item (-1 cancelled), text typed */
    TD_PYUI_EV_ERROR,     /* id: text says what could not be done */
    TD_PYUI_EV_RESIZE,    /* window id: a, b its new client size */
};

typedef struct
{
    uint8_t op;
    uint8_t flags;
    uint16_t len;   /* bytes of text after the header */
    uint16_t id;    /* the object, numbered by the script */
    uint16_t win;   /* TD_PYUI_ADD: the widget's window */
    int16_t a, b, c, d, e, f;
} td_pyui_msg_t;

#define TD_PYUI_TEXT_MAX   1024   /* text in one message */
#define TD_PYUI_MAX_CELLS  2000   /* all canvases of a script together */

/* The desktop side: start serving scripts (td_apps_register_all does). */
void td_pyui_start(void);

/* The script side, from the script's task. attach() is false when the
 * desktop is not running or memory is short. send() is false while the
 * queue is full (try again after a short sleep); recv() returns 1 and the
 * message, or 0 if there is none; text longer than cap is cut. A script
 * that ends calls detach_begin() (its windows close) and then frees the
 * queues with detach_done(), which is false until the desktop has let go. */
bool td_pyui_attach(void);
bool td_pyui_send(const td_pyui_msg_t *m, const char *text);
int td_pyui_recv(td_pyui_msg_t *m, char *text, int cap);
void td_pyui_detach_begin(void);
bool td_pyui_detach_done(void);

/* ------------------------------------------------------- sessions */

/* The desktop belongs to one user at a time, mirroring TinyDesk Shell: root sees
 * the whole filesystem and has its home in /root; any other user has
 * /home/<user> and cannot leave it. Their Desktop folder, the Files app and
 * the Terminal's shell all follow the current user. */

/* Start the session for the shell's current user (or root). */
void td_session_init(void);

/* Switch the desktop to another user: their windows are closed, the
 * Terminal screen is cleared and (if switch_shell) the shell continues as
 * that user. The user must exist; authentication is the caller's job. */
void td_session_switch(const char *user, bool switch_shell);

const char *td_session_user(void);
bool td_session_is_root(void);
const char *td_session_home(void);   /* real path, e.g. "/fs/home/bob" */
const char *td_session_jail(void);   /* the Files app cannot go above this */

/* Forget where the Files app was (the next window starts at home). */
void td_files_reset(void);

#endif /* TD_APPS_H */
