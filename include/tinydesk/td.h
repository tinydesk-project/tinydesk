/*
 * td.h - tinydesk umbrella header and main-loop API.
 *
 *     static td_hal_t hal = { my_read, my_write, my_millis, my_sleep, NULL };
 *     td_init(&hal);
 *     td_apps_register_all();   // optional demo apps
 *     td_run();                 // returns after td_quit()
 *     td_shutdown();
 */
#ifndef TD_H
#define TD_H

#include "td_canvas.h"
#include "td_config.h"
#include "td_hal.h"
#include "td_input.h"
#include "td_screen.h"
#include "td_sysinfo.h"
#include "td_widgets.h"
#include "td_wm.h"

  /* Put text on the clipboard of the PC running the terminal (OSC 52; only
 * terminals that support it act on it). At most TD_OSC52_MAX bytes. */
void td_host_clipboard_set(const char *text, int len);

#define TD_VERSION        "0.2.0"
#define TD_REPO_URL       "https://github.com/tinydesk-project/tinydesk"
#define TD_SHELL_REPO_URL "https://github.com/tinydesk-project/tinydesk-shell"

  /* Runtime statistics (shown by System Monitor). */
typedef struct
{
    int cols, rows;              /* screen size in use */
    int term_cols, term_rows;    /* size the terminal reported (may exceed TD_MAX_COLS x TD_MAX_ROWS) */
    uint32_t frames;             /* frames rendered */
    uint32_t frame_ms;           /* duration of the last compose + render */
    uint32_t loop_ms;            /* duration of the last main-loop pass */
    uint32_t bytes_sent;         /* total bytes written to the terminal */
    uint32_t bytes_per_sec;      /* over the last second */
    uint32_t dropped_frames;     /* frames the link did not accept */
    bool link_up;                /* false while writes are being dropped */
} td_stats_t;

  /* Set up the screen, send the terminal setup sequence and detect the
 * terminal size (waits up to TD_SIZE_QUERY_TIMEOUT_MS). Returns 0. */
int td_init(const td_hal_t *hal);

  /* Run the main loop until td_quit() is called. */
void td_run(void);

  /* One pass of the main loop, for embedding in another loop. Returns false
 * once td_quit() has been called. Does not sleep. */
bool td_step(void);

  /* Ask td_run() to return. */
void td_quit(void);

  /* Restore the terminal (leave the alternate screen, show the cursor,
 * disable mouse reporting). */
void td_shutdown(void);

  /* Resend the terminal setup and redraw everything on the next frame. */
void td_full_redraw(void);

  /* A second display: another terminal, such as a Telnet client or a
 * terminal on a second serial port, attached beside the primary one (the
 * HAL's). It shows the same screen (TD_SCREENS_DUPLICATE: the desktop
 * takes the smaller of the two sizes) or extends it (TD_SCREENS_EXTEND:
 * display 2 is beside display 1, where the layout says). Each terminal
 * has its own input, frame pacing and link watch. read_byte / write behave
 * as the HAL's. Pictures go to the primary only. Attaching takes one
 * screen buffer from the heap (TD_MAX_COLS x TD_MAX_ROWS x 8 bytes). */
 /* (The types are in td_screen.h.) */
bool td_screen2_attach(int (*read_byte)(void *ctx), int (*write)(void *ctx, const uint8_t *buf, int len), void *ctx);
void td_screen2_detach(void);
bool td_screen2_attached(void);
void td_screens_set_layout(const td_screens_layout_t *layout);
void td_screens_layout(td_screens_layout_t *out);
bool td_screens_display(int index, td_display_info_t *out);   /* false: no such display */
void td_screens_set_mode(int mode);   /* the layout with only the mode changed */
int td_screens_mode(void);

  /* Desktop loop only: defer every frame for at most timeout_ms (0 releases
 * it). Input and timers keep running. Used for bounded drawing batches. */
void td_frame_hold(uint32_t timeout_ms);

  /* Milliseconds from the HAL. */
uint32_t td_millis(void);

  /* Milliseconds since td_init(). */
uint32_t td_uptime_ms(void);

  /* Current statistics. */
const td_stats_t *td_stats(void);

#endif /* TD_H */
