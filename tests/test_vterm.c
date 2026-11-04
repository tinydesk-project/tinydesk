/*
 * test_vterm.c - the terminal emulator used by the Terminal app.
 */
#include "td_test.h"
#include "tinydesk/td_vterm.h"

static td_vterm_t s_vt;
static char s_reply[64];

static void on_reply(void *user, const char *data, int len)
{
    (void)user;
    memcpy(s_reply, data, (size_t)len);
    s_reply[len] = '\0';
}

static void put(const char *s)
{
    td_vterm_write(&s_vt, (const uint8_t *)s, (int)strlen(s));
}

static char at(int x, int y)
{
    return (char)s_vt.cells[y * TD_VT_MAX_COLS + x].ch;
}

static bool row_is(int y, const char *text)
{
    for (int i = 0; text[i]; i++)
        if (at(i, y) != text[i])
            return false;
    return true;
}

static void test_text_and_newlines(void)
{
    td_vterm_init(&s_vt, 20, 5);
    put("hello\r\nworld\n!");
    CHECK(row_is(0, "hello"));
    CHECK(row_is(1, "world"));
    CHECK(row_is(2, "!"));          /* LF implies CR (newline mode) */
    CHECK_EQ(s_vt.cx, 1);
    CHECK_EQ(s_vt.cy, 2);

    put("\b\bX");
    CHECK(row_is(2, "X"));
    put("\tT");
    CHECK_EQ(at(8, 2), 'T');
}

static void test_wrap_and_scroll(void)
{
    td_vterm_init(&s_vt, 5, 3);
    put("abcdefg");
    CHECK(row_is(0, "abcde"));
    CHECK(row_is(1, "fg"));
    put("\n1\n2\n3");
    CHECK(row_is(2, "3"));
    CHECK(td_vterm_scrollback_lines(&s_vt) >= 1);
}

static void test_csi(void)
{
    td_vterm_init(&s_vt, 20, 5);
    put("0123456789");
    put("\x1b[1;4H");               /* row 1, col 4 */
    CHECK_EQ(s_vt.cx, 3);
    put("\x1b[K");                  /* erase to end of line */
    CHECK(row_is(0, "012   "));
    put("\x1b[2J\x1b[H");
    CHECK(row_is(0, "   "));
    CHECK_EQ(s_vt.cx, 0);

    put("abc\x1b[2D\x1b[P");        /* delete 'b' */
    CHECK(row_is(0, "ac"));
    put("\x1b[3;5Hx\x1b[A\x1b[2Cy");
    CHECK_EQ(at(4, 2), 'x');
    CHECK_EQ(at(7, 1), 'y');

    /* TinyDesk Shell's line editor redraw: CR, erase line, prompt, move left. */
    put("\r\x1b[2Kroot# ls\x1b[2D");
    CHECK(row_is(1, "root# ls"));
    CHECK_EQ(s_vt.cx, 6);
}

static void test_colours(void)
{
    td_vterm_init(&s_vt, 20, 5);
    put("\x1b[1;32mG\x1b[0m\x1b[44mB\x1b[38;5;200mP\x1b[7mR");
    td_vcell_t *c = s_vt.cells;
    CHECK_EQ(c[0].fg, 10);          /* bold green -> bright green */
    CHECK_EQ(c[1].bg, 4);
    CHECK_EQ(c[2].fg, 200);
    CHECK_EQ(c[3].fg, 4);           /* reverse swaps */
    CHECK_EQ(c[3].bg, 200);
}

static void test_queries(void)
{
    td_vterm_init(&s_vt, 20, 5);
    s_vt.reply = on_reply;
    put("\x1b[3;7H\x1b[6n");
    CHECK(strcmp(s_reply, "\x1b[3;7R") == 0);
    put("\x1b[5n");
    CHECK(strcmp(s_reply, "\x1b[0n") == 0);
    put("\x1b[?25l");
    CHECK(!s_vt.cursor_visible);
    put("\x1b]0;title\x07Z");       /* OSC is swallowed */
    CHECK_EQ(at(6, 2), 'Z');
}

static void test_utf8_and_resize(void)
{
    td_vterm_init(&s_vt, 10, 4);
    put("\xE2\x94\x80");
    CHECK_EQ(s_vt.cells[0].ch, 0x2500);
    put("\r\n\r\n\r\nlast");
    td_vterm_resize(&s_vt, 10, 2);   /* the cursor line stays visible */
    CHECK_EQ(s_vt.cy, 1);
    CHECK(row_is(1, "last"));
}

int main(void)
{
    td_vterm_init(&s_vt, 10, 4);
    put("\x1b[?2026hHELLO");
    CHECK(s_vt.synchronized_output);
    CHECK(row_is(0, "HELLO"));
    put("\x1b[?2026l");
    CHECK(!s_vt.synchronized_output);
    put("\x1b[?2026h\x1b" "c");
    CHECK(!s_vt.synchronized_output);
    test_text_and_newlines();
    test_wrap_and_scroll();
    test_csi();
    test_colours();
    test_queries();
    test_utf8_and_resize();
    return TD_TEST_RESULT();
}
