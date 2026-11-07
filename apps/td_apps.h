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
void td_pixels_register(void);
void td_imageview_register(void);
void td_trash_register(void);      /* the Recycle Bin, and its hourly clean-up */
void td_mouse_register(void);      /* only where td_sysinfo()->mouse is set */
void td_keyboard_register(void);   /* only where td_sysinfo()->keyboard is set */
void td_terminal_register(void);
void td_editor_register(void);
void td_network_register(void);
void td_displays_register(void);      /* Settings -> Displays */
void td_osk_register(void);        /* the On-screen keyboard app */
void td_store_register(void);      /* the Store: apps from a catalog */
void td_remote_register(void);     /* Remote Desktop: another TinyDesk over Telnet */
/* What a VT100/xterm sends for a key event (out: 16 bytes); 0: nothing. */
int td_terminal_encode_key(const td_event_t *ev, char *out);
/* The Store's parts, for tests: parse catalog.txt into an array of
 * td_store_entry_size() bytes per entry (returns the count, -1: not a
 * catalog), why the board cannot run one (NULL: it can), SHA-256 as hex. */
int td_store_parse(char *text, void *list, int max);
int td_store_entry_size(void);
const char *td_store_cannot_run(const void *entry);
void td_store_sha256(const void *data, uint32_t len, char out[65]);

/* ------------------------------------------------- on-screen keyboard */

/* Keys drawn into part of a window and sized to it (apps/osk.c): a grid
 * 12 units wide and 5 rows high, a unit 3 to 8 columns and a row 1 to 6
 * lines as the area allows. The owner calls td_osk_draw() from on_draw
 * and hands td_osk_mouse() its mouse events (client cells). A key types
 * when released on the key it was pressed on; the gaps between keys
 * belong to the keys, so a finger between two still lands on one. Shift
 * works as on a phone: one tap for the next character, two quick taps
 * to lock it (CAPS), one more to release it. */
enum
{
    TD_OSK_SHIFT_OFF,
    TD_OSK_SHIFT_ONCE,   /* the next character only */
    TD_OSK_SHIFT_LOCK    /* until Shift is tapped again */
};
typedef struct
{
    td_rect_t area;      /* client cells the keys use */
    const char *enter;   /* the Enter key's label ("Enter", "Sign in", ...) */
    bool password;       /* a Clear key instead of Esc */
    uint8_t shift;       /* TD_OSK_SHIFT_* */
    bool sym;            /* the symbols */
    uint32_t shift_ms;   /* when Shift was last tapped (double tap) */
    int held;            /* the key pressed and not released yet, or -1 */
} td_osk_t;

#define TD_OSK_NONE  0     /* nothing to type (also Shift and #+=) */
#define TD_OSK_HIDE  (-1)  /* the Hide key */
#define TD_OSK_CLEAR (-2)  /* the Clear key (password keyboards) */

void td_osk_init(td_osk_t *k, td_rect_t area, const char *enter, bool password);
void td_osk_draw(const td_osk_t *k);
/* What a released key types: a character, TD_KEY_BACKSPACE, TD_KEY_ENTER,
 * TD_KEY_LEFT, TD_KEY_RIGHT, TD_KEY_ESC, TD_OSK_HIDE, TD_OSK_CLEAR, or
 * TD_OSK_NONE. *used: the event was on the keys. */
int td_osk_mouse(td_osk_t *k, const td_event_t *ev, bool *used);

/* The On-screen keyboard app opens by itself while a text box has the
 * focus (the "Keyboard for text fields" setting). */
void td_osk_follow_focus(bool on);
/* Open or close the On-screen keyboard window (Sign in, the kiosk's way
 * out); its keys go to the focused window as key presses. */
void td_osk_show(bool on);
bool td_osk_shown(void);
bool td_osk_follows_focus(void);

/* ------------------------------------------------- files & desktop */

/* Open a text file in the Editor (NULL: a new, unnamed file). */
void td_editor_open(const char *path);

/* Image files: BMP (uncompressed: 1, 4, 8-bit palette, 16, 24, 32-bit,
 * also with bit-field masks) and PPM/PGM (binary P6/P5, 8 or 16 bits per
 * sample). Files whose name ends in .bmp, .ppm or .pgm (any case). */
bool td_is_image(const char *name);

/* Open an image file (a real path, as Files and the Desktop use) in the
 * Image Viewer; NULL opens the viewer without one. */
void td_imageview_open(const char *path);

/* Reading an image row by row, in the order the file stores the rows (BMP:
 * usually bottom to top), with about 4 KB of buffer plus one row: images
 * larger than the RAM can be shown scaled down. Needs fs->read_at. */
#define TD_IMAGE_MAX_W 4096
#define TD_IMAGE_MAX_H 32768

typedef struct
{
    int w, h;          /* pixels */
    char format[16];   /* "BMP 24-bit", "PPM", "PGM 16-bit", "PPM text"... */
    /* the rest is private */
    const td_fs_ops_t *fs;
    char path[TD_PATH_MAX];
    uint32_t data;     /* offset of the first stored row */
    uint32_t stride;   /* bytes per stored row (0 for plain PPM/PGM) */
    int kind, bpp, maxval;
    bool bottom_up;
    bool plain;        /* P3/P2: samples as decimal text */
    uint32_t pos;      /* plain: offset of the next sample */
    uint32_t mask[3];  /* 16/32-bit BMP: red, green, blue */
    uint8_t shift[3], bits[3];
    int next;          /* rows delivered */
    uint8_t *buf;      /* file bytes buf_off .. buf_off + buf_len */
    int buf_cap, buf_len;
    uint32_t buf_off;
    uint8_t palette[256][3];
} td_image_t;

/* Read the header. False with a reason in err for files that are not a
 * supported image, too large, or unreadable. */
bool td_image_load(td_image_t *img, const td_fs_ops_t *fs, const char *path, char *err, int errcap);

/* The next stored row: its row number y (0 = top) and w RGB triples in
 * rgb. False after the last row or on a read error. */
bool td_image_row(td_image_t *img, int *y, uint8_t *rgb);

/* Start again with the first stored row. */
void td_image_rewind(td_image_t *img);

/* Free the buffer (the struct is the caller's). */
void td_image_close(td_image_t *img);

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

/* ------------------------------------------------------ recycle bin */

/* apps/trash.c. Files and the Desktop move what is deleted into the
 * current user's bin, ~/.trash: each item in a folder of its own,
 * ~/.trash/<id>/<name>, with ~/.trash/<id>/.tdtrash (where it was, when,
 * how big). The bin holds at most the user's size limit, the oldest going
 * first, and can delete items after a time. */
#define TD_TRASH_SIZES 6   /* size codes: 0 1 MB (the default), 1 off, 2 256 KB,
                            * 3 4 MB, 4 16 MB, 5 64 MB */
#define TD_TRASH_KEEPS 5   /* keep codes: 0 for ever, then 1, 7, 30, 90 days */

/* The user's limits (saved with the other settings: td_settings_save()). */
typedef struct
{
    uint8_t size;   /* size code */
    uint8_t keep;   /* keep code */
} td_trash_prefs_t;
td_trash_prefs_t *td_trash_prefs(void);

uint32_t td_trash_limit(void);            /* bytes; 0: the bin is off */
const char *td_trash_size_name(int code); /* "1 MB", "Off"... */
const char *td_trash_keep_name(int code); /* "never", "7 days"... */
int td_trash_next_size(int code);         /* the next larger limit, then off */

typedef enum
{
    TD_TRASH_OK,
    TD_TRASH_OFF,       /* the bin is off, or the path is in it */
    TD_TRASH_TOO_BIG,   /* larger than the size limit */
    TD_TRASH_FAILED
} td_trash_result_t;

/* Move a file or folder (a real path) into the bin, then apply the limits. */
td_trash_result_t td_trash_put(const char *path, bool is_dir);

/* Ask the user and delete: into the bin ("Move ... to the Recycle Bin?"),
 * or for good when for_good, when the bin is off or the item is bigger than
 * it holds. done (optional) runs after the answer, to show the change. */
void td_trash_ask_delete(const char *path, bool is_dir, bool for_good, void (*done)(void));
/* The same for several items with one question ("Move 5 items to the
 * Recycle Bin?"): those too big for the bin, or all when for_good or the
 * bin is off, are deleted for good. The paths are copied. One item asks as
 * td_trash_ask_delete does. */
void td_trash_ask_delete_many(const char *const *paths, const bool *is_dir, int count, bool for_good,
                              void (*done)(void));

/* Apply the size and time limits now (also hourly and after each delete). */
void td_trash_purge(void);

/* Put item id back where it was (NULL, or a message); *where gets the path
 * it went to. Users other than root only restore into their home. */
const char *td_trash_restore(const char *id, char *where, int where_cap);
void td_trash_delete(const char *id);   /* one item, for good */
void td_trash_empty(void);              /* everything, for good */

/* ------------------------------------------------------ clipboard */

/* A text clipboard shared by the apps (the Editor's copy, cut and paste).
 * It is emptied when the desktop user changes. */
bool td_clipboard_set(const char *text, int len);
const char *td_clipboard_get(int *len);   /* NULL when empty */
void td_clipboard_clear(void);

/* ---------------------------------------------------- date & time */

/* Open the Date & time window (also opened by clicking the clock). */
void td_datetime_open(void);

/* Date and time of a UTC moment in the user's zone and formats
 * ("02-11-2026 18:05"); "" for 0. */
void td_date_time(int64_t utc, char *buf, int cap);

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
/* Loadable Python bytecode/native modules and self-contained app packages. */
bool td_is_compiled_app(const char *name);

/* Files with a Run command: shell, Python source, .mpy and .tdapp. */
bool td_is_runnable(const char *name);

/* Shortcuts (apps/shortcut.c): NAME.tdlink files whose "target=" line
 * names a file or folder by its shell path. On the Desktop they are
 * icons, in ~/Start menu entries of the Start menu; opening one opens its
 * target. ~/Apps holds a user's app packages (.tdapp). Paths are real. */
bool td_is_shortcut(const char *name);
bool td_shortcut_target(const char *link, char *target, int cap);
const char *td_shortcut_make(const char *dir, const char *target);   /* NULL or an error */
const char *td_apps_dir(void);
const char *td_start_dir(void);
bool td_in_start_menu(const char *target);
const char *td_start_menu_remove(const char *target);              /* NULL or an error */
/* Copy a file into ~/Apps (copy: its real path there). NULL or an error. */
const char *td_copy_to_apps(const char *path, char *copy, int cap);
/* Open a file or folder as the Desktop and Files do: folders in Files,
 * pictures in the Image Viewer, app packages run, shortcuts their target,
 * other files in the Editor. */
void td_open_path(const char *path, bool is_dir);
/* At sign-in: make ~/Apps and ~/Start menu, list the user's entries. */
void td_shortcuts_session(void);
/* Right-click entries for a file or folder: Send to Desktop (when not on
 * it), and for .tdapp packages Add to / Remove from Start menu and Copy
 * to Apps. Fills items and returns how many; the chosen one by index. */
int td_shortcut_menu(const char *path, bool on_desktop, const char **items, int max);
void td_shortcut_menu_chosen(const char *path, bool on_desktop, int index);

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
    TD_PYUI_PAGE,         /* canvas id: a the page drawn into, b the page shown (-1: as is) */
    TD_PYUI_PAGE_COPY,    /* canvas id: page a copied onto page b */
    TD_PYUI_BIG,          /* canvas id: text in big characters at a, b in colours c, d;
                           * e scale (1-4), f width in characters (0: the text's own,
                           * otherwise right-aligned and blanked to that width) */
    /* Pixels. A canvas has 2 x 2 pixels to a cell, in a layer under its
     * cells: a cell drawn in shows them, text whose background is
     * TD_PYUI_CLEAR shows over them. Images are pixel pictures the desktop
     * keeps for the script; they are only seen when drawn into a canvas.
     * Coordinates are pixels; "target" is a canvas (its draw page) or an
     * image. */
    TD_PYUI_IMAGE,        /* image id: text is the td_pyui_image_t pointer, which the
                           * desktop takes over (td_pyui_send_image) */
    TD_PYUI_IMAGE_DATA,   /* target id: text is a td_pyui_image_t pointer whose pixels
                           * go to a, b; the desktop frees it */
    TD_PYUI_DRAW_IMAGE,   /* target id: image win into a, b, size c, d (0: the part's
                           * own, otherwise stretched), its part at e, f of the size in
                           * text (td_pyui_part_t; 0: to the image's edge); flags 1:
                           * nine-slice, the border's pixels kept and the rest stretched */
    TD_PYUI_RECT,         /* target id: a, b, c, d in colour e; f corner radius; flags 1:
                           * outline only; flags 2: text is 7-byte RGB gradient
                           * (rgb1[3], rgb2[3], horizontal boolean), not dithered */
    TD_PYUI_LINE,         /* target id: a, b to c, d in colour e */
    TD_PYUI_GRADIENT,     /* target id: a, b, c, d from the RGB colour text[0..2] to
                           * text[3..5], dithered; flags 1: left to right, else top to
                           * bottom */
    TD_PYUI_CIRCLE,       /* target id: centre a, b, radius c in colour e; flags 1:
                           * outline only; flags 2: same 7-byte gradient as RECT */
    TD_PYUI_PTEXT,        /* target id: text in pixels at a, b in colour c on d
                           * (TD_PYUI_CLEAR: nothing behind it), e times the size;
                           * flags 0: the 8x16 font, 1: the 3x5 one of TD_PYUI_BIG */
    TD_PYUI_PICTURE,      /* canvas id: image win over cells a, b, size c x d, at the
                           * terminal's own resolution where it can (td_picture.h);
                           * its part at e, f; text td_pyui_picture_t then the label;
                           * flags 1: nine-slice. Below it the canvas gets the same in
                           * its pixels, for terminals without pictures. */
    TD_PYUI_BATCH_BEGIN,  /* session-wide, nestable; at most 1000 ms without a frame */
    TD_PYUI_BATCH_END,    /* release at the outer end; no rollback on error/timeout */
    TD_PYUI_BAR,          /* pixel rect a,b,c,d; e: 0..1000 full, f: corner radius;
                           * text[0..2]: fill, track, edge palette colours;
                            * len 10 adds the 7-byte RGB gradient after colours;
                           * flags 1: light top edge. One complete repaint. */
    TD_PYUI_FONT,         /* immutable generated font pointer; desktop owns it */
    TD_PYUI_FONT_TEXT,    /* canvas id, font win; cells a,b,c,d; e fg, f bg;
                           * flags: horizontal 0..2, vertical <<2, wrap 16 */
    TD_PYUI_RENDER_CACHE, /* session-wide: a capacity KiB (0..128), flags 1 warm idle pages */
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
    TD_PYUI_W_CANVAS,     /* c, d size; e pages (0: 1) */
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

/* A colour with none of its own: text and fills over a canvas's pixels
 * show the pixels (text gets the colour most of its cell's pixels have);
 * in an image, transparent. */
#define TD_PYUI_CLEAR (-2)

/* A pixel picture: w x h palette indexes, and where it is transparent. One
 * block from td_pyui_image_new(), freed with free(). */
typedef struct
{
    int16_t w, h;
    uint8_t *px;      /* w * h, row by row */
    uint8_t *mask;    /* NULL (opaque), or w * h, 1 where transparent */
    uint16_t *rgb;    /* optional retained RGB565 source for native pictures */
} td_pyui_image_t;

/* One allocation; file bytes and parsed view remain valid until free(). */
#include "tinydesk/td_font.h"
typedef struct
{
    size_t bytes;
    td_font_t face;
    uint8_t data[];
} td_pyui_font_t;
void td_pyui_send_font(uint16_t id, td_pyui_font_t *font, void (*wait)(void));

/* TD_PYUI_PICTURE: the image's part (0: to its edge), the nine-slice
 * border in the image's pixels, and the label's colour and size (0: as
 * large as fits). The label (UTF-8) follows. */
typedef struct
{
    int16_t w, h, border, fg, scale;
} td_pyui_picture_t;

/* The size of the part of an image TD_PYUI_DRAW_IMAGE draws, and the
 * nine-slice border. */
typedef struct
{
    int16_t w, h, border;
} td_pyui_part_t;

/* The 8x16 font of TD_PYUI_PTEXT: printable ASCII and Latin-1 (16 rows,
 * bit 7 leftmost), or NULL for a character it lacks (apps/pyui_font.c). */
const uint8_t *td_pyui_glyph(uint32_t cp);

#define TD_PYUI_TEXT_MAX  1024   /* text in one message */
#define TD_PYUI_MAX_PAGES 16     /* pages of one canvas */
#ifndef TD_PYUI_MAX_CELLS
#define TD_PYUI_MAX_CELLS 2000   /* all canvases of a script together, every page counted;
                                  * builds with PSRAM set more */
#endif
#ifndef TD_PYUI_MAX_PIXELS
#define TD_PYUI_MAX_PIXELS 16384 /* bytes of images, canvas layers and picture backgrounds
                                  * together (a pixel is a byte, a mask one more); builds
                                  * with PSRAM set more */
#endif

/* The desktop side: start serving scripts (td_apps_register_all does). */
void td_pyui_start(void);

/* A script is using tdui (from its first request until its windows are
 * gone after it ends). */
bool td_pyui_running(void);

/* Kiosk mode (apps/kiosk.c): when td_sysinfo()->kiosk_app is set, the
 * desktop starts in kiosk mode (td_wm_set_kiosk) and runs that Python
 * script in the shell, again whenever it ends. Holding the top-right
 * corner for 5 s, F10 three times within 2 s or Ctrl+Alt+Delete asks for
 * root's password and leaves kiosk mode until the next restart. Called by
 * td_apps_register_all(). */
void td_kiosk_start(void);
bool td_kiosk_active(void);

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

/* Pixels, script side. take() reserves bytes of TD_PYUI_MAX_PIXELS (false
 * when they do not fit), give() returns them; the desktop gives back what
 * images and pixel layers it frees. image_new() is a w x h image of colour
 * 0, with a mask (all opaque) when asked, or NULL without memory.
 * send_image() sends op (TD_PYUI_IMAGE or TD_PYUI_IMAGE_DATA) with the
 * image, which the desktop then owns; it is freed here when there is no
 * desktop to take it (and its bytes given back for TD_PYUI_IMAGE). It
 * waits while the queue is full, calling wait(). */
bool td_pyui_pixels_take(long bytes);
bool td_pyui_enable_rgb(void);
bool td_pyui_rgb_enabled(void);
void td_pyui_cache_stats(uint32_t out[12]);
void td_pyui_pixels_give(long bytes);
td_pyui_image_t *td_pyui_image_new(int w, int h, bool mask);
td_pyui_image_t *td_pyui_image_new_rgb(int w, int h, bool mask, bool rgb);
long td_pyui_image_bytes(const td_pyui_image_t *img);
void td_pyui_send_image(uint8_t op, uint16_t id, int a, int b, td_pyui_image_t *img, void (*wait)(void));

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
bool td_session_locked(void);
bool td_session_auth_pending(void);
bool td_session_has_right(uint32_t right);
int td_session_role(void);
void td_session_lock(void);
void td_kiosk_stop_for_login(void);
void td_session_accept_authenticated(const char *user);
void td_session_accept_login(const char *user, uint32_t generation);
bool td_session_path_allowed(const char *path, int action);
void td_login_open(void);
void td_login_status(const char *message);
void td_users_register(void);
void td_search_register(void);   /* search.c: Alt+S, Win+S */
const char *td_session_home(void);   /* real path, e.g. "/fs/home/bob" */
const char *td_session_jail(void);   /* the Files app cannot go above this */

/* Forget where the Files app was (the next window starts at home). */
void td_files_reset(void);

#endif /* TD_APPS_H */
