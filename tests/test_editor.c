/*
 * test_editor.c - the Editor's undo and redo, cut / copy / paste, and
 * closing with unsaved changes. Keys and pastes are synthetic events; the
 * file lives in memory, and the text is read back from the screen. Also
 * the extras: indents, comments, find and replace, go to a line, Python
 * completion, and running a file with its error line found in the output.
 */
#include "td_test.h"
#include "td_apps.h"

#define PATH "/fs/a.txt"

static uint32_t s_now = 10000;
static int fake_read(void *ctx)
{
    (void)ctx;
    return -1;
}
static int fake_write(void *ctx, const uint8_t *b, int n)
{
    (void)ctx;
    (void)b;
    return n;
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

/* ------------------------------------------------- one file in memory */

static char s_file[20000];
static int s_file_len;
static int s_writes;

static int fs_read(const char *path, char *buf, int cap)
{
    (void)path;
    int n = s_file_len < cap ? s_file_len : cap;
    memcpy(buf, s_file, (size_t)n);
    return n;
}

static int fs_write(const char *path, const char *data, int len)
{
    (void)path;
    if (len > (int)sizeof(s_file))
        return -1;
    memcpy(s_file, data, (size_t)len);
    s_file_len = len;
    s_writes++;
    return 0;
}

static int fs_exists(const char *path)
{
    (void)path;
    return 1;
}

static const td_fs_ops_t s_fs = {.root = "/fs", .read = fs_read, .write = fs_write, .exists = fs_exists};
static const td_sysinfo_t s_info = {.fs = &s_fs};

static bool file_is(const char *expect)
{
    if (s_file_len == (int)strlen(expect) && memcmp(s_file, expect, (size_t)s_file_len) == 0)
        return true;
    printf("  file: \"%.*s\", expected \"%s\"\n", s_file_len, s_file, expect);
    return false;
}

/* ------------------------------------------------------------- input */

static void key(uint32_t k, uint8_t mods)
{
    td_event_t ev = {0};
    ev.type = TD_EV_KEY;
    ev.key = k;
    ev.mods = mods;
    ev.time_ms = s_now;
    td_wm_dispatch(&ev);
}

static void ctrl(char c)
{
    key((uint32_t)c, TD_MOD_CTRL);
}

static void type(const char *s)
{
    while (*s)
        key((uint8_t)*s++, 0);
}

/* A bracketed paste, as a terminal sends it, through the input parser. */
static void paste(const char *text)
{
    static td_input_t in;
    td_input_init(&in);
    const char *parts[3] = {"\x1b[200~", text, "\x1b[201~"};
    for (int i = 0; i < 3; i++)
        for (const char *p = parts[i]; *p; p++)
            td_input_feed(&in, (uint8_t)*p, s_now);
    td_event_t ev;
    while (td_event_pop(&ev))
    {
        td_wm_dispatch(&ev);
        if (ev.type == TD_EV_PASTE)
            td_input_paste_done(&in);
    }
}

/* ------------------------------------------------------------ screen */

static td_window_t *s_ed;
static td_buffer_t s_back;

/* Open PATH holding `text` in a fresh Editor window. */
static void open_file(const char *text)
{
    td_wm_close_all();
    s_file_len = (int)strlen(text);
    memcpy(s_file, text, (size_t)s_file_len);
    s_writes = 0;
    td_editor_open(PATH);
    s_ed = td_win_focused();
    CHECK(s_ed != NULL);
}

/* Row `row` of the Editor's client area as drawn, without trailing blanks. */
static const char *row_text(int row)
{
    static char out[128];
    td_wm_compose(&s_back);
    td_rect_t c = td_win_client(s_ed);
    int n = 0;
    for (int x = 0; x < c.w && n < (int)sizeof(out) - 1; x++)
    {
        uint32_t ch = td_buffer_cell(&s_back, c.x + x, c.y + row)->ch;
        out[n++] = ch < 0x80 ? (char)ch : '?';
    }
    while (n > 0 && out[n - 1] == ' ')
        n--;
    out[n] = '\0';
    return out;
}

static bool shows(int row, const char *expect)
{
    const char *got = row_text(row);
    if (strcmp(got, expect) == 0)
        return true;
    printf("  row %d: \"%s\", expected \"%s\"\n", row, got, expect);
    return false;
}

/* The status bar is the last row. */
static bool status_contains(const char *text)
{
    return strstr(row_text(td_win_client(s_ed).h - 1), text) != NULL;
}

static bool status_has(const char *text)
{
    if (status_contains(text))
        return true;
    printf("  status: \"%s\", expected \"%s\" in it\n", row_text(td_win_client(s_ed).h - 1), text);
    return false;
}

/* Unsaved changes show as a '*' before the title. */
static bool changed(void)
{
    return s_ed->title[0] == '*';
}

/* -------------------------------------------------------------- tests */

static void test_undo_by_word(void)
{
    open_file("");
    type("hello world");
    CHECK(shows(0, "hello world"));
    CHECK(changed());

    /* Typing is undone a word at a time. */
    ctrl('z');
    CHECK(shows(0, "hello"));
    CHECK(status_has("Col 7"));                /* the space is still there */
    ctrl('z');
    CHECK(shows(0, ""));
    CHECK(!changed());                         /* back to the file as it was */
    ctrl('z');
    CHECK(status_has("Nothing to undo"));

    /* Redo with Ctrl+Y or Ctrl+Shift+Z. */
    ctrl('y');
    CHECK(shows(0, "hello"));
    key('z', TD_MOD_CTRL | TD_MOD_SHIFT);
    CHECK(shows(0, "hello world"));
    ctrl('y');
    CHECK(status_has("Nothing to redo"));

    ctrl('s');
    CHECK(file_is("hello world"));
    CHECK(!changed());
}

static void test_changed_follows_the_save_point(void)
{
    open_file("abc");
    CHECK(!changed());
    key(TD_KEY_END, 0);
    type("!");
    CHECK(changed());
    ctrl('s');
    CHECK(file_is("abc!"));
    CHECK(!changed());

    /* Undoing past the save marks the file changed; redoing back to it
     * clears the mark. */
    ctrl('z');
    CHECK(shows(0, "abc"));
    CHECK(changed());
    ctrl('y');
    CHECK(!changed());

    /* Typing after a save is a step of its own. */
    type("?");
    ctrl('z');
    CHECK(shows(0, "abc!"));
    CHECK(!changed());
}

static void test_save_twice(void)
{
    /* The file keeps its name after a save: the next one writes it again
     * instead of asking for a name. */
    open_file("1");
    key(TD_KEY_END, 0);
    type("2");
    ctrl('s');
    CHECK(strncmp(s_ed->title, "a.txt", 5) == 0);
    type("3");
    ctrl('s');
    CHECK(td_win_focused() == s_ed);
    CHECK(file_is("123"));
}

static void test_new_edit_ends_redo(void)
{
    open_file("");
    type("one two");
    ctrl('z');
    type("three");
    CHECK(shows(0, "one three"));
    ctrl('y');
    CHECK(status_has("Nothing to redo"));
    ctrl('z');
    CHECK(shows(0, "one"));
    ctrl('z');
    CHECK(shows(0, ""));
}

static void test_delete_runs(void)
{
    open_file("abcdef");
    key(TD_KEY_END, 0);

    /* A run of Backspace is one step, and so is a run of Delete. */
    for (int i = 0; i < 3; i++)
        key(TD_KEY_BACKSPACE, 0);
    CHECK(shows(0, "abc"));
    key(TD_KEY_HOME, 0);
    for (int i = 0; i < 2; i++)
        key(TD_KEY_DELETE, 0);
    CHECK(shows(0, "c"));
    ctrl('z');
    CHECK(shows(0, "abc"));
    CHECK(status_has("Col 1"));                /* where the Deletes started */
    ctrl('z');
    CHECK(shows(0, "abcdef"));
    CHECK(status_has("Col 7"));
}

static void test_typing_over_a_selection(void)
{
    /* Replacing the selection and typing on is one step. */
    open_file("old text");
    ctrl('a');
    type("new");
    CHECK(shows(0, "new"));
    ctrl('z');
    CHECK(shows(0, "old text"));
    ctrl('y');
    CHECK(shows(0, "new"));

    /* Shift+arrows select too. */
    open_file("abcdef");
    for (int i = 0; i < 3; i++)
        key(TD_KEY_RIGHT, TD_MOD_SHIFT);
    type("X");
    CHECK(shows(0, "Xdef"));
    ctrl('z');
    CHECK(shows(0, "abcdef"));
}

static void test_cut_copy_paste(void)
{
    open_file("abc");
    ctrl('a');
    ctrl('x');
    CHECK(shows(0, ""));
    ctrl('v');
    ctrl('v');
    CHECK(shows(0, "abcabc"));

    /* Each cut and each paste is a step of its own. */
    ctrl('z');
    CHECK(shows(0, "abc"));
    ctrl('z');
    CHECK(shows(0, ""));
    ctrl('z');
    CHECK(shows(0, "abc"));
    CHECK(!changed());

    /* Copying changes nothing and is not a step. */
    ctrl('a');
    ctrl('c');
    CHECK(status_has("Copied 3 characters"));
    key(TD_KEY_END, 0);
    ctrl('v');
    CHECK(shows(0, "abcabc"));
    ctrl('z');
    CHECK(shows(0, "abc"));
    CHECK(!changed());
}

static void test_paste_from_the_terminal(void)
{
    /* CR and CRLF become LF, and the whole paste is one step. */
    open_file("");
    paste("one\r\ntwo\rthree");
    CHECK(shows(0, "one"));
    CHECK(shows(1, "two"));
    CHECK(shows(2, "three"));
    CHECK(status_has("Pasted 13 characters"));
    ctrl('z');
    CHECK(shows(0, ""));
    CHECK(shows(1, ""));

    /* It also went to the clipboard. */
    ctrl('v');
    ctrl('s');
    CHECK(file_is("one\ntwo\nthree"));
}

static void test_history_limits(void)
{
    /* The oldest steps are dropped when the history is full; what is left
     * still undoes and redoes to the right text. Words differ, so text
     * restored from the wrong place in the history would show. */
    enum
    {
        WORDS = 200
    };
    static char full[WORDS * 2 + 1];
    open_file("");
    for (int i = 0; i < WORDS; i++)
    {
        char word[3] = {(char)('a' + i % 26), ' ', '\0'};
        type(word);
        memcpy(full + i * 2, word, 2);
    }
    int undone = 0;
    while (undone <= WORDS)
    {
        ctrl('z');
        if (status_contains("Nothing to undo"))
            break;
        undone++;
    }
    int kept = WORDS - undone;
    CHECK(kept > 0 && kept < WORDS);
    ctrl('s');
    char expect[WORDS * 2 + 1];
    snprintf(expect, sizeof(expect), "%.*s", kept * 2, full);
    CHECK(file_is(expect));
    for (int i = 0; i < undone; i++)
        ctrl('y');
    ctrl('s');
    CHECK(file_is(full));

    /* A paste larger than the whole history (TD_EDITOR_UNDO, 6 KB by
     * default) is kept in the text but cannot be undone; the file stays
     * marked as changed. */
    static char big[10001];
    memset(big, 'x', sizeof(big) - 1);
    open_file("");
    td_clipboard_set(big, (int)sizeof(big) - 1);
    ctrl('v');
    ctrl('z');
    CHECK(status_has("Nothing to undo"));
    CHECK(changed());
    ctrl('s');
    CHECK_EQ(s_file_len, (int)sizeof(big) - 1);
}

static void test_close_asks_about_changes(void)
{
    /* Without changes Esc closes at once. */
    open_file("x");
    key(TD_KEY_ESC, 0);
    CHECK(!td_win_is_open(s_ed));

    /* With changes it asks first; Esc on the question keeps the Editor. */
    open_file("x");
    key(TD_KEY_END, 0);
    type("y");
    key(TD_KEY_ESC, 0);
    CHECK(td_win_is_open(s_ed));
    td_window_t *question = td_win_focused();
    CHECK(question != s_ed && (question->flags & TD_WIN_MODAL));
    key(TD_KEY_ESC, 0);
    CHECK(td_win_focused() == s_ed);
    CHECK(changed());

    /* Discard closes without saving. */
    key(TD_KEY_ESC, 0);
    key(TD_KEY_TAB, 0);                        /* Save -> Discard */
    key(TD_KEY_ENTER, 0);
    CHECK(!td_win_is_open(s_ed));
    CHECK_EQ(s_writes, 0);
    CHECK(file_is("x"));

    /* Save writes the file, then closes. */
    open_file("x");
    key(TD_KEY_END, 0);
    type("z");
    key(TD_KEY_ESC, 0);
    key(TD_KEY_ENTER, 0);                      /* Save */
    CHECK(!td_win_is_open(s_ed));
    CHECK(file_is("xz"));
}

/* ------------------------------------------------------ Python files */

/* Open `path` holding `text`. */
static void open_as(const char *path, const char *text)
{
    td_wm_close_all();
    s_file_len = (int)strlen(text);
    memcpy(s_file, text, (size_t)s_file_len);
    s_writes = 0;
    td_editor_open(path);
    s_ed = td_win_focused();
    CHECK(s_ed != NULL);
}

static void test_indent(void)
{
    /* Enter keeps the indent, adds one after ':' and drops one after
     * return; Backspace in the indent goes back to a multiple of 4. */
    open_as("/fs/t.py", "");
    type("def f(x):");
    key(TD_KEY_ENTER, 0);
    type("if x:");
    key(TD_KEY_ENTER, 0);
    type("return 1");
    key(TD_KEY_ENTER, 0);
    type("y = 2");
    key(TD_KEY_ENTER, 0);
    key(TD_KEY_BACKSPACE, 0);
    type("z");
    ctrl('s');
    CHECK(file_is("def f(x):\n    if x:\n        return 1\n    y = 2\nz"));
    CHECK(shows(0, "  1 def f(x):"));       /* line numbers for Python */

    /* Tab and Shift+Tab move the selected lines; one undo step each. */
    open_as("/fs/t.py", "a\nb\n\nc\n");
    for (int i = 0; i < 4; i++)
        key(TD_KEY_DOWN, TD_MOD_SHIFT);
    key(TD_KEY_TAB, 0);
    key(TD_KEY_TAB, 0);
    ctrl('s');
    CHECK(file_is("        a\n        b\n\n        c\n"));   /* blank lines stay blank */
    key(TD_KEY_TAB, TD_MOD_SHIFT);
    ctrl('s');
    CHECK(file_is("    a\n    b\n\n    c\n"));
    for (int i = 0; i < 3; i++)
        ctrl('z');
    ctrl('s');
    CHECK(file_is("a\nb\n\nc\n"));

    /* Tab without a selection: spaces to the next multiple of 4. */
    open_as("/fs/a.txt", "ab");
    key(TD_KEY_END, 0);
    key(TD_KEY_TAB, 0);
    type("c");
    ctrl('s');
    CHECK(file_is("ab  c"));
}

static void test_comment(void)
{
    open_as("/fs/t.py", "x = 1\n    y = 2\n\nz\n");
    ctrl('a');
    key('_', TD_MOD_CTRL);                    /* what Ctrl+/ sends */
    ctrl('s');
    CHECK(file_is("# x = 1\n#     y = 2\n\n# z\n"));
    key('/', TD_MOD_CTRL);
    ctrl('s');
    CHECK(file_is("x = 1\n    y = 2\n\nz\n"));

    /* One line: at its own indent; the cursor stays on the text. */
    open_as("/fs/t.py", "if a:\n    b()\n");
    key(TD_KEY_DOWN, 0);
    key(TD_KEY_END, 0);
    key('_', TD_MOD_CTRL);
    type("!");
    ctrl('s');
    CHECK(file_is("if a:\n    # b()!\n"));
}

/* Answer the dialog that has the keyboard. */
static void answer(const char *text)
{
    td_window_t *box = td_win_focused();
    CHECK(box != s_ed);
    type(text);
    key(TD_KEY_ENTER, 0);
}

static void test_find_replace_go_to(void)
{
    open_as("/fs/a.txt", "one two\nthree two\nfour");
    ctrl('f');
    answer("two");
    CHECK(status_has("Ln 1, Col 8"));         /* selected; the cursor after it */
    CHECK(status_has("3 selected"));
    key(TD_KEY_F3, 0);
    CHECK(status_has("Ln 2, Col 10"));
    key(TD_KEY_F3, 0);
    CHECK(status_has("From the top"));
    CHECK(status_has("Ln 1, Col 8"));

    ctrl('r');
    answer("");                               /* suggested: the selection */
    answer("2");
    CHECK(status_has("Replaced 2"));
    ctrl('s');
    CHECK(file_is("one 2\nthree 2\nfour"));
    ctrl('z');                                /* one step */
    ctrl('s');
    CHECK(file_is("one two\nthree two\nfour"));

    ctrl('g');
    answer("3");
    CHECK(status_has("Ln 3, Col 1"));
    ctrl('g');
    answer("99");
    CHECK(status_has("Ln 3, Col 1"));
}

static void test_buffer_grows(void)
{
    /* The buffer starts at the file's size and grows with the text, up to
     * TD_EDITOR_MAX (16384 by default). */
    static char big[16001];
    memset(big, 'a', sizeof(big) - 1);
    open_as("/fs/a.txt", "x");
    for (int i = 0; i < 4; i++)
    {
        td_clipboard_set(big, 3000);
        ctrl('v');
    }
    ctrl('s');
    CHECK_EQ(s_file_len, 12001);
    td_clipboard_set(big, 5000);
    ctrl('v');
    CHECK(status_has("The file is full"));
    type("b");
    ctrl('s');
    CHECK_EQ(s_file_len, 12002);
    ctrl('z');
    ctrl('z');
    ctrl('s');
    CHECK_EQ(s_file_len, 9001);

    /* A file that fills it: typing says so. */
    open_as("/fs/a.txt", big);
    td_clipboard_set(big, 384);
    ctrl('v');
    CHECK_EQ(s_writes, 0);
    ctrl('s');
    CHECK_EQ(s_file_len, 16384);
    type("z");
    CHECK(status_has("The file is full"));
}

static void test_completion(void)
{
    td_editor_prefs()->completion = 0;
    open_as("/fs/t.py", "import tinydesk\n");
    key(TD_KEY_END, TD_MOD_CTRL);
    type("tinydesk.");
    CHECK(!strstr(row_text(2), "log"));       /* off: nothing pops up */
    key(' ', TD_MOD_CTRL);                    /* Ctrl+Space asks */
    CHECK(strstr(row_text(2), "log") != NULL);
    key(TD_KEY_DOWN, 0);
    key(TD_KEY_ENTER, 0);
    type("(");
    ctrl('s');
    CHECK(file_is("import tinydesk\ntinydesk.modbus_read("));

    /* Popping up while typing; words of the file are offered too. */
    td_editor_prefs()->completion = 1;
    open_as("/fs/t.py", "counter = 0\n");
    key(TD_KEY_END, TD_MOD_CTRL);
    type("pri");
    key(TD_KEY_TAB, 0);
    type("(cou");
    key(TD_KEY_ENTER, 0);
    type(")");
    ctrl('s');
    CHECK(file_is("counter = 0\nprint(counter)"));
    td_editor_prefs()->completion = 0;
}

/* --------------------------------------------- run and the error line */

static char s_sent[256];
static int s_sent_len;
static const char *s_output;

static int be_start(void *ctx, int cols, int rows)
{
    (void)ctx;
    (void)cols;
    (void)rows;
    return 0;
}

static int be_read(void *ctx, uint8_t *buf, int cap)
{
    (void)ctx;
    if (!s_output)
        return 0;
    int n = (int)strlen(s_output);
    if (n > cap)
        n = cap;
    memcpy(buf, s_output, (size_t)n);
    s_output = n < (int)strlen(s_output) ? s_output + n : NULL;
    return n;
}

static int be_write(void *ctx, const uint8_t *buf, int len)
{
    (void)ctx;
    if (s_sent_len + len < (int)sizeof(s_sent))
    {
        memcpy(s_sent + s_sent_len, buf, (size_t)len);
        s_sent_len += len;
        s_sent[s_sent_len] = '\0';
    }
    return len;
}

static const td_term_backend_t s_backend = {
    .name = "test",
    .start = be_start,
    .read = be_read,
    .write = be_write,
};

static void test_run_and_error(void)
{
    open_as("/fs/t.py", "x = 1\nprint(y)\n");
    s_sent_len = 0;
    key(TD_KEY_F5, 0);
    CHECK(strcmp(s_sent, "\x15py \"/t.py\"\r") == 0);
    CHECK(status_has("F5 Run"));

    /* The output pane shows what the program prints. */
    s_output = "Traceback (most recent call last):\r\n"
               "  File \"/lib/m.py\", line 9, in f\r\n"
               "  File \"/t.py\", line 2, in <module>\r\n"
               "NameError: name 'y' isn't defined\r\n\x1b[1m$ \x1b[0m";
    td_terminal_pump();
    CHECK(status_has("Line 2: NameError: name 'y' isn't defined"));
    bool seen = false;
    for (int r = 0; r < td_win_client(s_ed).h; r++)
        seen |= strstr(row_text(r), "NameError") != NULL && strncmp(row_text(r), " Ln", 3) != 0;
    CHECK(seen);
    key(TD_KEY_F8, 0);
    CHECK(status_has("Ln 2, Col 1"));

    /* F7: keys go to the program until F7 or Esc. */
    key(TD_KEY_F7, 0);
    s_sent_len = 0;
    type("ab");
    ctrl('c');
    CHECK(strcmp(s_sent, "ab\x03") == 0);
    key(TD_KEY_ESC, 0);
    CHECK(td_win_is_open(s_ed));
    type("#");
    ctrl('s');
    CHECK(file_is("x = 1\n#print(y)\n"));

    /* Unsaved changes are saved first. */
    s_sent_len = 0;
    type("!");
    key(TD_KEY_F5, 0);
    CHECK(file_is("x = 1\n#!print(y)\n"));
    CHECK(strstr(s_sent, "py \"/t.py\"") != NULL);
    td_wm_close_all();
}

int main(void)
{
    td_set_sysinfo(&s_info);
    td_init(&s_hal);   /* no size answer: 80x25 */
    test_undo_by_word();
    test_changed_follows_the_save_point();
    test_save_twice();
    test_new_edit_ends_redo();
    test_delete_runs();
    test_typing_over_a_selection();
    test_cut_copy_paste();
    test_paste_from_the_terminal();
    test_history_limits();
    test_close_asks_about_changes();
    td_terminal_set_backend(&s_backend);
    test_indent();
    test_comment();
    test_find_replace_go_to();
    test_buffer_grows();
    test_completion();
    test_run_and_error();
    return TD_TEST_RESULT();
}
