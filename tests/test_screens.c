/*
 * test_screens.c - a second screen (td_screen2_attach): two emulated
 * terminals (td_vterm, which answer the size query and ESC[5n like real
 * ones) show the same desktop (duplicate: the smaller size) or one wide
 * desktop (extend: the primary is the main monitor with the taskbar; a
 * click on the second terminal lands at its column plus the primary's
 * width; a window maximises on the monitor that holds it). A second
 * terminal that stops answering does not hold up the primary, and
 * detaching it gives the primary its own size back.
 */
#include "td_test.h"
#include "tinydesk/td.h"
#include "tinydesk/td_vterm.h"

#include <stdlib.h>
#include <string.h>

static uint32_t s_now = 1000;

typedef struct
{
    td_vterm_t vt;
    uint8_t in[4096];       /* what the terminal sends back (answers, input) */
    int head, tail;
    bool mute;              /* it stops answering */
    long written;
} fake_t;

static fake_t *s_a, *s_b;

static void push(fake_t *f, const char *data, int len)
{
    for (int i = 0; i < len; i++)
    {
        f->in[f->head] = (uint8_t)data[i];
        f->head = (f->head + 1) % (int)sizeof(f->in);
    }
}
static void reply(void *user, const char *data, int len)
{
    fake_t *f = user;
    if (!f->mute)
        push(f, data, len);
}
static int term_read(void *ctx)
{
    fake_t *f = ctx;
    if (f->tail == f->head)
        return -1;
    int c = f->in[f->tail];
    f->tail = (f->tail + 1) % (int)sizeof(f->in);
    return c;
}
static int term_write(void *ctx, const uint8_t *buf, int len)
{
    fake_t *f = ctx;
    f->written += len;
    td_vterm_write(&f->vt, buf, len);
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

static void run(int ms)
{
    for (int t = 0; t < ms; t += 10)
    {
        td_step();
        s_now += 10;
    }
}

/* Does row y of a terminal contain text? */
static bool row_has(fake_t *f, int y, const char *text)
{
    char line[TD_VT_MAX_COLS + 1];
    int n = f->vt.cols;
    for (int x = 0; x < n; x++)
    {
        uint16_t ch = f->vt.cells[y * TD_VT_MAX_COLS + x].ch;
        line[x] = ch >= 32 && ch < 127 ? (char)ch : ' ';
    }
    line[n] = 0;
    return strstr(line, text) != NULL;
}

static fake_t *new_fake(int cols, int rows)
{
    fake_t *f = calloc(1, sizeof(*f));
    td_vterm_init(&f->vt, cols, rows);
    f->vt.reply = reply;
    f->vt.reply_user = f;
    return f;
}

static void click(fake_t *f, int x, int y)
{
    char seq[40];
    int n = snprintf(seq, sizeof(seq), "\x1b[<0;%d;%dM\x1b[<0;%d;%dm", x + 1, y + 1, x + 1, y + 1);
    push(f, seq, n);
}

static td_window_t *s_win;
static int s_click_x = -1;
static bool on_event(td_window_t *w, const td_event_t *ev)
{
    (void)w;
    if (ev->type == TD_EV_MOUSE && ev->action == TD_MOUSE_PRESS)
        s_click_x = ev->x;
    return false;
}

int main(void)
{
    s_a = new_fake(80, 24);
    s_b = new_fake(60, 20);
    td_hal_t hal = {term_read, term_write, fake_millis, fake_sleep, s_a};
    td_init(&hal);
    run(500);
    CHECK_EQ(td_stats()->cols, 80);
    CHECK(row_has(s_a, 23, "[Start]"));

    /* Duplicate: both show the desktop, at the smaller size. */
    td_screens_set_mode(TD_SCREENS_DUPLICATE);
    CHECK(td_screen2_attach(term_read, term_write, s_b));
    run(1500);
    CHECK_EQ(td_stats()->cols, 60);
    CHECK_EQ(td_stats()->rows, 20);
    CHECK_EQ(td_wm_main_cols(), 0);
    CHECK(row_has(s_a, 19, "[Start]"));
    CHECK(row_has(s_b, 19, "[Start]"));

    /* Extend: 80 + 60 columns, 20 rows; the taskbar on the primary only. */
    td_screens_set_mode(TD_SCREENS_EXTEND);
    run(1500);
    CHECK_EQ(td_stats()->cols, 140);
    CHECK_EQ(td_stats()->rows, 20);
    CHECK_EQ(td_wm_main_cols(), 80);
    CHECK(row_has(s_a, 19, "[Start]"));
    CHECK(!row_has(s_b, 19, "[Start]"));

    /* A window on the second monitor: clicks there arrive at 80 + x, and
     * maximising fills that monitor only. */
    td_window_desc_t d = {.title = "Second", .rect = td_rect(90, 2, 30, 8), .flags = TD_WIN_DEFAULT, .on_event = on_event};
    s_win = td_win_create(&d);
    CHECK(s_win != NULL);
    run(300);
    CHECK(row_has(s_b, 2, "Second"));
    click(s_b, 20, 5);   /* the second terminal's column 20 is the screen's 100 */
    run(300);
    CHECK_EQ(s_click_x + td_win_client(s_win).x, 100);   /* windows get client columns */
    td_win_toggle_maximize(s_win);
    run(300);
    CHECK_EQ(s_win->rect.x, 80);
    CHECK_EQ(s_win->rect.w, 60);
    td_win_toggle_maximize(s_win);

    /* A second terminal that stops answering does not hold up the primary. */
    s_b->mute = true;
    long before = s_a->written;
    for (int i = 0; i < 5; i++)
    {
        td_win_set_title(s_win, i % 2 ? "Again" : "Second");
        run(200);
    }
    CHECK(s_a->written > before);
    s_b->mute = false;

    /* Detached: the primary's own size again, one monitor. */
    td_screen2_detach();
    run(800);
    CHECK(!td_screen2_attached());
    CHECK_EQ(td_stats()->cols, 80);
    CHECK_EQ(td_stats()->rows, 24);
    CHECK_EQ(td_wm_main_cols(), 0);
    CHECK(row_has(s_a, 23, "[Start]"));
    return TD_TEST_RESULT();
}
