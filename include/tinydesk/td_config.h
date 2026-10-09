/*
 * td_config.h - compile-time limits for tinydesk.
 *
 * Every static pool in the core is sized here. Override any value from the
 * build system (for example -DTD_MAX_COLS=100) before this header is read.
 */
#ifndef TD_CONFIG_H
#define TD_CONFIG_H

/* Largest terminal size the screen buffers can hold. Each buffer costs
 * TD_MAX_COLS * TD_MAX_ROWS * 8 bytes and there are two of them. The
 * defaults are for PCs (a maximised terminal on a large screen); the
 * ESP-IDF builds set smaller limits. */
#ifndef TD_MAX_COLS
#define TD_MAX_COLS 400
#endif
#ifndef TD_MAX_ROWS
#define TD_MAX_ROWS 150
#endif

/* Longest real path the apps handle: the file system root plus the path
 * the shell sees ("/fs" + "/home/bob/Desktop/notes.txt" on a board). On a
 * PC the root is <working directory>/tinydesk_fs and can be long, so the
 * PC build sets 512. Every path buffer of the apps has this size (several
 * are on the stack). */
#ifndef TD_PATH_MAX
#define TD_PATH_MAX 160
#endif

/* Size used until the terminal answers the size query. */
#ifndef TD_DEFAULT_COLS
#define TD_DEFAULT_COLS 80
#endif
#ifndef TD_DEFAULT_ROWS
#define TD_DEFAULT_ROWS 25
#endif

/* Smallest usable desktop. */
#define TD_MIN_COLS 40
#define TD_MIN_ROWS 12

/* Output buffer used by the renderer; written to the HAL in large chunks. */
#ifndef TD_OUT_BUF_SIZE
#define TD_OUT_BUF_SIZE 4096
#endif

/* Input event ring buffer. */
#ifndef TD_EVENT_QUEUE_SIZE
#define TD_EVENT_QUEUE_SIZE 32
#endif

/* A lone ESC becomes the ESC key after this many milliseconds. */
#ifndef TD_ESC_TIMEOUT_MS
#define TD_ESC_TIMEOUT_MS 50
#endif

/* Bracketed paste: the most text one paste may bring (it is held in a heap
 * buffer only while it arrives), and how long a paste may stall before it
 * is delivered anyway (the end marker got lost). */
#ifndef TD_PASTE_MAX
#define TD_PASTE_MAX 8192
#endif
#ifndef TD_PASTE_TIMEOUT_MS
#define TD_PASTE_TIMEOUT_MS 2000
#endif

/* Copying in the Editor also puts the text on the PC's clipboard through
 * OSC 52 (terminals that support it: Windows Terminal, xterm, WezTerm,
 * kitty...; PuTTY ignores it). Longer copies are not sent. 0 disables. */
#ifndef TD_OSC52_MAX
#define TD_OSC52_MAX 8192
#endif

/* Unfinished escape sequences are discarded after this long. */
#ifndef TD_SEQ_TIMEOUT_MS
#define TD_SEQ_TIMEOUT_MS 500
#endif

/* Answer time allowed for the start-up terminal size query. */
#ifndef TD_SIZE_QUERY_TIMEOUT_MS
#define TD_SIZE_QUERY_TIMEOUT_MS 300
#endif

/* The terminal size is re-queried this often so window resizes are noticed
 * (0 disables periodic queries). */
#ifndef TD_SIZE_POLL_MS
#define TD_SIZE_POLL_MS 1000
#endif

/* After a dropped frame the renderer retries a full redraw this often. */
#ifndef TD_LINK_RETRY_MS
#define TD_LINK_RETRY_MS 500
#endif

/* Frames dropped for less than this are a slow terminal: every cell is sent
 * again over the picture it still shows, without clearing (no flicker).
 * Longer means the terminal was gone (closed, unplugged): the next redraw,
 * or input, sets the terminal up again and clears it. */
#ifndef TD_LINK_LOST_MS
#define TD_LINK_LOST_MS 3000
#endif

/* Frame pacing: after each frame the terminal is asked ESC[5n, and the next
 * frame waits for its answer, so a terminal that draws slowly (PuTTY) never
 * has more than one frame queued and input shows up at once. A terminal
 * that does not answer within this time gets the next frame anyway; after
 * three such frames without any answer, pacing stays off for it. 0 turns
 * pacing off (no ESC[5n, no waiting). */
#ifndef TD_ACK_TIMEOUT_MS
#define TD_ACK_TIMEOUT_MS 500
#endif

/* Input after this much silence is treated as a (re)connected terminal. */
#ifndef TD_RECONNECT_SILENCE_MS
#define TD_RECONNECT_SILENCE_MS 5000
#endif

/* Window manager pools. */
#ifndef TD_MAX_WINDOWS
#define TD_MAX_WINDOWS 16            /* the last one is kept for a message box */
#endif
#ifndef TD_MAX_WIDGETS
#define TD_MAX_WIDGETS 128           /* all windows together; the last 4 for a message box */
#endif
#ifndef TD_MAX_TIMERS
#define TD_MAX_TIMERS 24             /* one per window with on_tick, plus the desktop's own */
#endif
#ifndef TD_MAX_APPS
#define TD_MAX_APPS 16
#endif

/* String sizes (bytes, including the terminating NUL). */
#ifndef TD_TITLE_MAX
#define TD_TITLE_MAX 32
#endif
#ifndef TD_TEXT_MAX
#define TD_TEXT_MAX 64
#endif

/* Double-click time window. */
#ifndef TD_DOUBLE_CLICK_MS
#define TD_DOUBLE_CLICK_MS 400
#endif

/* Main-loop idle sleep. */
#ifndef TD_LOOP_SLEEP_MS
#define TD_LOOP_SLEEP_MS 5
#endif

/* Terminal-emulator view (used by the Terminal app). Cells are 4 bytes. */
#ifndef TD_VT_MAX_COLS
#define TD_VT_MAX_COLS TD_MAX_COLS
#endif
#ifndef TD_VT_MAX_ROWS
#define TD_VT_MAX_ROWS TD_MAX_ROWS
#endif
#ifndef TD_VT_SCROLLBACK
#define TD_VT_SCROLLBACK 200
#endif

#endif /* TD_CONFIG_H */
