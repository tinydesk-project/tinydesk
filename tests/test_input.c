/*
 * test_input.c - every sequence from spec section 8, fed whole and split
 * byte by byte across separate calls.
 */
#include "td_test.h"
#include "tinydesk/td_input.h"

static td_input_t s_p;
static uint32_t s_now = 1000;

/* Feed a string; `split` feeds each byte as its own call with time
 * advancing a little (as separate reads would). */
static void feed(const char *s, int len, bool split)
{
    for (int i = 0; i < len; i++)
    {
        td_input_feed(&s_p, (uint8_t)s[i], s_now);
        if (split)
            s_now += 1;
    }
    td_input_poll_timeouts(&s_p, s_now);
}

#define FEED(lit, split) feed(lit, (int)sizeof(lit) - 1, split)

static bool pop(td_event_t *ev)
{
    return td_event_pop(ev);
}

static void expect_key(const char *seq, int len, uint32_t key, uint8_t mods)
{
    for (int split = 0; split < 2; split++)
    {
        td_event_clear();
        td_input_init(&s_p);
        feed(seq, len, split != 0);
        td_event_t ev;
        bool got = pop(&ev);
        CHECK(got);
        if (!got)
        {
            printf("  no event for sequence (split=%d)\n", split);
            continue;
        }
        CHECK_EQ(ev.type, TD_EV_KEY);
        if (ev.key != key || ev.mods != mods)
            printf("  sequence gave key %#x mods %u (split=%d)\n", (unsigned)ev.key, ev.mods, split);
        CHECK_EQ(ev.key, key);
        CHECK_EQ(ev.mods, mods);
        CHECK(!pop(&ev));
    }
}

#define KEY(lit, key, mods) expect_key(lit, (int)sizeof(lit) - 1, key, mods)

static void test_keys(void)
{
    KEY("a", 'a', 0);
    KEY("\xC3\xA9", 0xE9, 0);
    KEY("\r", TD_KEY_ENTER, 0);
    KEY("\t", TD_KEY_TAB, 0);
    KEY("\x7f", TD_KEY_BACKSPACE, 0);
    KEY("\x08", TD_KEY_BACKSPACE, 0);
    KEY("\x01", 'a', TD_MOD_CTRL);
    KEY("\x1a", 'z', TD_MOD_CTRL);

    KEY("\x1b[A", TD_KEY_UP, 0);
    KEY("\x1b[B", TD_KEY_DOWN, 0);
    KEY("\x1b[C", TD_KEY_RIGHT, 0);
    KEY("\x1b[D", TD_KEY_LEFT, 0);
    KEY("\x1bOA", TD_KEY_UP, 0);
    KEY("\x1bOD", TD_KEY_LEFT, 0);

    KEY("\x1b[H", TD_KEY_HOME, 0);
    KEY("\x1b[F", TD_KEY_END, 0);
    KEY("\x1b[1~", TD_KEY_HOME, 0);
    KEY("\x1b[4~", TD_KEY_END, 0);
    KEY("\x1b[2~", TD_KEY_INSERT, 0);
    KEY("\x1b[3~", TD_KEY_DELETE, 0);
    KEY("\x1b[5~", TD_KEY_PGUP, 0);
    KEY("\x1b[6~", TD_KEY_PGDN, 0);

    KEY("\x1bOP", TD_KEY_F1, 0);
    KEY("\x1bOQ", TD_KEY_F2, 0);
    KEY("\x1bOR", TD_KEY_F3, 0);
    KEY("\x1bOS", TD_KEY_F4, 0);
    KEY("\x1b[15~", TD_KEY_F5, 0);
    KEY("\x1b[17~", TD_KEY_F6, 0);
    KEY("\x1b[18~", TD_KEY_F7, 0);
    KEY("\x1b[19~", TD_KEY_F8, 0);
    KEY("\x1b[20~", TD_KEY_F9, 0);
    KEY("\x1b[21~", TD_KEY_F10, 0);
    KEY("\x1b[23~", TD_KEY_F11, 0);
    KEY("\x1b[24~", TD_KEY_F12, 0);

    KEY("\x1b[1;5A", TD_KEY_UP, TD_MOD_CTRL);
    KEY("\x1b[1;2D", TD_KEY_LEFT, TD_MOD_SHIFT);
    KEY("\x1b[1;3C", TD_KEY_RIGHT, TD_MOD_ALT);
    KEY("\x1b[1;6B", TD_KEY_DOWN, TD_MOD_CTRL | TD_MOD_SHIFT);
    KEY("\x1b[5;5~", TD_KEY_PGUP, TD_MOD_CTRL);
    KEY("\x1b[Z", TD_KEY_TAB, TD_MOD_SHIFT);

    KEY("\x1bx", 'x', TD_MOD_ALT);
}

static void test_lone_esc(void)
{
    td_event_clear();
    td_input_init(&s_p);
    td_input_feed(&s_p, 0x1B, 5000);
    td_input_poll_timeouts(&s_p, 5010);
    td_event_t ev;
    CHECK(!pop(&ev));                        /* too early */
    td_input_poll_timeouts(&s_p, 5000 + TD_ESC_TIMEOUT_MS);
    CHECK(pop(&ev));
    CHECK_EQ(ev.key, TD_KEY_ESC);
}

static void test_mouse(void)
{
    for (int split = 0; split < 2; split++)
    {
        td_event_clear();
        td_input_init(&s_p);
        td_event_t ev;

        FEED("\x1b[<0;10;5M", split);        /* left press at (10,5) 1-based */
        CHECK(pop(&ev));
        CHECK_EQ(ev.type, TD_EV_MOUSE);
        CHECK_EQ(ev.button, TD_BUTTON_LEFT);
        CHECK_EQ(ev.action, TD_MOUSE_PRESS);
        CHECK_EQ(ev.x, 9);
        CHECK_EQ(ev.y, 4);

        FEED("\x1b[<32;12;6M", split);       /* drag with left held */
        CHECK(pop(&ev));
        CHECK_EQ(ev.action, TD_MOUSE_DRAG);
        CHECK_EQ(ev.x, 11);

        FEED("\x1b[<0;12;6m", split);        /* release */
        CHECK(pop(&ev));
        CHECK_EQ(ev.action, TD_MOUSE_RELEASE);

        FEED("\x1b[<2;1;1M", split);         /* right press */
        CHECK(pop(&ev));
        CHECK_EQ(ev.button, TD_BUTTON_RIGHT);
        CHECK_EQ(ev.x, 0);

        FEED("\x1b[<64;3;3M", split);        /* wheel up */
        CHECK(pop(&ev));
        CHECK_EQ(ev.button, TD_BUTTON_WHEEL_UP);
        FEED("\x1b[<65;3;3M", split);        /* wheel down */
        CHECK(pop(&ev));
        CHECK_EQ(ev.button, TD_BUTTON_WHEEL_DOWN);

        FEED("\x1b[<16;3;3M", split);        /* ctrl + left */
        CHECK(pop(&ev));
        CHECK_EQ(ev.mods, TD_MOD_CTRL);

        /* Legacy X10: ESC [ M b x y with 32 added to each. */
        FEED("\x1b[M\x20\x25\x23", split);
        CHECK(pop(&ev));
        CHECK_EQ(ev.button, TD_BUTTON_LEFT);
        CHECK_EQ(ev.x, 4);
        CHECK_EQ(ev.y, 2);
        CHECK(!pop(&ev));
    }
}

static void test_drag_merging(void)
{
    td_event_clear();
    td_input_init(&s_p);
    FEED("\x1b[<32;2;2M\x1b[<32;3;2M\x1b[<32;4;2M", false);
    td_event_t ev;
    CHECK(pop(&ev));
    CHECK_EQ(ev.x, 3);                       /* only the latest position */
    CHECK(!pop(&ev));
}

static void test_cursor_report(void)
{
    td_event_clear();
    td_input_init(&s_p);
    FEED("\x1b[25;80R", true);
    td_event_t ev;
    CHECK(pop(&ev));
    CHECK_EQ(ev.type, TD_EV_RESIZE);
    CHECK_EQ(ev.x, 80);
    CHECK_EQ(ev.y, 25);
}

/* Feed a whole paste and check the text that arrives with TD_EV_PASTE. */
static void expect_paste(const char *seq, int len, bool split, const char *text)
{
    feed(seq, len, split);
    td_event_t ev;
    bool got = false;
    while (pop(&ev))
    {
        if (ev.type != TD_EV_PASTE)
            continue;
        got = true;
        int n;
        const char *s = td_paste_text(&n);
        CHECK(s != NULL);
        CHECK_EQ(n, (int)strlen(text));
        if (s && (n != (int)strlen(text) || memcmp(s, text, (size_t)n) != 0))
            printf("  paste gave \"%.*s\" (split=%d)\n", n, s, split);
        CHECK(s && memcmp(s, text, strlen(text)) == 0);
        td_input_paste_done(&s_p);
    }
    CHECK(got);
}

#define PASTE(lit, split, text) expect_paste(lit, (int)sizeof(lit) - 1, split, text)

static void test_paste(void)
{
    for (int split = 0; split < 2; split++)
    {
        td_event_clear();
        td_input_init(&s_p);
        PASTE("[200~gr[25;80Row[201~", split != 0, "grow");

        /* The size reply in the middle of the paste still takes effect. */
        td_event_clear();
        td_input_init(&s_p);
        FEED("[200~gr[25;80Row", split != 0);
        td_event_t ev;
        CHECK(pop(&ev));
        CHECK_EQ(ev.type, TD_EV_RESIZE);
        CHECK_EQ(ev.x, 80);
        CHECK_EQ(ev.y, 25);
        CHECK(!pop(&ev));
        PASTE("[201~", split != 0, "grow");

        /* Things that only look like the end marker or a report are text. */
        td_event_clear();
        td_input_init(&s_p);
        PASTE("[200~a[20x[25;8[201~", split != 0,
              "a[20x[25;8");
    }
}

static void test_garbage(void)
{
    /* Unknown / private / overlong sequences are dropped and parsing
     * continues normally afterwards. */
    td_event_clear();
    td_input_init(&s_p);
    FEED("\x1b[?1;2c", false);
    FEED("\x1b[99~", false);
    FEED("\x1b[1;2;3;4;5;6;7;8;9;10;11;12;13;14;15;16;17;18;19;20q", false);
    FEED("\x1b[A", false);
    td_event_t ev;
    CHECK(pop(&ev));
    CHECK_EQ(ev.key, TD_KEY_UP);
    CHECK(!pop(&ev));

    /* A sequence interrupted by a new ESC is abandoned. */
    FEED("\x1b[1;\x1b[B", false);
    CHECK(pop(&ev));
    CHECK_EQ(ev.key, TD_KEY_DOWN);
    CHECK(!pop(&ev));

    /* A stalled sequence times out without producing anything. */
    td_input_feed(&s_p, 0x1B, 100);
    td_input_feed(&s_p, '[', 100);
    td_input_feed(&s_p, '1', 100);
    td_input_poll_timeouts(&s_p, 100 + TD_SEQ_TIMEOUT_MS);
    CHECK(!pop(&ev));
    FEED("q", false);
    CHECK(pop(&ev));
    CHECK_EQ(ev.key, 'q');
}

int main(void)
{
    test_keys();
    test_lone_esc();
    test_mouse();
    test_drag_merging();
    test_cursor_report();
    test_paste();
    test_garbage();
    return TD_TEST_RESULT();
}
