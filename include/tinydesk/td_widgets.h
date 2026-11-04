/*
 * td_widgets.h - the v1 widget set: label, button, checkbox, textbox, list,
 * progress bar, scrollbar, and the message box helper.
 *
 * Widget positions are relative to the window's client area. A negative x
 * or y is measured from the right / bottom edge (y = -1 is the last row).
 * A width or height <= 0 extends to that many cells before the right /
 * bottom edge, so td_list(win, td_rect(0, 0, 0, -2), ...) fills the window
 * except for two rows at the bottom. Such widgets follow window resizes.
 */
#ifndef TD_WIDGETS_H
#define TD_WIDGETS_H

#include <stdbool.h>
#include <stdint.h>

#include "td_wm.h"

typedef enum
{
    TD_WT_LABEL,
    TD_WT_BUTTON,
    TD_WT_CHECKBOX,
    TD_WT_TEXTBOX,
    TD_WT_LIST,
    TD_WT_PROGRESS,
    TD_WT_SCROLLBAR,
} td_widget_type_t;

typedef enum
{
    TD_ALIGN_LEFT,
    TD_ALIGN_CENTER,
    TD_ALIGN_RIGHT
} td_align_t;

/* Colour value meaning "use the theme". */
#define TD_COLOR_DEFAULT (-1)

typedef void (*td_widget_fn)(td_widget_t *w, void *user);

/* Return the text of list item `index`. *fg may be set to a palette colour
 * for this item (it starts as TD_COLOR_DEFAULT). */
typedef const char *(*td_list_item_fn)(td_widget_t *w, int index, int *fg, void *user);

/* A widget. Read the fields freely; change them through the functions. */
struct td_widget
{
    bool used;
    uint8_t type;
    uint8_t align;
    bool focusable;
    bool visible;
    bool pressed;            /* button held down with the mouse */
    bool secret;             /* textbox: show '*' instead of the text */
    td_rect_t rect;          /* relative to the client area (see above) */
    td_window_t *win;
    td_widget_t *next;
    char text[TD_TEXT_MAX];  /* label / caption / textbox contents (see td_widget_text) */
    char *ext;               /* textbox: a longer buffer of the app's own, or NULL */
    int ext_cap;             /* its size in bytes */
    int value;               /* checked, percent, selected item, cursor */
    int scroll;              /* first visible item / column */
    int count;               /* list item count */
    int maxlen;              /* textbox maximum length in bytes */
    int fg, bg;              /* TD_COLOR_DEFAULT or a palette index */
    td_widget_fn on_activate;
    td_list_item_fn get_item;
    td_widget_t *target;     /* scrollbar: the list it controls */
    uint32_t last_click_ms;  /* list double-click detection */
    void *user;
};

/* Static or dynamic text. width <= 0 means "to the right edge". */
td_widget_t *td_label(td_window_t *win, int x, int y, int width, const char *text);

/* Push button drawn as "[ caption ]"; fn runs on click, Enter or Space. */
td_widget_t *td_button(td_window_t *win, int x, int y, const char *caption,
                       td_widget_fn fn, void *user);

/* "[x] caption" toggle; fn runs after every change. */
td_widget_t *td_checkbox(td_window_t *win, int x, int y, const char *caption,
                         bool checked, td_widget_fn fn, void *user);

/* Single-line text entry; fn runs when Enter is pressed. */
td_widget_t *td_textbox(td_window_t *win, int x, int y, int width, int maxlen,
                        td_widget_fn fn, void *user);

/* Pasted text (TD_EV_PASTE) into the window's focused text box; false if
 * no text box has the focus. */
bool td_widgets_paste(td_window_t *win);

/* Scrollable list. get_item supplies the text, fn runs on Enter or
 * double-click. Set the number of items with td_list_set_count(). */
td_widget_t *td_list(td_window_t *win, td_rect_t rect, td_list_item_fn get_item,
                     td_widget_fn fn, void *user);

/* Horizontal bar, 0..100 %. */
td_widget_t *td_progress(td_window_t *win, int x, int y, int width);

/* Vertical scrollbar that follows and controls `target` (a list). */
td_widget_t *td_scrollbar(td_window_t *win, int x, int y, int height, td_widget_t *target);

/* Common setters. */
void td_widget_set_text(td_widget_t *w, const char *text);

/* The widget's text ("" for NULL): use it instead of w->text, which is
 * empty for a text box with its own buffer. */
const char *td_widget_text(const td_widget_t *w);

/* Give a text box a buffer of the app's own, longer than TD_TEXT_MAX (for
 * a URL, say). The buffer must live as long as the window; its contents
 * become the text, and maxlen becomes cap - 1. */
void td_textbox_set_buffer(td_widget_t *w, char *buf, int cap);
void td_widget_printf(td_widget_t *w, const char *fmt, ...);
void td_widget_set_align(td_widget_t *w, td_align_t align);
void td_widget_set_color(td_widget_t *w, int fg, int bg);
void td_widget_set_visible(td_widget_t *w, bool visible);
void td_widget_focus(td_widget_t *w);

/* Absolute screen rectangle of a widget (resolves <= 0 sizes). */
td_rect_t td_widget_rect(const td_widget_t *w);

/* Checkbox. */
bool td_checkbox_get(const td_widget_t *w);
void td_checkbox_set(td_widget_t *w, bool checked);

/* Progress bar. */
void td_progress_set(td_widget_t *w, int percent);

/* List. */
void td_list_set_count(td_widget_t *w, int count);
int td_list_selected(const td_widget_t *w);
/* Restore a live list without scrolling its selection into view. -1 clears
 * selection when the previously selected object has disappeared. */
void td_list_set_view(td_widget_t *w, int selected, int scroll);
void td_list_select(td_widget_t *w, int index);

/* Modal message box. `buttons` is a '|' separated list such as "OK" or
 * "Yes|No". fn receives the index of the chosen button, or -1 for Esc.
 * Returns the dialog window (or NULL). */
td_window_t *td_msgbox(const char *title, const char *text, const char *buttons,
                       void (*fn)(int button, void *user), void *user);

/* Modal one-line text prompt with OK / Cancel. fn runs with the entered
 * text when OK or Enter is pressed (not on Cancel / Esc). */
td_window_t *td_inputbox(const char *title, const char *prompt, const char *initial,
                         void (*fn)(const char *text, void *user), void *user);

/* The same for passwords: the typed text is shown as '*'. */
td_window_t *td_passwordbox(const char *title, const char *prompt,
                            void (*fn)(const char *text, void *user), void *user);

/* --- used by the window manager ------------------------------------- */

/* Draw all widgets of a window (origin/clip set to the client area). */
void td_widgets_draw(td_window_t *win);

/* Offer an event to the widgets. Mouse coordinates are client-relative.
 * Returns true if a widget consumed it. */
bool td_widgets_event(td_window_t *win, const td_event_t *ev);

/* Free every widget belonging to win. */
void td_widgets_free(td_window_t *win);

/* Move keyboard focus to the next (dir > 0) or previous focusable widget. */
void td_widgets_focus_next(td_window_t *win, int dir);

/* Free every widget in the pool. */
void td_widgets_reset(void);

#endif /* TD_WIDGETS_H */
