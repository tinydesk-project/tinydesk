/*
 * tdsim.c - run the full host desktop (apps + TinyDesk Shell) against a simulated
 * terminal and print text "screenshots". Used to check behaviour end to end
 * without an interactive console, e.g. in CI:
 *
 *   tdsim size=80x25 wait=500 shot key=Enter wait=800 type="ls\r" wait=300 shot
 *
 * Actions (run in order):
 *   size=COLSxROWS     terminal size (first argument only)
 *   wait=MS            run the main loop for MS milliseconds
 *   type=TEXT          send text; \r \n \t \e \\ and \xHH escapes work
 *   key=NAME           Enter Esc Tab BTab Up Down Left Right Home End PgUp
 *                      PgDn Del Bksp F1..F12 CtrlA..CtrlZ
 *   click=X,Y          left click at 0-based cell X,Y
 *   rclick=X,Y         right click
 *   hold=X,Y,MS        left button held down at X,Y for MS milliseconds
 *   drag=X1,Y1,X2,Y2   left-button drag
 *   move=X,Y           mouse movement without a button (hover)
 *   wheel=X,Y,up|down  mouse wheel
 *   shot               print the screen
 *   save=FILE          write everything the desktop sent so far to FILE
 *                      (render it with: vtshot FILE COLS ROWS --svg out.svg)
 *   stats              print traffic statistics
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define dup    _dup
#define fdopen _fdopen
#else
#include <time.h>
#include <unistd.h>
#endif

#include "net_fake.h"
#include "td_apps.h"
#include "td_host_hal.h"
#include "tinydesk/td_vterm.h"

static td_vterm_t s_term;          /* what the user would see */
static uint8_t s_in[8192];
static int s_in_head, s_in_count;
static uint32_t s_written;

/* Our own handle on the real stdout: on POSIX the embedded shell takes
 * over the process-wide stdout, so plain printf would land in its window. */
static FILE *s_out;

static void push_input(const char *data, int len)
{
    for (int i = 0; i < len && s_in_count < (int)sizeof(s_in); i++)
    {
        s_in[(s_in_head + s_in_count) % sizeof(s_in)] = (uint8_t)data[i];
        s_in_count++;
    }
}

static int sim_read(void *ctx)
{
    (void)ctx;
    if (s_in_count == 0)
        return -1;
    uint8_t b = s_in[s_in_head];
    s_in_head = (s_in_head + 1) % (int)sizeof(s_in);
    s_in_count--;
    return b;
}

/* Everything the desktop wrote, for save=FILE. */
static uint8_t *s_log;
static size_t s_log_len, s_log_cap;

static void log_output(const uint8_t *buf, int len)
{
    if (s_log_len + (size_t)len > s_log_cap)
    {
        size_t cap = s_log_cap ? s_log_cap * 2 : 65536;
        while (cap < s_log_len + (size_t)len)
            cap *= 2;
        uint8_t *p = realloc(s_log, cap);
        if (!p)
            return;
        s_log = p;
        s_log_cap = cap;
    }
    memcpy(s_log + s_log_len, buf, (size_t)len);
    s_log_len += (size_t)len;
}

static int sim_write(void *ctx, const uint8_t *buf, int len)
{
    (void)ctx;
    log_output(buf, len);
    td_vterm_write(&s_term, buf, len);
    s_written += (uint32_t)len;
    return len;
}

static uint32_t sim_millis(void *ctx)
{
    (void)ctx;
#ifdef _WIN32
    return (uint32_t)GetTickCount64();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)(ts.tv_sec * 1000u + ts.tv_nsec / 1000000u);
#endif
}

static void sim_sleep(void *ctx, uint32_t ms)
{
    (void)ctx;
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts = {ms / 1000u, (long)(ms % 1000u) * 1000000L};
    nanosleep(&ts, NULL);
#endif
}

static const td_hal_t s_hal = {sim_read, sim_write, sim_millis, sim_sleep, NULL};

/* Test accounts: root / secret and bob / pw. */
static bool sim_user_exists(const char *u)
{
    return strcmp(u, "root") == 0 || strcmp(u, "bob") == 0;
}
static bool sim_auth(const char *u, const char *p)
{
    return (strcmp(u, "root") == 0 && strcmp(p, "secret") == 0) ||
           (strcmp(u, "bob") == 0 && strcmp(p, "pw") == 0);
}

/* The simulated terminal answers queries (cursor position = size). */
static void term_reply(void *user, const char *data, int len)
{
    (void)user;
    push_input(data, len);
}

static void run_for(uint32_t ms)
{
    uint32_t end = sim_millis(NULL) + ms;
    while ((int32_t)(end - sim_millis(NULL)) > 0)
    {
        td_step();
        sim_sleep(NULL, 5);
    }
}

static void shot(void)
{
    fprintf(s_out, "+");
    for (int x = 0; x < s_term.cols; x++)
        fprintf(s_out, "-");
    fprintf(s_out, "+\n");
    for (int y = 0; y < s_term.rows; y++)
    {
        fprintf(s_out, "|");
        for (int x = 0; x < s_term.cols; x++)
        {
            uint8_t u[4];
            int n = td_utf8_encode(s_term.cells[y * TD_VT_MAX_COLS + x].ch, u);
            fwrite(u, 1, (size_t)n, s_out);
        }
        fprintf(s_out, "|\n");
    }
    fprintf(s_out, "+");
    for (int x = 0; x < s_term.cols; x++)
        fprintf(s_out, "-");
    fprintf(s_out, "+\n");
    fflush(s_out);
}

static void send_text(const char *s)
{
    char out[1024];
    int n = 0;
    for (; *s && n < (int)sizeof(out) - 1; s++)
    {
        if (*s != '\\' || !s[1])
        {
            out[n++] = *s;
            continue;
        }
        s++;
        switch (*s)
        {
        case 'r':
            out[n++] = '\r';
            break;
        case 'n':
            out[n++] = '\n';
            break;
        case 't':
            out[n++] = '\t';
            break;
        case 'e':
            out[n++] = 0x1b;
            break;
        case 'x':
        {
            char hex[3] = {s[1], s[1] ? s[2] : 0, 0};
            out[n++] = (char)strtol(hex, NULL, 16);
            s += 2;
            break;
        }
        default:
            out[n++] = *s;
            break;
        }
    }
    push_input(out, n);
}

static const struct
{
    const char *name;
    const char *seq;
} s_keys[] = {
    {"Enter", "\r"},
    {"Esc", "\x1b"},
    {"Tab", "\t"},
    {"BTab", "\x1b[Z"},
    {"Up", "\x1b[A"},
    {"Down", "\x1b[B"},
    {"Right", "\x1b[C"},
    {"Left", "\x1b[D"},
    {"Home", "\x1b[H"},
    {"End", "\x1b[F"},
    {"PgUp", "\x1b[5~"},
    {"PgDn", "\x1b[6~"},
    {"ShiftPgUp", "\x1b[5;2~"},
    {"ShiftPgDn", "\x1b[6;2~"},
    {"Del", "\x1b[3~"},
    {"Bksp", "\x7f"},
    {"F1", "\x1bOP"},
    {"F2", "\x1bOQ"},
    {"F3", "\x1bOR"},
    {"F4", "\x1bOS"},
    {"F5", "\x1b[15~"},
    {"F6", "\x1b[17~"},
    {"F7", "\x1b[18~"},
    {"F8", "\x1b[19~"},
    {"F9", "\x1b[20~"},
    {"F10", "\x1b[21~"},
    {"F11", "\x1b[23~"},
    {"F12", "\x1b[24~"},
};

static void send_key(const char *name)
{
    if (strncmp(name, "Ctrl", 4) == 0 && name[4] >= 'A' && name[4] <= 'Z' && !name[5])
    {
        char c = (char)(name[4] - 'A' + 1);
        push_input(&c, 1);
        return;
    }
    for (size_t i = 0; i < sizeof(s_keys) / sizeof(s_keys[0]); i++)
    {
        if (strcmp(s_keys[i].name, name) == 0)
        {
            push_input(s_keys[i].seq, (int)strlen(s_keys[i].seq));
            return;
        }
    }
    fprintf(stderr, "tdsim: unknown key %s\n", name);
}

static void send_mouse(int b, int x, int y, char final)
{
    char seq[32];
    int n = snprintf(seq, sizeof(seq), "\x1b[<%d;%d;%d%c", b, x + 1, y + 1, final);
    push_input(seq, n);
}

int main(int argc, char **argv)
{
    int cols = 80, rows = 25, first = 1;
    if (argc > 1 && sscanf(argv[1], "size=%dx%d", &cols, &rows) == 2)
        first = 2;

    s_out = fdopen(dup(1), "w");
    if (!s_out)
        return 1;
    s_term.reply = term_reply;
    td_vterm_init(&s_term, cols, rows);
    td_host_use_net(td_net_fake());
    td_host_use_users(sim_user_exists, sim_auth);
    td_host_setup(&s_hal, "simulated host");
    run_for(100);

    for (int i = first; i < argc; i++)
    {
        const char *a = argv[i];
        int x, y, x2, y2;
        char dir[8];
        if (strncmp(a, "wait=", 5) == 0)
        {
            run_for((uint32_t)atoi(a + 5));
        }
        else if (strncmp(a, "type=", 5) == 0)
        {
            send_text(a + 5);
            run_for(80);
        }
        else if (strncmp(a, "key=", 4) == 0)
        {
            send_key(a + 4);
            run_for(80);
        }
        else if (sscanf(a, "click=%d,%d", &x, &y) == 2)
        {
            send_mouse(0, x, y, 'M');
            run_for(20);
            send_mouse(0, x, y, 'm');
            run_for(80);
        }
        else if (sscanf(a, "hold=%d,%d,%d", &x, &y, &x2) == 3)
        {
            send_mouse(0, x, y, 'M');
            run_for((uint32_t)x2);
            send_mouse(0, x, y, 'm');
            run_for(80);
        }
        else if (sscanf(a, "drag=%d,%d,%d,%d", &x, &y, &x2, &y2) == 4)
        {
            send_mouse(0, x, y, 'M');
            run_for(20);
            send_mouse(32, x2, y2, 'M');
            run_for(20);
            send_mouse(0, x2, y2, 'm');
            run_for(80);
        }
        else if (sscanf(a, "rclick=%d,%d", &x, &y) == 2)
        {
            send_mouse(2, x, y, 'M');
            run_for(20);
            send_mouse(2, x, y, 'm');
            run_for(80);
        }
        else if (sscanf(a, "move=%d,%d", &x, &y) == 2)
        {
            send_mouse(35, x, y, 'M');   /* motion, no button (hover) */
            run_for(80);
        }
        else if (sscanf(a, "wheel=%d,%d,%7s", &x, &y, dir) == 3)
        {
            send_mouse(strcmp(dir, "up") == 0 ? 64 : 65, x, y, 'M');
            run_for(80);
        }
        else if (strcmp(a, "shot") == 0)
        {
            shot();
        }
        else if (strncmp(a, "save=", 5) == 0)
        {
            FILE *f = fopen(a + 5, "wb");
            if (!f || fwrite(s_log, 1, s_log_len, f) != s_log_len)
            {
                fprintf(stderr, "tdsim: cannot write %s\n", a + 5);
                return 2;
            }
            fclose(f);
        }
        else if (strcmp(a, "stats") == 0)
        {
            const td_stats_t *st = td_stats();
            fprintf(s_out, "screen %dx%d, frames %u, bytes %u, dropped %u\n", st->cols, st->rows,
                    (unsigned)st->frames, (unsigned)s_written, (unsigned)st->dropped_frames);
        }
        else
        {
            fprintf(stderr, "tdsim: unknown action %s\n", a);
            return 2;
        }
    }
    fflush(s_out);
    td_shutdown();
    return 0;
}
