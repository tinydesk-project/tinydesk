/*
 * td.c - initialisation and the cooperative main loop.
 *
 *   read input -> parse -> dispatch events -> run timers
 *   -> compose into the back buffer -> diff against each terminal's front buffer
 *
 * It also watches each link: when writes stop draining, frames are dropped
 * and every cell is sent again periodically. A short outage is a slow
 * terminal, so the cells are sent over the picture it shows; after a long
 * one (no terminal attached) the terminal is set up and cleared again.
 *
 * Screens: besides the port's own terminal (the primary: the serial link,
 * the USB port, the LCD) a second one can be attached, such as a Telnet
 * client (td_screen2_attach). It shows the same screen (TD_SCREENS_DUPLICATE)
 * or extends it to the right (TD_SCREENS_EXTEND): the shared screen is then
 * as wide as both and each terminal is sent its own columns. Every terminal
 * has its own input parser, frame pacing, link watch and front buffer, so a
 * slow or newly connected one never holds up the other. Pictures go to the
 * primary only; the second terminal shows what the windows drew under them.
 */
#include <stdlib.h>
#include <string.h>

#include "tinydesk/td.h"
#include "tinydesk/td_picture.h"

typedef struct
{
    bool used;
    td_hal_t hal;                 /* this terminal's read / write, the shared clock */
    td_buffer_t *front;           /* what it shows */
    td_renderer_t renderer;
    td_input_t input;
    int x0, w;                    /* the shared screen's columns it shows */
    int cols, rows;               /* its size, limited to TD_MIN..TD_MAX */
    int term_cols, term_rows;     /* the size it reported */
    bool sized;                   /* it has reported a size */
    bool resync_pending;          /* resend setup + full redraw */
    bool repaint_pending;         /* clear + full redraw (it was resized) */
    bool redraw_pending;          /* every cell again, no clear (frames were dropped) */
    bool have_input;              /* any byte ever received */
    uint32_t last_input_ms;
    uint32_t last_size_query_ms;
    uint32_t last_retry_ms;
    uint32_t link_down_ms;        /* when frames started being dropped */
    bool link_up;
    bool ack_pending;             /* ESC[5n sent after a frame, no answer yet */
    uint32_t ack_sent_ms;
    bool ack_seen;                /* it has answered at least once */
    int ack_misses;               /* timeouts before its first answer */
    bool pacing_off;              /* it never answers: do not wait for it */
    bool behind;                  /* the screen changed since its last frame */
} term_t;

static const td_hal_t *s_hal;
static td_buffer_t s_front, s_back;
static term_t s_term[2];          /* [0] the primary, [1] an attached second terminal */
static int s_mode = TD_SCREENS_DUPLICATE;
static td_stats_t s_stats;

static bool s_running;
static uint32_t s_rate_start_ms;
static uint32_t s_rate_start_bytes;
static uint32_t s_last_clock_s;
static uint32_t s_start_ms;
static uint32_t s_hold_start, s_hold_ms;

static const td_sysinfo_t s_no_sysinfo;
static const td_sysinfo_t *s_sysinfo = &s_no_sysinfo;

void td_set_sysinfo(const td_sysinfo_t *info)
{
    s_sysinfo = info ? info : &s_no_sysinfo;
}
const td_sysinfo_t *td_sysinfo(void)
{
    return s_sysinfo;
}

uint32_t td_millis(void)
{
    return s_hal ? s_hal->millis(s_hal->ctx) : 0;
}
uint32_t td_uptime_ms(void)
{
    return td_millis() - s_start_ms;
}
const td_stats_t *td_stats(void)
{
    return &s_stats;
}
void td_quit(void)
{
    s_running = false;
}
void td_full_redraw(void)
{
    for (int i = 0; i < 2; i++)
        if (s_term[i].used)
            s_term[i].resync_pending = true;
}

void td_frame_hold(uint32_t timeout_ms)
{
    s_hold_start = td_millis();
    s_hold_ms = timeout_ms;
}

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : v > hi ? hi
                                : v;
}

/* ------------------------------------------------------------- layout */

/* The shared screen's size from the terminals' sizes and the mode, and
 * which columns each terminal shows. */
static void layout(void)
{
    term_t *p = &s_term[0], *s = &s_term[1];
    int cols = p->cols, rows = p->rows, main_cols = 0;
    bool second = s->used && s->sized;
    if (second && s_mode == TD_SCREENS_EXTEND && p->cols + TD_MIN_COLS <= TD_MAX_COLS)
    {
        cols = p->cols + s->cols;
        if (cols > TD_MAX_COLS)
            cols = TD_MAX_COLS;
        rows = p->rows < s->rows ? p->rows : s->rows;
        main_cols = p->cols;
    }
    else if (second)
    {
        cols = p->cols < s->cols ? p->cols : s->cols;   /* duplicate: what both can show */
        rows = p->rows < s->rows ? p->rows : s->rows;
    }
    p->x0 = 0;
    p->w = main_cols ? main_cols : cols;
    if (s->used)
    {
        s->x0 = main_cols;
        s->w = main_cols ? cols - main_cols : cols;
    }
    td_pictures_clip(main_cols);
    if (cols != s_stats.cols || rows != s_stats.rows || main_cols != td_wm_main_cols())
    {
        s_stats.cols = cols;
        s_stats.rows = rows;
        td_buffer_init(&s_back, cols, rows);
        for (int i = 0; i < 2; i++)
            if (s_term[i].used)
            {
                td_buffer_init(s_term[i].front, cols, rows);
                s_term[i].repaint_pending = true;   /* clear stray cells outside the old area too */
            }
        td_wm_set_main_cols(main_cols);
        td_wm_set_screen_size(cols, rows);
    }
}

/* Apply a size reported by a terminal. */
static void apply_size(term_t *t, int cols, int rows)
{
    /* A terminal that changed size has usually cleared or shifted what it
     * showed, even when the size we use stays the same (the window grew
     * past TD_MAX_COLS x TD_MAX_ROWS, or shrank back to it): redraw all.
     * Only repaint, without the terminal setup: the Windows console
     * answers a repeated "alternate screen on" with a new alternate
     * screen of the old size, which undid every resize. */
    if (cols != t->term_cols || rows != t->term_rows)
    {
        t->term_cols = cols;
        t->term_rows = rows;
        t->repaint_pending = true;
    }
    t->cols = clampi(cols, TD_MIN_COLS, TD_MAX_COLS);
    t->rows = clampi(rows, TD_MIN_ROWS, TD_MAX_ROWS);
    t->sized = true;
    if (t == &s_term[0])
    {
        s_stats.term_cols = cols;
        s_stats.term_rows = rows;
    }
    layout();
}

/* ------------------------------------------------- one terminal's link */

/* Clear the terminal and send the whole screen again. */
static void repaint(term_t *t)
{
    t->renderer.write_failed = false;
    td_render_raw(&t->renderer, "\x1b[0m\x1b[2J\x1b[H");
    td_render_reset_state(&t->renderer);
    td_buffer_invalidate(t->front);
    td_wm_invalidate();
    t->repaint_pending = false;
}

/* After frames were dropped on a link that is still open: the terminal still
 * shows the last good picture, so every cell is sent again over it, without
 * clearing (a clear would flash the whole screen). CAN ends an escape
 * sequence that was cut off. */
static void redraw(term_t *t)
{
    t->renderer.write_failed = false;
    td_render_raw(&t->renderer, "\x18\x1b[0m");
    td_render_reset_state(&t->renderer);
    td_buffer_invalidate(t->front);
    td_wm_invalidate();
    t->redraw_pending = false;
}

/* Send the terminal setup and the size query, and force a full redraw. */
static void resync(term_t *t)
{
    t->renderer.write_failed = false;
    td_render_setup_terminal(&t->renderer);
    td_render_query_size(&t->renderer);
    if (t == &s_term[0])
    {
        /* Can it show pictures (Sixel in the device attributes), and how
         * big are its cells in pixels? Until it says so: no pictures. */
        td_term_set_sixel(false);
        td_pictures_forget();
        td_render_raw(&t->renderer, "\x1b[c\x1b[16t");
    }
    td_render_flush(&t->renderer);
    td_buffer_invalidate(t->front);
    td_wm_invalidate();
    t->resync_pending = false;
    t->repaint_pending = false;
    t->redraw_pending = false;
    t->last_size_query_ms = td_millis();
    t->ack_pending = false;
    t->ack_seen = false;
    t->ack_misses = 0;
    t->pacing_off = false;
}

static void term_start(term_t *t, const td_hal_t *hal)
{
    t->hal = *hal;
    td_render_init(&t->renderer, &t->hal);
    td_input_init(&t->input);
    t->link_up = true;
    t->last_input_ms = td_millis();
}

int td_init(const td_hal_t *hal)
{
    s_hal = hal;
    s_hold_ms = 0;
    s_start_ms = hal->millis(hal->ctx);
    memset(&s_stats, 0, sizeof(s_stats));
    memset(s_term, 0, sizeof(s_term));
    term_t *t = &s_term[0];
    t->used = true;
    t->front = &s_front;
    term_start(t, hal);
    td_event_clear();
    td_timers_reset();

    s_stats.cols = t->cols = TD_DEFAULT_COLS;
    s_stats.rows = t->rows = TD_DEFAULT_ROWS;
    t->w = s_stats.cols;
    td_buffer_init(&s_back, s_stats.cols, s_stats.rows);
    td_buffer_init(&s_front, s_stats.cols, s_stats.rows);
    td_wm_init(s_stats.cols, s_stats.rows);

    resync(t);
    t->link_up = s_stats.link_up = !t->renderer.write_failed;

    /* Wait briefly for the size answer; keep any other input for later. */
    uint32_t start = td_millis();
    bool answered = false;
    while (!answered && td_millis() - start < TD_SIZE_QUERY_TIMEOUT_MS)
    {
        int b;
        while ((b = hal->read_byte(hal->ctx)) >= 0)
            td_input_feed(&t->input, (uint8_t)b, td_millis());

        td_event_t pending[TD_EVENT_QUEUE_SIZE];
        int n = 0;
        td_event_t ev;
        while (td_event_pop(&ev))
        {
            if (ev.type == TD_EV_RESIZE)
            {
                apply_size(t, ev.x, ev.y);
                answered = true;
            }
            else if (n < TD_EVENT_QUEUE_SIZE)
            {
                pending[n++] = ev;
            }
        }
        for (int i = 0; i < n; i++)
            td_event_push(&pending[i]);
        if (!answered)
            hal->sleep_ms(hal->ctx, 5);
    }
    t->have_input = answered;
    t->last_input_ms = td_millis();
    s_rate_start_ms = td_millis();
    s_running = true;
    return 0;
}

/* ------------------------------------------------------ a second screen */

int td_screens_mode(void)
{
    return s_mode;
}

void td_screens_set_mode(int mode)
{
    if (mode != TD_SCREENS_DUPLICATE && mode != TD_SCREENS_EXTEND)
        return;
    s_mode = mode;
    if (s_term[1].used)
    {
        layout();
        td_full_redraw();
    }
}

bool td_screen2_attach(int (*read_byte)(void *ctx), int (*write)(void *ctx, const uint8_t *buf, int len), void *ctx)
{
    if (!s_hal || !read_byte || !write)
        return false;
    term_t *t = &s_term[1];
    if (t->used)
        td_screen2_detach();
    td_buffer_t *front = malloc(sizeof(*front));
    if (!front)
        return false;
    memset(t, 0, sizeof(*t));
    t->used = true;
    t->front = front;
    td_hal_t hal = {read_byte, write, s_hal->millis, s_hal->sleep_ms, ctx};
    term_start(t, &hal);
    /* Until it reports its size it shows the primary's screen. */
    t->cols = s_stats.cols;
    t->rows = s_stats.rows;
    td_buffer_init(front, s_stats.cols, s_stats.rows);
    t->x0 = 0;
    t->w = s_stats.cols;
    resync(t);
    return true;
}

void td_screen2_detach(void)
{
    term_t *t = &s_term[1];
    if (!t->used)
        return;
    free(t->front);
    memset(t, 0, sizeof(*t));
    layout();
    td_full_redraw();
}

bool td_screen2_attached(void)
{
    return s_term[1].used;
}

/* --------------------------------------------------------------- input */

static void dispatch_events(void)
{
    td_event_t ev;
    while (td_event_pop(&ev))
    {
        td_wm_dispatch(&ev);
        if (ev.type == TD_EV_PASTE)
        {
            for (int i = 0; i < 2; i++)
                if (s_term[i].used)
                    td_input_paste_done(&s_term[i].input);
        }
    }
}

/* The events terminal t's parser just queued: its size and frame answers
 * are its own; mouse positions move by the columns it shows. */
static void own_events(term_t *t)
{
    td_event_t keep[TD_EVENT_QUEUE_SIZE];
    int n = 0;
    td_event_t ev;
    while (td_event_pop(&ev))
    {
        if (ev.type == TD_EV_RESIZE)
            apply_size(t, ev.x, ev.y);
        else if (ev.type == TD_EV_ACK)
        {
            t->ack_pending = false;
            t->ack_seen = true;
        }
        else if (n < TD_EVENT_QUEUE_SIZE)
        {
            if (ev.type == TD_EV_MOUSE)
                ev.x += t->x0;
            keep[n++] = ev;
        }
    }
    for (int i = 0; i < n; i++)
        td_event_push(&keep[i]);
}

/* Read everything waiting on a terminal's link. Returns true if bytes arrived. */
static bool read_input(term_t *t, uint32_t now)
{
    bool got = false;
    int b;
    while ((b = t->hal.read_byte(t->hal.ctx)) >= 0)
    {
        if (!got)
        {
            /* Input after a long silence, or while frames were being
             * dropped, usually means a terminal has just been opened. */
            bool silent = t->have_input && now - t->last_input_ms > TD_RECONNECT_SILENCE_MS;
            bool lost = !t->link_up && now - t->link_down_ms >= TD_LINK_LOST_MS;
            if (silent || lost)
                t->resync_pending = true;
        }
        got = true;
        td_input_feed(&t->input, (uint8_t)b, now);
        /* A paste can hold far more keys than the queue: hand them out
         * before it fills up. */
        if (td_event_count() >= TD_EVENT_QUEUE_SIZE - 4)
        {
            own_events(t);
            dispatch_events();
        }
    }
    td_input_poll_timeouts(&t->input, now);
    own_events(t);
    if (got)
    {
        t->have_input = true;
        t->last_input_ms = now;
    }
    return got;
}

/* ------------------------------------------------ the PC's clipboard */

void td_host_clipboard_set(const char *text, int len)
{
    td_renderer_t *r = &s_term[0].renderer;
    if (!s_hal || !text || len <= 0 || TD_OSC52_MAX <= 0 || len > TD_OSC52_MAX)
        return;
    static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char chunk[68];
    int n = 0;
    td_render_raw(r, "\x1b]52;c;");
    for (int i = 0; i < len; i += 3)
    {
        uint32_t v = (uint32_t)(uint8_t)text[i] << 16;
        if (i + 1 < len)
            v |= (uint32_t)(uint8_t)text[i + 1] << 8;
        if (i + 2 < len)
            v |= (uint8_t)text[i + 2];
        chunk[n++] = B64[(v >> 18) & 63];
        chunk[n++] = B64[(v >> 12) & 63];
        chunk[n++] = i + 1 < len ? B64[(v >> 6) & 63] : '=';
        chunk[n++] = i + 2 < len ? B64[v & 63] : '=';
        if (n >= 64)
        {
            chunk[n] = '\0';
            td_render_raw(r, chunk);
            n = 0;
        }
    }
    chunk[n] = '\0';
    td_render_raw(r, chunk);
    td_render_raw(r, "\x07");
    td_render_flush(r);
}

/* -------------------------------------------------------------- frames */

static void update_rate(uint32_t now)
{
    s_stats.bytes_sent = s_term[0].renderer.bytes_sent;
    uint32_t elapsed = now - s_rate_start_ms;
    if (elapsed < 1000)
        return;
    s_stats.bytes_per_sec = (uint32_t)((uint64_t)(s_stats.bytes_sent - s_rate_start_bytes) * 1000u / elapsed);
    s_rate_start_ms = now;
    s_rate_start_bytes = s_stats.bytes_sent;
}

static void picture_write(void *user, const char *s, int n)
{
    td_render_write(user, s, n);
}

/* Send terminal t its part of the composed screen. */
static void send_frame(term_t *t)
{
    uint32_t sent_before = t->renderer.bytes_sent;
    bool primary = t == &s_term[0];
    if (primary)
        td_pictures_mark(t->front, &s_back);   /* before front catches up */
    td_render_write(&t->renderer, "\x1b[?2026h", 8);
    bool ok = td_render_diff_region(&t->renderer, t->front, &s_back, t->x0, t->w);
    if (ok)
        ok = td_render_flush(&t->renderer); /* cells/frame begin precede a local picture sink */
    if (ok)
    {
        bool moved = false;
        if (primary)
            td_pictures_send(picture_write, &t->renderer, &moved);
        td_render_write(&t->renderer, "\x1b[?2026l", 8);
        ok = td_render_flush(&t->renderer);
        if (moved)
            td_render_reset_state(&t->renderer);   /* the cursor came back with ESC 8; colours as before */
        if (!ok && primary)
            td_pictures_forget();
    }
    else
    {
        td_render_write(&t->renderer, "\x1b[?2026l", 8);
        td_render_flush(&t->renderer);
    }
    if (!ok)
    {
        /* Part of the frame never reached the terminal: resend it all
         * later, and stop trying every frame. */
        if (primary)
            s_stats.dropped_frames++;
        /* A terminal that took part of the frame is still there, only slow:
         * the outage counts from the last frame that got nothing through. */
        if (t->link_up || t->renderer.bytes_sent != sent_before)
            t->link_down_ms = td_millis();
        t->link_up = false;
        td_buffer_invalidate(t->front);
        t->last_retry_ms = td_millis();
    }
    else
    {
        /* While frames were dropped no size query went out, so the terminal
         * had nothing to answer: that silence does not mean it was replaced
         * (a terminal that was gone is caught by TD_LINK_LOST_MS). */
        if (!t->link_up)
            t->last_input_ms = td_millis();
        t->link_up = true;
        if (TD_ACK_TIMEOUT_MS > 0 && !t->pacing_off)
        {
            /* Its answer means the terminal has shown this frame. */
            td_render_raw(&t->renderer, "\x1b[5n");
            td_render_flush(&t->renderer);
            t->ack_pending = true;
            t->ack_sent_ms = td_millis();
        }
    }
    if (primary)
        s_stats.link_up = t->link_up;
}

/* True while the last frame is still on its way through the terminal.
 * Without an answer in TD_ACK_TIMEOUT_MS the next frame goes anyway; a
 * terminal that never answers stops being waited for after three. */
static bool waiting_for_terminal(term_t *t, uint32_t now)
{
    if (!t->ack_pending)
        return false;
#if TD_ACK_TIMEOUT_MS > 0
    if (now - t->ack_sent_ms < TD_ACK_TIMEOUT_MS)
        return true;
#endif
    t->ack_pending = false;
    if (!t->ack_seen && ++t->ack_misses >= 3)
        t->pacing_off = true;
    return false;
}

/* The link watch and the size poll of one terminal. */
static void watch(term_t *t, uint32_t now)
{
    if (!t->link_up)
    {
        if (now - t->last_retry_ms >= TD_LINK_RETRY_MS)
        {
            if (now - t->link_down_ms >= TD_LINK_LOST_MS)
                t->resync_pending = true;
            else
                t->redraw_pending = true;
        }
    }
    else if (TD_SIZE_POLL_MS > 0 && now - t->last_size_query_ms >= TD_SIZE_POLL_MS && !td_input_pasting(&t->input))
    {
        /* Terminals do not report resizes over a serial line; ask. Not
         * during a paste: the reply would arrive in the middle of it (the
         * parser copes, but there is no need). */
        t->renderer.write_failed = false;
        td_render_query_size(&t->renderer);
        td_render_flush(&t->renderer);
        t->last_size_query_ms = now;
    }
}

/* Whether terminal t takes a frame now, after its pending resync / repaint
 * / redraw. */
static bool due(term_t *t, uint32_t now)
{
    if (t->resync_pending)
    {
        resync(t);
        return true;
    }
    if (t->repaint_pending)
    {
        repaint(t);
        return true;
    }
    if (t->redraw_pending)
    {
        redraw(t);
        return true;
    }
    return t->link_up && t->behind && !waiting_for_terminal(t, now);
}

bool td_step(void)
{
    uint32_t now = td_millis();

    /* Each terminal's events are dispatched before the next one is read,
     * so its mouse positions and answers stay its own. */
    for (int i = 0; i < 2; i++)
        if (s_term[i].used)
        {
            read_input(&s_term[i], now);
            dispatch_events();
        }
    td_timers_run(now);

    /* The taskbar clock changes once a second. */
    uint32_t sec = now / 1000u;
    if (sec != s_last_clock_s)
    {
        s_last_clock_s = sec;
        td_wm_invalidate();
    }

    for (int i = 0; i < 2; i++)
        if (s_term[i].used)
            watch(&s_term[i], now);

    /* Even a resize or reconnect must not publish a partial batch. The
     * deadline is absolute: a dead producer cannot freeze the desktop. */
    if (s_hold_ms && td_millis() - s_hold_start < s_hold_ms)
    {
        update_rate(td_millis());
        s_stats.loop_ms = td_millis() - now;
        return s_running;
    }
    s_hold_ms = 0;

    bool changed = td_wm_needs_redraw();
    bool take[2] = {false, false};
    for (int i = 0; i < 2; i++)
        if (s_term[i].used)
        {
            s_term[i].behind |= changed;
            take[i] = due(&s_term[i], now);
        }
    if (take[0] || take[1])
    {
        /* One composition for every terminal that takes this frame; one
         * that waits catches up from its own front buffer next time. */
        uint32_t t0 = td_millis();
        td_pictures_begin();
        td_wm_compose(&s_back);
        for (int i = 0; i < 2; i++)
            if (take[i])
            {
                send_frame(&s_term[i]);
                s_term[i].behind = false;   /* one waiting for its answer stays behind: next frame */
            }
        s_stats.frames++;
        s_stats.frame_ms = td_millis() - t0;
    }

    update_rate(td_millis());
    s_stats.loop_ms = td_millis() - now;
    return s_running;
}

void td_run(void)
{
    while (td_step())
        s_hal->sleep_ms(s_hal->ctx, TD_LOOP_SLEEP_MS);
}

void td_shutdown(void)
{
    if (!s_hal)
        return;
    for (int i = 1; i >= 0; i--)
        if (s_term[i].used)
        {
            s_term[i].renderer.write_failed = false;
            td_render_restore_terminal(&s_term[i].renderer);
        }
}
