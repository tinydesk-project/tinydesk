/*
 * td_wm.h - windows, z-order, focus, the desktop, taskbar, start menu,
 * registered apps and themes.
 */
#ifndef TD_WM_H
#define TD_WM_H

#include <stdbool.h>
#include <stdint.h>

#include "td_config.h"
#include "td_input.h"
#include "td_screen.h"

/* Window flags. */
#define TD_WIN_MOVABLE    0x0001u
#define TD_WIN_RESIZABLE  0x0002u
#define TD_WIN_CLOSABLE   0x0004u
#define TD_WIN_MODAL      0x0008u
#define TD_WIN_RAW_KEYS   0x0010u  /* all keys (Tab, Esc, ...) go to on_event */
#define TD_WIN_HIDDEN     0x0100u  /* minimised to the taskbar */
#define TD_WIN_MAXIMIZED  0x0200u
#define TD_WIN_FULLSCREEN 0x0400u  /* no frame, covers the taskbar */

#define TD_WIN_DEFAULT (TD_WIN_MOVABLE | TD_WIN_RESIZABLE | TD_WIN_CLOSABLE)

typedef struct td_window td_window_t;
typedef struct td_widget td_widget_t;
/* Authentication gate: only this window receives input and is composed.
 * Unlike a modal dialog it also blocks taskbar and global shortcuts. */
void td_wm_set_input_gate(bool locked,td_window_t *window);

/* A file or folder being dragged with the mouse. */
typedef struct
{
    char path[TD_PATH_MAX];   /* real path */
    char name[48];
    bool is_dir;
} td_drag_item_t;

/* Everything needed to open a window. Unused callbacks may be NULL. */
typedef struct
{
    const char *title;
    td_rect_t rect;         /* outer frame; x or y < 0 centres the window */
    uint16_t flags;
    int min_w, min_h;       /* resize limits (0 = defaults) */

    /* Draw the client area. Origin and clip are already set to the client
     * rectangle, so (0, 0) is its top-left cell. Widgets are drawn after. */
    void (*on_draw)(td_window_t *win, int client_w, int client_h);

    /* Handle an event that no widget consumed. Mouse coordinates are
     * relative to the client area. Return true when handled. */
    bool (*on_event)(td_window_t *win, const td_event_t *ev);

    /* Called just before the window is destroyed. */
    void (*on_close)(td_window_t *win);

    /* Asked when the user closes the window ([x], window menu, Esc on a
     * dialog). Return false to keep it open (e.g. to ask about unsaved
     * changes first). NULL means always allow. */
    bool (*on_close_request)(td_window_t *win);

    /* Called every tick_ms milliseconds while the window is open. */
    void (*on_tick)(td_window_t *win);
    uint32_t tick_ms;

    /* Drag and drop (optional). on_drag_start is asked when a left-button
     * drag starts at client (x, y): fill *item and return true to drag a
     * file or folder instead of passing the motion on. on_drop receives an
     * item released over the client area; return true if it was used. */
    bool (*on_drag_start)(td_window_t *win, int x, int y, td_drag_item_t *item);
    bool (*on_drop)(td_window_t *win, int x, int y, const td_drag_item_t *item);

    /* Two-cell glyph for the taskbar button (large taskbar). NULL: the
     * icon of the app being launched, if any. */
    const char *icon;

    void *user;
} td_window_desc_t;

/* A window. Read the fields freely; change them through the functions. */
struct td_window
{
    bool used;
    uint8_t id;
    uint32_t generation;    /* changes when a closed pool slot is reused */
    char title[TD_TITLE_MAX];
    td_rect_t rect;          /* outer frame in screen cells */
    td_rect_t restore_rect;  /* rect before maximise / fullscreen */
    uint16_t flags;
    int min_w, min_h;
    td_widget_t *widgets;    /* singly linked, in creation order */
    td_widget_t *focus;      /* focused widget or NULL */
    bool incomplete;         /* a widget (TD_MAX_WIDGETS) or its tick timer (TD_MAX_TIMERS) did not fit */
    void (*on_draw)(td_window_t *win, int client_w, int client_h);
    bool (*on_event)(td_window_t *win, const td_event_t *ev);
    void (*on_close)(td_window_t *win);
    bool (*on_close_request)(td_window_t *win);
    void (*on_tick)(td_window_t *win);
    bool (*on_drag_start)(td_window_t *win, int x, int y, td_drag_item_t *item);
    bool (*on_drop)(td_window_t *win, int x, int y, const td_drag_item_t *item);
    int timer_id;
    const char *icon;        /* taskbar glyph or NULL */
    void *user;
};

/* ----------------------------------------------------------- windows */

/* Open a window on top of the others and focus it. Returns NULL when the
 * window pool is full. */
td_window_t *td_win_create(const td_window_desc_t *desc);

/* Close a window (calls on_close, frees its widgets). */
void td_win_close(td_window_t *win);

/* Close as if the user asked to: on_close_request may refuse. */
void td_win_request_close(td_window_t *win);

/* Raise, un-minimise and focus a window. */
void td_win_focus(td_window_t *win);

/* Focused (top-most visible) window, or NULL. */
td_window_t *td_win_focused(void);

/* Change the title. */
void td_win_set_title(td_window_t *win, const char *title);

/* Move / resize the outer frame (clamped to the screen and minimum size). */
void td_win_move(td_window_t *win, int x, int y);
void td_win_resize(td_window_t *win, int w, int h);

/* Minimise to the taskbar, toggle maximise, set fullscreen. */
void td_win_minimize(td_window_t *win);
void td_win_toggle_maximize(td_window_t *win);
void td_win_set_fullscreen(td_window_t *win, bool on);

/* Client area in absolute screen coordinates. */
td_rect_t td_win_client(const td_window_t *win);

/* Request a redraw (the next frame recomposes the whole screen). */
void td_win_invalidate(td_window_t *win);

/* True when win points at a window that is currently open. */
bool td_win_is_open(const td_window_t *win);

/* ---------------------------------------------------- window manager */

/* Reset all windows and set the screen size. */
void td_wm_init(int cols, int rows);

/* Tell the window manager the terminal size changed. */
void td_wm_set_screen_size(int cols, int rows);

/* The screen size in cells. */
void td_wm_screen_size(int *cols, int *rows);

/* Route one event to the start menu, taskbar, windows and widgets. */
void td_wm_dispatch(const td_event_t *ev);

/* Draw the desktop, windows, taskbar and menu into back. */
void td_wm_compose(td_buffer_t *back);

/* True if something changed since the last compose. */
bool td_wm_needs_redraw(void);

/* Mark the screen as needing a recompose. */
void td_wm_invalidate(void);

/* Open / close the start menu. */
void td_wm_toggle_start_menu(void);

/* Add an entry to the start menu's lower section (e.g. "Switch user..."). */
void td_wm_add_start_item(const char *label, void (*fn)(void));

/* Close every window at once, without asking (used when the user changes). */
void td_wm_close_all(void);

/* Kiosk mode: no taskbar, Start menu, desktop icons or window menus, every
 * window but a dialog full screen without a frame, and no keys that switch,
 * close or resize windows. hook (optional) sees every mouse and key event
 * first and returns true to take it. Turning it off gives the windows their
 * frames back. */
void td_wm_set_kiosk(bool on, bool (*hook)(const td_event_t *ev));
bool td_wm_kiosk(void);

/* The taskbar clock (optional). text() fills in the clock (return false to
 * hide it); click() gets TD_BUTTON_LEFT or TD_BUTTON_RIGHT and the cell. */
typedef struct
{
    bool (*text)(char *buf, int cap);
    void (*click)(int button, int x, int y);
    /* Optional: the time and the date apart, for the small (time only) and
     * the large (two rows) taskbar. */
    bool (*parts)(char *time, int tcap, char *date, int dcap);
} td_clock_provider_t;
void td_wm_set_clock(const td_clock_provider_t *clock);

/* Name shown in the taskbar tray (the logged-in user); NULL hides it. */
void td_wm_set_user_label(const char *label);

/* Sizes of the desktop icons, the start menu and the taskbar.
 *   icons:  small = one line (glyph + name), medium = 4x3 box + two label
 *           lines, large = 8x5 box + two wider label lines
 *   start menu: small = names, medium = glyph + name, large = glyph + name
 *           with a row of space (falls back to one row if it won't fit)
 *   taskbar: small = short buttons and the time only, medium = one row,
 *           large = two rows: glyph + title buttons, time over date */
typedef enum
{
    TD_UI_SMALL,
    TD_UI_MEDIUM,
    TD_UI_LARGE
} td_ui_size_t;
void td_wm_set_icon_size(td_ui_size_t size);
td_ui_size_t td_wm_icon_size(void);
void td_wm_set_start_menu_size(td_ui_size_t size);
td_ui_size_t td_wm_start_menu_size(void);
void td_wm_set_taskbar_size(td_ui_size_t size);
td_ui_size_t td_wm_taskbar_size(void);
/* Window frames: small = one title row, buttons [-][x] (the default);
 * medium = buttons [ - ][ x ] and a wider resize corner; large = the same
 * with two title rows. Windows grow or shrink by the title bar's change,
 * so their contents keep their size. */
void td_wm_set_window_size(td_ui_size_t size);
td_ui_size_t td_wm_window_size(void);
/* Rows of a framed window's title bar (1, or 2 with large frames). */
int td_wm_title_rows(void);
const char *td_ui_size_name(td_ui_size_t size);   /* "Small", "Medium", "Large" */

/* ------------------------------------------------------- popup menus */

/* Called with the index of the chosen item (not called when the menu is
 * dismissed with Esc or a click elsewhere). */
typedef void (*td_menu_fn)(int item, void *user);

/* Open a popup (context) menu with its top-left corner near (x, y). The
 * labels are copied; an item "-" is a separator. Up to 20 items. Mouse
 * hover and the arrow keys move the highlight; Enter or a click chooses. */
void td_menu_popup(int x, int y, const char *const *items, int count,
                   td_menu_fn fn, void *user);
void td_menu_close(void);
bool td_menu_is_open(void);

/* The open windows (dialogs left out), in window-pool slot order (a new
 * window reuses the first free slot); returns the count (at most max). */
int td_wm_windows(td_window_t **out, int max);

/* Height of the usable desktop (screen rows minus the taskbar). */
int td_wm_desktop_rows(void);

/* Top-most visible window at a screen cell, or NULL (desktop). */
td_window_t *td_wm_window_at(int x, int y);

/* Last known mouse position in screen cells; false before the first mouse
 * event. Used for hover highlights. */
bool td_wm_mouse_pos(int *x, int *y);

/* Draw the mouse pointer (a triangle in the cell under it), for a mouse
 * the terminal does not show, such as one on the board's own USB port.
 * Off by default; may be called from any task. */
void td_wm_set_pointer(bool on);

/* -------------------------------------------------------------- apps */

typedef struct
{
    const char *name;        /* shown in the start menu and under the icon */
    void (*launch)(void);    /* open (or focus) the app's window */
    const char *icon;        /* two-cell desktop icon glyph, e.g. ">_" (NULL:
                              * first letter of the name) */
} td_app_t;

/* Register an app for the start menu. The descriptor must stay valid.
 * Returns 0, or -1 when TD_MAX_APPS is reached. */
int td_app_register(const td_app_t *app);

/* Registered apps. */
int td_app_count(void);
const td_app_t *td_app_get(int index);

/* Launch an app by name; returns false if it is not registered. */
bool td_app_launch(const char *name);

/* ------------------------------------------------------------- theme */

typedef struct
{
    const char *name;
    uint8_t desktop_fg, desktop_bg;
    uint8_t win_fg, win_bg;              /* window client area */
    uint8_t frame_fg, frame_active_fg;   /* borders */
    uint8_t title_fg, title_bg;          /* focused title bar */
    uint8_t title_inactive_fg, title_inactive_bg;
    uint8_t button_fg, button_bg;
    uint8_t focus_fg, focus_bg;          /* focused widget */
    uint8_t input_fg, input_bg;          /* text boxes, lists */
    uint8_t select_fg, select_bg;        /* selected list item */
    uint8_t shadow_fg, shadow_bg;
    uint8_t taskbar_fg, taskbar_bg;
    uint8_t taskbar_active_fg, taskbar_active_bg;
    uint8_t menu_fg, menu_bg, menu_select_fg, menu_select_bg;
    uint8_t accent;                      /* highlights, progress bars */
    uint8_t dim;                         /* disabled / secondary text */
    uint8_t icon_fg;                     /* desktop icons */
    uint8_t close_hover_fg, close_hover_bg; /* [x] under the mouse */
} td_theme_t;

/* Active theme. */
const td_theme_t *td_theme(void);

/* Built-in themes: 0 = Classic, 1 = Dark (the default). */
#define TD_THEME_CLASSIC 0
#define TD_THEME_DARK    1
#define TD_THEME_DEFAULT TD_THEME_DARK
int td_theme_count(void);
const td_theme_t *td_theme_get(int index);
void td_theme_set(int index);
int td_theme_index(void);

/* Desktop background pattern character. */
void td_desktop_set_pattern(uint32_t ch);
uint32_t td_desktop_pattern(void);

/* Show or hide the desktop icons (one per registered app). */
void td_desktop_set_icons(bool on);
bool td_desktop_icons(void);

/* Extra desktop icons after the app icons, e.g. the files in a Desktop
 * folder. Index arguments count from 0 within the provider's items. */
typedef struct
{
    int (*count)(void *user);
    const char *(*label)(int index, void *user);
    const char *(*icon)(int index, void *user);      /* two cells, or NULL */
    /* Colour of item `index`'s glyph (256-colour palette index), or -1 for
     * the theme's icon colour. Optional. */
    int (*icon_fg)(int index, void *user);
    void (*open)(int index, void *user);             /* double-click / Enter */
    /* Right-click on item `index`, or on the empty desktop (index -1). */
    void (*context)(int index, int x, int y, void *user);
    /* Drag and drop: describe item `index` for dragging (return false to
     * refuse), and receive a drop on item `index` or on the empty desktop
     * (index -1). */
    bool (*drag)(int index, td_drag_item_t *item, void *user);
    void (*drop)(int index, const td_drag_item_t *item, void *user);
    void *user;
} td_desktop_provider_t;

/* True while a file or folder is being dragged. */
bool td_drag_active(void);

void td_desktop_set_provider(const td_desktop_provider_t *provider);

#endif /* TD_WM_H */
