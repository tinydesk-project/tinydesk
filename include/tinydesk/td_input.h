/*
 * td_input.h - input events, the escape-sequence parser, the event queue
 * and timers.
 */
#ifndef TD_INPUT_H
#define TD_INPUT_H

#include <stdbool.h>
#include <stdint.h>

#include "td_config.h"
#include "td_screen.h"

/* ------------------------------------------------------------- events */

/* ESC [ 5000 ~ is a tinydesk extension: "a terminal was just attached,
 * send the setup and redraw everything" (no key event). Tools that switch
 * terminals on a line that stays open, like tools/serial_bridge.py, send
 * it; a directly connected terminal does not need it (tinydesk redraws
 * after TD_RECONNECT_SILENCE_MS of silence). */
#define TD_SEQ_REDRAW 5000

/* TD_EV_PASTE: text pasted in the terminal (bracketed paste); read it with
 * td_paste_text() while the event is being handled. x is 1 if the paste
 * was longer than TD_PASTE_MAX and got cut. */
typedef enum
{
    TD_EV_KEY,
    TD_EV_MOUSE,
    TD_EV_RESIZE,
    TD_EV_TICK,
    TD_EV_PASTE
} td_ev_type_t;

/* Modifier bits. */
#define TD_MOD_SHIFT 0x01u
#define TD_MOD_ALT   0x02u
#define TD_MOD_CTRL  0x04u

/* Mouse actions. */
#define TD_MOUSE_PRESS   0
#define TD_MOUSE_RELEASE 1
#define TD_MOUSE_DRAG    2
#define TD_MOUSE_MOVE    3

/* Mouse buttons. */
#define TD_BUTTON_LEFT       0
#define TD_BUTTON_MIDDLE     1
#define TD_BUTTON_RIGHT      2
#define TD_BUTTON_NONE       3
#define TD_BUTTON_WHEEL_UP   4
#define TD_BUTTON_WHEEL_DOWN 5

/* Special keys live above the Unicode range so they never collide with a
 * character. Ctrl+letter arrives as the lower-case letter with TD_MOD_CTRL. */
enum
{
    TD_KEY_BASE = 0x110000,
    TD_KEY_ENTER,
    TD_KEY_TAB,
    TD_KEY_BACKSPACE,
    TD_KEY_ESC,
    TD_KEY_UP,
    TD_KEY_DOWN,
    TD_KEY_RIGHT,
    TD_KEY_LEFT,
    TD_KEY_HOME,
    TD_KEY_END,
    TD_KEY_INSERT,
    TD_KEY_DELETE,
    TD_KEY_PGUP,
    TD_KEY_PGDN,
    TD_KEY_F1,
    TD_KEY_F2,
    TD_KEY_F3,
    TD_KEY_F4,
    TD_KEY_F5,
    TD_KEY_F6,
    TD_KEY_F7,
    TD_KEY_F8,
    TD_KEY_F9,
    TD_KEY_F10,
    TD_KEY_F11,
    TD_KEY_F12,
};

typedef struct
{
    td_ev_type_t type;
    uint32_t key;      /* Unicode char or TD_KEY_* code */
    uint8_t mods;      /* TD_MOD_SHIFT | TD_MOD_ALT | TD_MOD_CTRL */
    int16_t x, y;      /* mouse: 0-based cell coords; resize: cols, rows */
    uint8_t button;    /* 0 left, 1 middle, 2 right, 4/5 wheel */
    uint8_t action;    /* PRESS, RELEASE, DRAG, MOVE */
    uint32_t time_ms;  /* when the event was parsed */
} td_event_t;

/* Short readable name of a key, e.g. "Ctrl+a", "F5", "x" (for demos). */
const char *td_key_name(const td_event_t *ev, char *buf, int cap);

/* -------------------------------------------------------- event queue */

/* Add an event. Consecutive mouse drag/move events are merged so a slow
 * link never builds up a backlog. Returns false if the queue was full. */
bool td_event_push(const td_event_t *ev);

/* Take the oldest event. Returns false when the queue is empty. */
bool td_event_pop(td_event_t *ev);

/* Number of queued events. */
int td_event_count(void);

/* Drop all queued events. */
void td_event_clear(void);

/* ------------------------------------------------------------- parser */

/* Parser state. Treat the fields as private. */
typedef struct
{
    uint8_t state;
    uint8_t seq[32];          /* bytes of the current escape sequence */
    uint8_t seq_len;
    bool seq_overflow;        /* sequence too long: skip to its end */
    uint8_t x10_left;         /* raw bytes still expected (legacy mouse) */
    uint32_t seq_start;       /* time the sequence started */
    td_utf8_decoder_t utf8;
    uint32_t last_byte_ms;    /* time of the most recent byte */
    /* Bracketed paste being received, and the last one delivered. */
    char *paste;
    int paste_len, paste_cap;
    uint8_t paste_match;      /* bytes held in seq: end marker or size reply */
    bool paste_cut;
    char *pasted;
    int pasted_len;
} td_input_t;

/* Reset the parser. */
void td_input_init(td_input_t *p);

/* The text of the TD_EV_PASTE being handled (NULL if none). Line breaks are
 * as the terminal sent them (usually CR). */
const char *td_paste_text(int *len);

/* Free the delivered paste (the main loop calls it after the event). */
void td_input_paste_done(td_input_t *p);

/* Feed one received byte; completed events go to the event queue. */
void td_input_feed(td_input_t *p, uint8_t byte, uint32_t now_ms);

/* True while a bracketed paste is being received. */
bool td_input_pasting(const td_input_t *p);

/* Call regularly: turns a lone ESC into the ESC key after
 * TD_ESC_TIMEOUT_MS and discards stalled sequences. */
void td_input_poll_timeouts(td_input_t *p, uint32_t now_ms);

/* ------------------------------------------------------------- timers */

typedef void (*td_timer_fn)(void *user);

/* Start a timer. Returns an id >= 0, or -1 if the pool is full. The first
 * call happens interval_ms after now_ms. */
int td_timer_start(uint32_t interval_ms, bool repeat, td_timer_fn fn,
                   void *user, uint32_t now_ms);

/* Stop a timer (ids that are not running are ignored). */
void td_timer_stop(int id);

/* Run every timer that is due. */
void td_timers_run(uint32_t now_ms);

/* Stop all timers. */
void td_timers_reset(void);

#endif /* TD_INPUT_H */
