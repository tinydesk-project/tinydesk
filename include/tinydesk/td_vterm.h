/*
 * td_vterm.h - a small VT100/ANSI terminal emulator used to show the output
 * of a shell inside a window.
 *
 * It is deliberately a subset (the spec leaves full VT100 emulation out of
 * v1): printable UTF-8, CR/LF/BS/TAB/BEL, cursor movement, erase, insert and
 * delete, scroll regions, SGR colours (16, 256 and approximated true
 * colour), save/restore cursor, cursor visibility, the alternate screen,
 * and replies to device-status / cursor-position queries. That covers line
 * editors and simple full-screen programs such as TinyDesk Shell's nano.
 */
#ifndef TD_VTERM_H
#define TD_VTERM_H

#include <stdbool.h>
#include <stdint.h>

#include "td_config.h"
#include "td_screen.h"

/* 4-byte cell: bold and reverse are folded into the colours. */
typedef struct
{
    uint16_t ch;
    uint8_t fg, bg;
} td_vcell_t;

/* Called with bytes the emulator must send back to the program (answers
 * to ESC [ 6 n and friends). */
typedef void (*td_vterm_reply_fn)(void *user, const char *data, int len);

typedef struct
{
    int cols, rows;
    td_vcell_t cells[TD_VT_MAX_ROWS * TD_VT_MAX_COLS];   /* stride TD_VT_MAX_COLS */
    td_vcell_t sb[TD_VT_SCROLLBACK * TD_VT_MAX_COLS];    /* scrollback ring */
    int sb_head;          /* next scrollback line to overwrite */
    int sb_count;
    int cx, cy;           /* cursor */
    int saved_cx, saved_cy;
    uint8_t fg, bg;       /* current colours (before bold/reverse) */
    uint8_t attr;         /* TD_BOLD | TD_REVERSE | TD_UNDERLINE */
    uint8_t saved_fg, saved_bg, saved_attr;
    uint8_t def_fg, def_bg;
    int top, bottom;      /* scroll region, inclusive rows */
    bool wrap_pending;    /* last column written; wrap before next char */
    bool autowrap;
    bool cursor_visible;
    bool synchronized_output; /* DEC 2026: retain display until frame end */
    bool newline_mode;    /* LF also returns the carriage (default on) */

    uint8_t state;
    char params[40];
    int plen;
    bool priv;            /* CSI started with '?' */
    td_utf8_decoder_t utf8;

    td_vterm_reply_fn reply;
    void *reply_user;
    bool dirty;           /* set when anything changed; clear it yourself */

    /* Optional, set after td_vterm_init(): the bytes of DCS strings
     * (ESC P ... ESC \), such as Sixel pictures, go to dcs(dcs_user, b),
     * -1 before the first and -2 after the last; without it they are
     * skipped. sixel: say so in the device attributes (ESC [ c); cell_w,
     * cell_h: a cell's size in pixels, the answer to ESC [ 16 t. */
    void (*dcs)(void *user, int b);
    void *dcs_user;
    bool sixel;
    int cell_w, cell_h;
} td_vterm_t;

/* Reset to a blank screen of the given size (clamped to the maximum). */
void td_vterm_init(td_vterm_t *vt, int cols, int rows);

/* Change the size, keeping the content anchored at the top-left and the
 * cursor visible (lines scrolled off the top go to the scrollback). */
void td_vterm_resize(td_vterm_t *vt, int cols, int rows);

/* Feed program output. */
void td_vterm_write(td_vterm_t *vt, const uint8_t *data, int len);

/* Lines currently held in the scrollback. */
int td_vterm_scrollback_lines(const td_vterm_t *vt);

/* Draw at (x, y) through the normal drawing primitives. scroll_back > 0
 * shows older lines; the cursor is drawn when show_cursor is set. */
void td_vterm_draw(const td_vterm_t *vt, int x, int y, int scroll_back, bool show_cursor);

#endif /* TD_VTERM_H */
