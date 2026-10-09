/*
 * test_link.c - the main loop's link handling: frames dropped by a slow
 * terminal are sent again without clearing the screen (no flicker); only a
 * long outage, a terminal that was gone, sets the terminal up and clears it.
 * Frame pacing: the next frame waits for the terminal's answer to ESC[5n.
 */
#include "td_test.h"
#include "tinydesk/td.h"

static uint32_t s_now = 1000;
static int s_accept = 1 << 30; /* bytes the fake link still takes */
static char s_out[200000];
static int s_len;
static const char *s_in = "";
static bool s_answering = true; /* the fake terminal answers ESC[5n */
static int s_answers;           /* answers it still has to send */
static int s_answer_pos;

static int fake_read(void *ctx)
{
    (void)ctx;
    static const char answer[] = "\x1b[0n";
    if (s_answers > 0)
    {
        int c = (unsigned char)answer[s_answer_pos++];
        if (!answer[s_answer_pos])
        {
            s_answer_pos = 0;
            s_answers--;
        }
        return c;
    }
    return *s_in ? (unsigned char)*s_in++ : -1;
}
static int fake_write(void *ctx, const uint8_t *buf, int len)
{
    (void)ctx;
    if (len > s_accept)
        len = s_accept;
    s_accept -= len;
    if (s_len + len < (int)sizeof(s_out))
        memcpy(s_out + s_len, buf, (size_t)len);
    s_len += len;
    for (int i = 0; i + 4 <= len; ++i)
    {
        if (s_answering && memcmp(buf + i, "\x1b[5n", 4) == 0)
            s_answers++;
    }
    return len;
}
static uint32_t fake_millis(void *ctx)
{
    (void)ctx;
    return s_now;
}
static void fake_sleep(void *ctx, uint32_t ms)
{
    (void)ctx;
    s_now += ms;
}
static const td_hal_t s_hal = {fake_read, fake_write, fake_millis, fake_sleep, NULL};

static void clear_output(void)
{
    s_len = 0;
}
static bool sent(const char *seq)
{
    s_out[s_len < (int)sizeof(s_out) ? s_len : (int)sizeof(s_out) - 1] = '\0';
    return strstr(s_out, seq) != NULL;
}

/* A new picture of the test window went out. */
static bool frame_sent(void)
{
    return sent("oooooooo") || sent("XXXXXXXX");
}

/* One loop pass, ms later. */
static void step(uint32_t ms)
{
    s_now += ms;
    td_step();
}

/* A window whose text changes every frame, so every step has cells to send. */
static td_window_t *s_win;
static unsigned s_frame;
static void on_draw(td_window_t *win, int cw, int ch)
{
    (void)win;
    (void)ch;
    char text[32];
    /* Every character changes from one frame to the next, so every frame
     * sends the whole word (the renderer sends only changed cells). */
    snprintf(text, sizeof(text), "%s", ++s_frame % 2 ? "oooooooo" : "XXXXXXXX");
    td_textn(0, 0, text, cw, 7, 0, 0);
}

/* Make the next frame fail: the link takes only a few bytes. */
static void drop_frame(void)
{
    s_accept = 10;
    td_win_invalidate(s_win);
    step(1);
    s_accept = 1 << 30;
}

int main(void)
{
    s_in = "\x1b[25;80R"; /* the terminal answers the size query: 80x25 */
    td_init(&s_hal);
    td_window_desc_t d = {.title = "Link", .rect = td_rect(2, 2, 30, 6), .on_draw = on_draw};
    s_win = td_win_create(&d);
    CHECK(s_win != NULL);
    step(10);
    CHECK(td_stats()->link_up);

    /* A slow terminal: one frame dropped, then every cell again without a
     * clear, and the link is up again. */
    uint32_t dropped = td_stats()->dropped_frames;
    drop_frame();
    CHECK_EQ(td_stats()->dropped_frames, dropped + 1);
    CHECK(!td_stats()->link_up);
    clear_output();
    step(TD_LINK_RETRY_MS);
    CHECK(td_stats()->link_up);
    CHECK(sent("\x18\x1b[0m")); /* ends a cut-off sequence */
    CHECK(!sent("\x1b[2J"));    /* no clear: no flicker */
    CHECK(!sent("\x1b[?1049h"));
    CHECK(s_len > 80 * 25);     /* every cell again */

    /* Input during a short outage (a mouse move over PuTTY) does not clear
     * either. */
    drop_frame();
    s_in = "\x1b[<35;10;10M";
    clear_output();
    step(TD_LINK_RETRY_MS);
    CHECK(td_stats()->link_up);
    CHECK(!sent("\x1b[2J"));

    /* A terminal that stays slow for longer than TD_LINK_LOST_MS: every
     * retry gets part of the screen through, so it is never treated as gone
     * and never cleared. */
    clear_output();
    for (uint32_t t = 0; t <= 2 * TD_LINK_LOST_MS; t += TD_LINK_RETRY_MS)
    {
        s_accept = 5; /* a few bytes: the terminal is reading */
        td_win_invalidate(s_win);
        step(TD_LINK_RETRY_MS);
    }
    CHECK(!td_stats()->link_up);
    s_accept = 1 << 30;
    step(TD_LINK_RETRY_MS);
    CHECK(td_stats()->link_up);
    CHECK(!sent("\x1b[2J"));
    CHECK(!sent("\x1b[?1049h"));
    /* Its first input after that (the answer to a size query) comes after
     * more than TD_RECONNECT_SILENCE_MS without input, but the terminal was
     * there all along: still no clear. */
    s_in = "\x1b[25;80R"; /* the size it has: not a resize */
    step(1);
    CHECK(!sent("\x1b[2J"));

    /* A long outage (no terminal reading: nothing gets through): the terminal
     * is set up again and cleared once it reads again. */
    s_accept = 0;
    td_win_invalidate(s_win);
    for (uint32_t t = 0; t <= TD_LINK_LOST_MS; t += TD_LINK_RETRY_MS)
        step(TD_LINK_RETRY_MS);
    CHECK(!td_stats()->link_up);
    s_accept = 1 << 30;
    clear_output();
    step(TD_LINK_RETRY_MS);
    CHECK(td_stats()->link_up);
    CHECK(sent("\x1b[?1049h"));
    CHECK(sent("\x1b[2J"));

    /* Input after a long outage (a terminal just opened): set up and cleared. */
    s_accept = 0;
    td_win_invalidate(s_win);
    for (uint32_t t = 0; t <= TD_LINK_LOST_MS; t += TD_LINK_RETRY_MS)
        step(TD_LINK_RETRY_MS);
    s_accept = 1 << 30;
    s_in = "a";
    clear_output();
    step(1);
    CHECK(sent("\x1b[2J"));

    /* Frame pacing: after a frame the terminal is asked ESC[5n, and the
     * next frame waits for its answer. */
    step(10);
    s_answering = false;
    td_win_invalidate(s_win);
    clear_output();
    step(1);
    CHECK(frame_sent());
    CHECK(sent("\x1b[5n"));
    td_win_invalidate(s_win);
    clear_output();
    step(10);
    CHECK(!frame_sent()); /* waiting for the answer */
    s_answers = 1;            /* the terminal has shown the frame */
    step(1);
    CHECK(frame_sent());  /* the next frame goes at once */

    /* A terminal that never answers: the next frame goes after
     * TD_ACK_TIMEOUT_MS; after three unanswered questions it is no longer
     * waited for (a new terminal set up with td_full_redraw starts afresh). */
    td_full_redraw();
    step(1);
    for (int i = 0; i < 3; ++i)
    {
        td_win_invalidate(s_win);
        clear_output();
        step(TD_ACK_TIMEOUT_MS / 2);
        CHECK(!frame_sent());
        step(TD_ACK_TIMEOUT_MS / 2);
        CHECK(frame_sent());
    }
    CHECK(!sent("\x1b[5n")); /* pacing is off */
    td_win_invalidate(s_win);
    clear_output();
    step(1);
    CHECK(frame_sent()); /* no waiting */

    return TD_TEST_RESULT();
}
