/*
 * td.c - initialisation and the cooperative main loop.
 *
 *   read input -> parse -> dispatch events -> run timers
 *   -> compose into the back buffer -> diff against the front buffer
 *
 * It also watches the serial link: when writes stop draining, frames are
 * dropped and every cell is sent again periodically. A short outage is a
 * slow terminal, so the cells are sent over the picture it shows; after a
 * long one (no terminal attached) the terminal is set up and cleared again.
 */
#include <string.h>

#include "tinydesk/td.h"
#include "tinydesk/td_picture.h"

static const td_hal_t *s_hal;
static td_buffer_t s_front, s_back;
static td_renderer_t s_renderer;
static td_input_t s_input;
static td_stats_t s_stats;

static bool s_running;
static bool s_resync_pending;     /* resend setup + full redraw */
static bool s_repaint_pending;    /* clear + full redraw (the terminal was resized) */
static bool s_redraw_pending;     /* every cell again, no clear (frames were dropped) */
static bool s_have_input;         /* any byte ever received */
static uint32_t s_last_input_ms;
static uint32_t s_last_size_query_ms;
static uint32_t s_last_retry_ms;
static uint32_t s_link_down_ms;   /* when frames started being dropped */
static bool s_ack_pending;        /* ESC[5n sent after a frame, no answer yet */
static uint32_t s_ack_sent_ms;
static bool s_ack_seen;           /* this terminal has answered at least once */
static int s_ack_misses;          /* timeouts before its first answer */
static bool s_pacing_off;         /* it never answers: do not wait for it */
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
    s_resync_pending = true;
}

void td_frame_hold(uint32_t timeout_ms)
{
    s_hold_start = td_millis();
    s_hold_ms = timeout_ms;
}

/* Apply a size reported by the terminal. */
static void apply_size(int cols, int rows)
{
    /* A terminal that changed size has usually cleared or shifted what it
     * showed, even when the size we use stays the same (the window grew
     * past TD_MAX_COLS x TD_MAX_ROWS, or shrank back to it): redraw all.
     * Only repaint, without the terminal setup: the Windows console
     * answers a repeated "alternate screen on" with a new alternate
     * screen of the old size, which undid every resize. */
    if (cols != s_stats.term_cols || rows != s_stats.term_rows)
    {
        s_stats.term_cols = cols;
        s_stats.term_rows = rows;
        s_repaint_pending = true;
    }
    if (cols < TD_MIN_COLS)
        cols = TD_MIN_COLS;
    if (rows < TD_MIN_ROWS)
        rows = TD_MIN_ROWS;
    if (cols > TD_MAX_COLS)
        cols = TD_MAX_COLS;
    if (rows > TD_MAX_ROWS)
        rows = TD_MAX_ROWS;
    if (cols == s_stats.cols && rows == s_stats.rows)
        return;

    s_stats.cols = cols;
    s_stats.rows = rows;
    td_buffer_init(&s_back, cols, rows);
    td_buffer_init(&s_front, cols, rows);
    td_wm_set_screen_size(cols, rows);
    /* Clear stray cells outside the old area too. */
    s_repaint_pending = true;
}

/* Clear the terminal and send the whole screen again. */
static void repaint(void)
{
    s_renderer.write_failed = false;
    td_render_raw(&s_renderer, "\x1b[0m\x1b[2J\x1b[H");
    td_render_reset_state(&s_renderer);
    td_buffer_invalidate(&s_front);
    td_wm_invalidate();
    s_repaint_pending = false;
}

/* After frames were dropped on a link that is still open: the terminal still
 * shows the last good picture, so every cell is sent again over it, without
 * clearing (a clear would flash the whole screen). CAN ends an escape
 * sequence that was cut off. */
static void redraw(void)
{
    s_renderer.write_failed = false;
    td_render_raw(&s_renderer, "\x18\x1b[0m");
    td_render_reset_state(&s_renderer);
    td_buffer_invalidate(&s_front);
    td_wm_invalidate();
    s_redraw_pending = false;
}

/* Send the terminal setup and the size query, and force a full redraw. */
static void resync(void)
{
    s_renderer.write_failed = false;
    td_render_setup_terminal(&s_renderer);
    td_render_query_size(&s_renderer);
    /* Can it show pictures (Sixel in the device attributes), and how big
     * are its cells in pixels? Until it says so: no pictures. */
    td_term_set_sixel(false);
    td_pictures_forget();
    td_render_raw(&s_renderer, "\x1b[c\x1b[16t");
    td_render_flush(&s_renderer);
    td_buffer_invalidate(&s_front);
    td_wm_invalidate();
    s_resync_pending = false;
    s_repaint_pending = false;
    s_redraw_pending = false;
    s_last_size_query_ms = td_millis();
    s_ack_pending = false;
    s_ack_seen = false;
    s_ack_misses = 0;
    s_pacing_off = false;
}

int td_init(const td_hal_t *hal)
{
    s_hal = hal;
    s_hold_ms = 0;
    s_start_ms = hal->millis(hal->ctx);
    memset(&s_stats, 0, sizeof(s_stats));
    td_render_init(&s_renderer, hal);
    td_input_init(&s_input);
    td_event_clear();
    td_timers_reset();

    s_stats.cols = TD_DEFAULT_COLS;
    s_stats.rows = TD_DEFAULT_ROWS;
    td_buffer_init(&s_back, s_stats.cols, s_stats.rows);
    td_buffer_init(&s_front, s_stats.cols, s_stats.rows);
    td_wm_init(s_stats.cols, s_stats.rows);

    resync();
    s_stats.link_up = !s_renderer.write_failed;

    /* Wait briefly for the size answer; keep any other input for later. */
    uint32_t start = td_millis();
    bool answered = false;
    while (!answered && td_millis() - start < TD_SIZE_QUERY_TIMEOUT_MS)
    {
        int b;
        while ((b = hal->read_byte(hal->ctx)) >= 0)
            td_input_feed(&s_input, (uint8_t)b, td_millis());

        td_event_t pending[TD_EVENT_QUEUE_SIZE];
        int n = 0;
        td_event_t ev;
        while (td_event_pop(&ev))
        {
            if (ev.type == TD_EV_RESIZE)
            {
                apply_size(ev.x, ev.y);
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
    s_have_input = answered;
    s_last_input_ms = td_millis();
    s_rate_start_ms = td_millis();
    s_running = true;
    return 0;
}

static void dispatch_events(void)
{
    td_event_t ev;
    while (td_event_pop(&ev))
    {
        if (ev.type == TD_EV_RESIZE)
        {
            apply_size(ev.x, ev.y);
        }
        else if (ev.type == TD_EV_ACK)
        {
            s_ack_pending = false;
            s_ack_seen = true;
        }
        else
        {
            td_wm_dispatch(&ev);
            if (ev.type == TD_EV_PASTE)
                td_input_paste_done(&s_input);
        }
    }
}

/* ------------------------------------------------ the PC's clipboard */

void td_host_clipboard_set(const char *text, int len)
{
    if (!s_hal || !text || len <= 0 || TD_OSC52_MAX <= 0 || len > TD_OSC52_MAX)
        return;
    static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char chunk[68];
    int n = 0;
    td_render_raw(&s_renderer, "\x1b]52;c;");
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
            td_render_raw(&s_renderer, chunk);
            n = 0;
        }
    }
    chunk[n] = '\0';
    td_render_raw(&s_renderer, chunk);
    td_render_raw(&s_renderer, "\x07");
    td_render_flush(&s_renderer);
}

/* Read everything waiting on the link. Returns true if bytes arrived. */
static bool read_input(uint32_t now)
{
    bool got = false;
    int b;
    while ((b = s_hal->read_byte(s_hal->ctx)) >= 0)
    {
        if (!got)
        {
            /* Input after a long silence, or while frames were being
             * dropped, usually means a terminal has just been opened. */
            bool silent = s_have_input && now - s_last_input_ms > TD_RECONNECT_SILENCE_MS;
            bool lost = !s_stats.link_up && now - s_link_down_ms >= TD_LINK_LOST_MS;
            if (silent || lost)
                s_resync_pending = true;
        }
        got = true;
        td_input_feed(&s_input, (uint8_t)b, now);
        /* A paste can hold far more keys than the queue: hand them out
         * before it fills up. */
        if (td_event_count() >= TD_EVENT_QUEUE_SIZE - 4)
            dispatch_events();
    }
    if (got)
    {
        s_have_input = true;
        s_last_input_ms = now;
    }
    return got;
}

static void update_rate(uint32_t now)
{
    s_stats.bytes_sent = s_renderer.bytes_sent;
    uint32_t elapsed = now - s_rate_start_ms;
    if (elapsed < 1000)
        return;
    s_stats.bytes_per_sec = (uint32_t)((uint64_t)(s_renderer.bytes_sent - s_rate_start_bytes) * 1000u / elapsed);
    s_rate_start_ms = now;
    s_rate_start_bytes = s_renderer.bytes_sent;
}

static void picture_write(void *user, const char *s, int n)
{
    td_render_write(user, s, n);
}

static void render_frame(void)
{
    uint32_t t0 = td_millis();
    uint32_t sent_before = s_renderer.bytes_sent;
    td_pictures_begin();
    td_wm_compose(&s_back);
    td_pictures_mark(&s_front, &s_back);   /* before front catches up */
    td_render_write(&s_renderer, "\x1b[?2026h", 8);
    bool ok = td_render_diff(&s_renderer, &s_front, &s_back);
    if (ok)
        ok = td_render_flush(&s_renderer); /* cells/frame begin precede a local picture sink */
    if (ok)
    {
        bool moved = false;
        td_pictures_send(picture_write, &s_renderer, &moved);
        td_render_write(&s_renderer, "\x1b[?2026l", 8);
        ok = td_render_flush(&s_renderer);
        if (moved)
            td_render_reset_state(&s_renderer);   /* the cursor came back with ESC 8; colours as before */
        if (!ok)
            td_pictures_forget();
    }
    else
    {
        td_render_write(&s_renderer, "\x1b[?2026l", 8);
        td_render_flush(&s_renderer);
    }
    s_stats.frames++;
    s_stats.frame_ms = td_millis() - t0;
    if (!ok)
    {
        /* Part of the frame never reached the terminal: resend it all
         * later, and stop trying every frame. */
        s_stats.dropped_frames++;
        /* A terminal that took part of the frame is still there, only slow:
         * the outage counts from the last frame that got nothing through. */
        if (s_stats.link_up || s_renderer.bytes_sent != sent_before)
            s_link_down_ms = td_millis();
        s_stats.link_up = false;
        td_buffer_invalidate(&s_front);
        s_last_retry_ms = td_millis();
    }
    else
    {
        /* While frames were dropped no size query went out, so the terminal
         * had nothing to answer: that silence does not mean it was replaced
         * (a terminal that was gone is caught by TD_LINK_LOST_MS). */
        if (!s_stats.link_up)
            s_last_input_ms = td_millis();
        s_stats.link_up = true;
        if (TD_ACK_TIMEOUT_MS > 0 && !s_pacing_off)
        {
            /* Its answer means the terminal has shown this frame. */
            td_render_raw(&s_renderer, "\x1b[5n");
            td_render_flush(&s_renderer);
            s_ack_pending = true;
            s_ack_sent_ms = td_millis();
        }
    }
}

/* True while the last frame is still on its way through the terminal.
 * Without an answer in TD_ACK_TIMEOUT_MS the next frame goes anyway; a
 * terminal that never answers stops being waited for after three. */
static bool waiting_for_terminal(uint32_t now)
{
    if (!s_ack_pending)
        return false;
#if TD_ACK_TIMEOUT_MS > 0
    if (now - s_ack_sent_ms < TD_ACK_TIMEOUT_MS)
        return true;
#endif
    s_ack_pending = false;
    if (!s_ack_seen && ++s_ack_misses >= 3)
        s_pacing_off = true;
    return false;
}

bool td_step(void)
{
    uint32_t now = td_millis();

    read_input(now);
    td_input_poll_timeouts(&s_input, now);
    dispatch_events();
    td_timers_run(now);

    /* The taskbar clock changes once a second. */
    uint32_t sec = now / 1000u;
    if (sec != s_last_clock_s)
    {
        s_last_clock_s = sec;
        td_wm_invalidate();
    }

    if (!s_stats.link_up)
    {
        if (now - s_last_retry_ms >= TD_LINK_RETRY_MS)
        {
            if (now - s_link_down_ms >= TD_LINK_LOST_MS)
                s_resync_pending = true;
            else
                s_redraw_pending = true;
        }
    }
    else if (TD_SIZE_POLL_MS > 0 && now - s_last_size_query_ms >= TD_SIZE_POLL_MS &&
             !td_input_pasting(&s_input))
    {
        /* Terminals do not report resizes over a serial line; ask. Not
         * during a paste: the reply would arrive in the middle of it (the
         * parser copes, but there is no need). */
        s_renderer.write_failed = false;
        td_render_query_size(&s_renderer);
        td_render_flush(&s_renderer);
        s_last_size_query_ms = now;
    }

    /* Even a resize or reconnect must not publish a partial batch. The
     * deadline is absolute: a dead producer cannot freeze the desktop. */
    if (s_hold_ms && td_millis() - s_hold_start < s_hold_ms)
    {
        update_rate(td_millis());
        s_stats.loop_ms = td_millis() - now;
        return s_running;
    }
    s_hold_ms = 0;

    if (s_resync_pending)
    {
        resync();
        render_frame();
    }
    else if (s_repaint_pending)
    {
        repaint();
        render_frame();
    }
    else if (s_redraw_pending)
    {
        redraw();
        render_frame();
    }
    else if (s_stats.link_up && td_wm_needs_redraw() && !waiting_for_terminal(now))
    {
        render_frame();
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
    s_renderer.write_failed = false;
    td_render_restore_terminal(&s_renderer);
}
