/*
 * tdsh_bridge_host.c - TinyDesk Shell inside the tinydesk Terminal window on
 * Windows and POSIX hosts.
 *
 *   UI loop (tinydesk)                 shell thread (TinyDesk Shell)
 *   keys  --write()--> [ in pipe ] --> stdin / line editor
 *   vterm <--read()--- [ out pipe ] <-- stdout / stderr
 *
 * The UI side never blocks; the shell side blocks on its pipe exactly as it
 * would on a real terminal. On Windows this file is compiled with
 * tdsh_win_compat.h force-included, which makes stdin/stdout assignable
 * per thread.
 */
#if !defined(_WIN32) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE   /* fopencookie */
#endif

#include "tdsh_bridge.h"
#include "tinydesk/td_config.h"   /* TD_PATH_MAX */

#include <stdio.h>
#include <string.h>

#include "td_proto_cmds.h"
#include "tdsh.h"
#if defined(TD_WITH_PYTHON)
#include "td_py.h"
#endif
#include "tdsh_terminal.h"

#ifdef _WIN32
#include <windows.h>
#include "tdsh_platform_win.h"
#else
#include <pthread.h>
#include <sys/stat.h>
#include "tdsh_posix.h"
#endif

/* ------------------------------------------------------------ pipes */

#define IN_PIPE_SIZE  1024
#define OUT_PIPE_SIZE 16384

typedef struct
{
#ifdef _WIN32
    CRITICAL_SECTION cs;
    CONDITION_VARIABLE cv;
#else
    pthread_mutex_t mu;
    pthread_cond_t cv;
#endif
    uint8_t *buf;
    int cap, head, count;
} bpipe_t;

static uint8_t s_in_buf[IN_PIPE_SIZE];
static uint8_t s_out_buf[OUT_PIPE_SIZE];
static bpipe_t s_in = {.buf = s_in_buf, .cap = IN_PIPE_SIZE};
static bpipe_t s_out = {.buf = s_out_buf, .cap = OUT_PIPE_SIZE};

static void pipe_init(bpipe_t *p)
{
#ifdef _WIN32
    InitializeCriticalSection(&p->cs);
    InitializeConditionVariable(&p->cv);
#else
    pthread_mutex_init(&p->mu, NULL);
    pthread_cond_init(&p->cv, NULL);
#endif
}

#ifdef _WIN32
#define LOCK(p)   EnterCriticalSection(&(p)->cs)
#define UNLOCK(p) LeaveCriticalSection(&(p)->cs)
#define WAIT(p)   SleepConditionVariableCS(&(p)->cv, &(p)->cs, INFINITE)
#define WAKE(p)   WakeAllConditionVariable(&(p)->cv)
#else
#define LOCK(p)   pthread_mutex_lock(&(p)->mu)
#define UNLOCK(p) pthread_mutex_unlock(&(p)->mu)
#define WAIT(p)   pthread_cond_wait(&(p)->cv, &(p)->mu)
#define WAKE(p)   pthread_cond_broadcast(&(p)->cv)
#endif

/* Copy up to len bytes in; returns the count. Caller holds the lock. */
static int put_locked(bpipe_t *p, const uint8_t *data, int len)
{
    int n = 0;
    while (n < len && p->count < p->cap)
    {
        p->buf[(p->head + p->count) % p->cap] = data[n++];
        p->count++;
    }
    return n;
}

static int take_locked(bpipe_t *p, uint8_t *out, int len)
{
    int n = 0;
    while (n < len && p->count > 0)
    {
        out[n++] = p->buf[p->head];
        p->head = (p->head + 1) % p->cap;
        p->count--;
    }
    return n;
}

/* Blocking write of everything (shell side). */
static void pipe_write_all(bpipe_t *p, const uint8_t *data, int len)
{
    LOCK(p);
    while (len > 0)
    {
        while (p->count == p->cap)
            WAIT(p);
        int n = put_locked(p, data, len);
        data += n;
        len -= n;
        WAKE(p);
    }
    UNLOCK(p);
}

/* Blocking read of at least one byte (shell side). */
static int pipe_read_some(bpipe_t *p, uint8_t *out, int len)
{
    LOCK(p);
    while (p->count == 0)
        WAIT(p);
    int n = take_locked(p, out, len);
    WAKE(p);
    UNLOCK(p);
    return n;
}

/* Non-blocking versions (UI side). */
static int pipe_try_write(bpipe_t *p, const uint8_t *data, int len)
{
    LOCK(p);
    int n = put_locked(p, data, len);
    if (n)
        WAKE(p);
    UNLOCK(p);
    return n;
}

static int pipe_try_read(bpipe_t *p, uint8_t *out, int len)
{
    LOCK(p);
    int n = take_locked(p, out, len);
    if (n)
        WAKE(p);
    UNLOCK(p);
    return n;
}

/* Ctrl+C for long-running commands (modbus read -i, py): true once it was
 * typed in the Terminal window. Like a terminal, the keys typed before it
 * are dropped; keys after it stay for the shell. Shell thread only. */
static bool break_requested(void)
{
    bool brk = false;
    LOCK(&s_in);
    for (int i = s_in.count - 1; i >= 0; i--)
    {
        if (s_in.buf[(s_in.head + i) % s_in.cap] == 0x03)
        {
            s_in.head = (s_in.head + i + 1) % s_in.cap;
            s_in.count -= i + 1;
            brk = true;
            WAKE(&s_in);
            break;
        }
    }
    UNLOCK(&s_in);
    return brk;
}

/* ---------------------------------------------------- shell streams */

static int stream_read(void *cookie, char *buf, int len)
{
    (void)cookie;
    return pipe_read_some(&s_in, (uint8_t *)buf, len);
}

static int stream_write(void *cookie, const char *buf, int len)
{
    (void)cookie;
    pipe_write_all(&s_out, (const uint8_t *)buf, len);
    return len;
}

#if defined(__GLIBC__)
static ssize_t cookie_read(void *c, char *buf, size_t len)
{
    return stream_read(c, buf, (int)len);
}
static ssize_t cookie_write(void *c, const char *buf, size_t len)
{
    return stream_write(c, buf, (int)len);
}
#endif

static FILE *open_terminal_stream(void)
{
#if defined(__GLIBC__)
    cookie_io_functions_t fns = {.read = cookie_read, .write = cookie_write};
    return fopencookie(NULL, "r+", fns);
#else
    return funopen(NULL, stream_read, stream_write, NULL, NULL);
#endif
}

/* Line-editor I/O goes straight to the pipes. */
static int term_read_byte(void *ctx, uint8_t *out)
{
    (void)ctx;
    return pipe_read_some(&s_in, out, 1) == 1 ? 0 : -1;
}

static int term_write(void *ctx, const void *data, size_t len)
{
    (void)ctx;
    pipe_write_all(&s_out, data, (int)len);
    return 0;
}

/* The Terminal window's width, for the line editor (set by the UI thread
 * from start() and resize(), read by the shell thread). */
static volatile int s_cols;

static int term_columns(void *ctx)
{
    (void)ctx;
    return s_cols;
}

static const tdsh_terminal_io_t s_term_io = {
    .read_byte = term_read_byte,
    .write_bytes = term_write,
    .columns = term_columns,
};

/* ------------------------------------------------------ shell thread */

static char s_fs_root[TD_PATH_MAX];
static char s_hostname[32];
static int s_init_rc = -1;

static void say(const char *s)
{
    pipe_write_all(&s_out, (const uint8_t *)s, (int)strlen(s));
}

static void build_prompt(const tdsh_session_t *s, char *out, size_t cap)
{
    const char *shown = s->cwd;
    char tilde[TDSH_MAX_PATH + 2];
    size_t hl = strlen(s->home);
    if (strcmp(s->cwd, s->home) == 0)
    {
        shown = "~";
    }
    else if (strncmp(s->cwd, s->home, hl) == 0 && s->cwd[hl] == '/')
    {
        snprintf(tilde, sizeof(tilde), "~%s", s->cwd + hl);
        shown = tilde;
    }
    snprintf(out, cap, "\033[1;32m%s@%s\033[0m:\033[1;34m%s\033[0m%c ",
             s->username, s->hostname, shown, strcmp(s->username, "root") == 0 ? '#' : '$');
}

static void run_sessions(void)
{
    static tdsh_session_t session;   /* large: keep it off the stack */
    for (;;)
    {
        memset(&session, 0, sizeof(session));
        if (tdsh_session_init(&session, "root", true) != 0)
        {
            say("shell: cannot create a session\r\n");
            return;
        }
        session.terminal_caps = TDSH_TERM_CAP_ANSI | TDSH_TERM_CAP_COLOR;

        say("\033[2J\033[H\033[1;36mTinyDesk Shell " TDSH_VERSION "\033[0m running inside TinyDesk\r\n");
        say("Type 'help' for commands, 'exit' to restart the session.\r\n\r\n");

        for (;;)
        {
            char prompt[TDSH_MAX_PATH + 96];
            char line[TDSH_MAX_LINE + 1];
            build_prompt(&session, prompt, sizeof(prompt));
            int n = tdsh_terminal_readline(&session, &s_term_io, prompt, line, sizeof(line));
            if (n < 0)
                return;
            if (strcmp(line, "exit") == 0 || strcmp(line, "logout") == 0)
                break;
            if (n > 0)
                tdsh_execute_line(&session, line);
            if (session.logout_requested)
                break;
        }
        say("\r\n[session ended - press Enter for a new one]\r\n");
        uint8_t c = 0;
        while (c != '\r' && c != '\n')
        {
            if (term_read_byte(NULL, &c) != 0)
                return;
        }
    }
}

static void shell_main(void)
{
#ifdef _WIN32
    s_init_rc = tdsh_win_init(s_fs_root, s_hostname);
#else
    tdsh_posix_config_t cfg = TDSH_POSIX_CONFIG_DEFAULT();
    cfg.hostname = s_hostname;
    cfg.default_user = "root";
    cfg.fs_root = s_fs_root;
    s_init_rc = tdsh_posix_init(&cfg);
#endif
    if (s_init_rc != 0)
    {
        char msg[96];
        snprintf(msg, sizeof(msg), "The shell failed to start (error %d)\r\n", s_init_rc);
        say(msg);
        return;
    }
    td_proto_register_shell_commands();          /* mqtt, modbus */
    td_proto_set_break_check(break_requested);   /* Ctrl+C stops modbus read -i */
#if defined(TD_WITH_PYTHON)
    td_py_register_shell_command();           /* py (MicroPython) */
    td_py_set_break_check(break_requested);   /* Ctrl+C stops a script */
#endif

    /* Everything the shell prints goes to the Terminal window. */
    FILE *io = open_terminal_stream();
    if (!io)
    {
        say("shell: cannot open terminal stream\r\n");
        return;
    }
    setvbuf(io, NULL, _IONBF, 0);
    stdin = io;
    stdout = io;
    stderr = io;
    run_sessions();
}

#ifdef _WIN32
static DWORD WINAPI shell_thread(LPVOID arg)
{
    (void)arg;
    shell_main();
    return 0;
}
#else
static void *shell_thread(void *arg)
{
    (void)arg;
    shell_main();
    return NULL;
}
#endif

/* ----------------------------------------------------------- backend */

static bool s_started;

static int backend_start(void *ctx, int cols, int rows)
{
    (void)ctx;
    (void)rows;
    s_cols = cols;
    if (s_started)
        return 0;
    pipe_init(&s_in);
    pipe_init(&s_out);
#ifdef _WIN32
    HANDLE h = CreateThread(NULL, 1u << 20, shell_thread, NULL, 0, NULL);
    if (!h)
        return -1;
    CloseHandle(h);
#else
    pthread_t t;
    if (pthread_create(&t, NULL, shell_thread, NULL) != 0)
        return -1;
    pthread_detach(t);
#endif
    s_started = true;
    return 0;
}

static int backend_read(void *ctx, uint8_t *buf, int cap)
{
    (void)ctx;
    return s_started ? pipe_try_read(&s_out, buf, cap) : 0;
}

static int backend_write(void *ctx, const uint8_t *buf, int len)
{
    (void)ctx;
    return s_started ? pipe_try_write(&s_in, buf, len) : 0;
}

static void backend_resize(void *ctx, int cols, int rows)
{
    (void)ctx;
    (void)rows;
    s_cols = cols;
}

static const td_term_backend_t s_backend = {
    .name = "tdsh",
    .start = backend_start,
    .read = backend_read,
    .write = backend_write,
    .resize = backend_resize,
};

const td_term_backend_t *td_tdsh_host_backend(const char *fs_root, const char *hostname)
{
    snprintf(s_fs_root, sizeof(s_fs_root), "%s", fs_root);
    snprintf(s_hostname, sizeof(s_hostname), "%s", hostname ? hostname : "tinydesk");
    return &s_backend;
}
