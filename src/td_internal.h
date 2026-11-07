/*
 * td_internal.h - what TinyDesk's core (src/) shares between its parts and
 * applications do not call: the picture transport around each frame and
 * the calls between the window manager and the widgets. Not part
 * of the stable API (see STABILITY.md): may change in any release.
 */
#ifndef TD_INTERNAL_H
#define TD_INTERNAL_H

#include "tinydesk/td.h"
#include "tinydesk/td_input.h"
#include "tinydesk/td_picture.h"
#include "tinydesk/td_screen.h"
#include "tinydesk/td_widgets.h"
#include "tinydesk/td_wm.h"

#ifdef __cplusplus
extern "C" {
#endif

/* from tinydesk/td_picture.h */
  /* Used by the core (src/td.c) around composing and sending a frame. */
void td_pictures_begin(void);
 /* Retain an existing image while its replacement is transmitted; update
 * the logical front cells without emitting their low-resolution fallback. */
void td_pictures_mark(td_buffer_t *front, const td_buffer_t *back);
void td_pictures_send(void (*write)(void *user, const char *s, int n), void *user, bool *cursor_moved);
 /* Only pictures inside area go out (w 0: all), placed from its corner:
 * the primary terminal's display on an extended screen. */
void td_pictures_clip(td_rect_t area);
 /* The window manager, while composing: pictures added so far, and drop
 * those of first .. last - 1 that r overlaps (a window, menu or the taskbar
 * drawn above them), so they cannot cover it. */
int td_pictures_count(void);
void td_pictures_cover(int first, int last, td_rect_t r);

/* from tinydesk/td_widgets.h */
/* The window's pressed list keeps a drag at x, y (client cells) for
 * selecting: the window manager starts no drag and drop there. */
bool td_widgets_keeps_drag(td_window_t *win, int x, int y);
/* The window manager took the pressed widget's drag over (drag and drop). */
void td_widgets_cancel_press(void);
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

#ifdef __cplusplus
}
#endif

#endif /* TD_INTERNAL_H */
