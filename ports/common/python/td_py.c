/*
 * td_py.c - the `py` command: MicroPython inside TinyDesk Shell.
 *
 *   py                     interactive prompt (Ctrl+D or exit() leaves it)
 *   py FILE [ARG...]       run a script (.py, or .mpy from mpy-cross);
 *                          sys.argv is [FILE, ARG...]
 *   py -c CODE [ARG...]    run CODE
 *   py -m KB ...           use a KB-kilobyte heap instead of the default
 *
 * The interpreter runs in the shell's own task, with the session's stdin
 * and stdout, so it works in the Terminal window, over Telnet and SSH, and
 * on the PC builds. Its heap is allocated for the run and freed afterwards:
 * nothing stays in RAM while no script runs. One script at a time
 * (MicroPython keeps its state in globals); a second `py` is told so.
 */
#include "td_py.h"

#include <errno.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "py/builtin.h"
#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/persistentcode.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/stream.h"
#include "shared/readline/readline.h"
#include "shared/runtime/gchelper.h"
#include "shared/runtime/pyexec.h"
#include "td_py_machine.h"
#include "td_py_vfs.h"

#if MICROPY_EMIT_NATIVE && !defined(ESP_PLATFORM)
#include <sys/mman.h>
#endif

#if defined(ESP_PLATFORM)
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#if defined(__XTENSA__)
#include "xtensa/hal.h"
#endif
#endif

/* Default heap sizes. With PSRAM (ESP32-WROVER, ESP32-S3) the heap goes
 * there and can be large; without it (ESP32-C6, 4 MB ESP32) it comes out
 * of internal RAM, and RAM_RESERVE is left for Wi-Fi, SSH and the desktop. */
#define HEAP_PSRAM_KB  512
#define HEAP_RAM_KB    64
#define HEAP_MIN_KB    16
#define RAM_RESERVE_KB 32
#define HEAP_PC_KB     1024

/* Stack the interpreter leaves free below its frames for C code it calls
 * (file system, printf, MQTT/TLS from the tinydesk module). */
#define STACK_MARGIN 6144

static bool (*s_break_check)(void);
static atomic_flag s_busy = ATOMIC_FLAG_INIT;
static tdsh_session_t *s_session;

const char td_py_help_text[] =
    "MicroPython in TinyDesk\n"
    "\n"
    "Run a script from the shell with:  py FILE [ARG...]\n"
    "Leave this prompt with Ctrl+D or exit(). Ctrl+C stops a running script.\n"
    "\n"
    "Modules: sys, os, time, json, re, math, random, binascii, hashlib, heapq,\n"
    "collections, struct, errno, gc, select, asyncio, micropython, machine and\n"
    "neopixel (pins and buses, for root), tdui (windows on the desktop) and\n"
    "tinydesk:\n"
    "  tinydesk.shell('ls -l')               run a shell command, get its status\n"
    "  tinydesk.log('text')                  add a line to the Log Viewer\n"
    "  tinydesk.mqtt_publish(topic, payload) publish on the connected broker\n"
    "  tinydesk.modbus_read(target, unit, table, addr, count=1)\n"
    "  tinydesk.modbus_write(target, unit, table, addr, values)\n"
    "Files: open(), os.listdir() etc. use TinyDesk paths (/home/root, ...).\n"
    "\n"
    "help('modules') lists the modules; help(obj) shows an object's members.\n";

void td_py_set_break_check(bool (*fn)(void))
{
    s_break_check = fn;
}

tdsh_session_t *td_py_session(void)
{
    return s_session;
}

/* ------------------------------------------------------------ HAL */

/* Called from the VM every few hundred bytecodes. A busy script runs in the
 * shell task: let it sleep 1 ms every 20 ms, so lower-priority tasks (the
 * idle task the watchdog watches, among them) still run. */
void td_py_poll(void)
{
    static uint64_t s_last_sleep;
    uint64_t now = tdsh_monotonic_ms();
    if (now - s_last_sleep >= 20)
    {
        tdsh_sleep_ms(1);
        s_last_sleep = tdsh_monotonic_ms();
    }
    if (s_break_check && s_break_check())
        mp_sched_keyboard_interrupt();
}

int mp_hal_stdin_rx_chr(void)
{
    int c = getchar();
    return c == EOF ? CHAR_CTRL_D : c;   /* end of input leaves the prompt */
}

mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len)
{
    size_t n = fwrite(str, 1, len, stdout);
    fflush(stdout);
    return n;
}

/* For sys.stdin/stdout's ioctl: output can always be written; whether a
 * key is waiting is not known from a stdio stream. */
uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags)
{
    return poll_flags & MP_STREAM_POLL_WR;
}

mp_uint_t mp_hal_ticks_ms(void)
{
    return (mp_uint_t)tdsh_monotonic_ms();
}

mp_uint_t mp_hal_ticks_us(void)
{
#if defined(ESP_PLATFORM)
    return (mp_uint_t)esp_timer_get_time();
#else
    return (mp_uint_t)(tdsh_monotonic_ms() * 1000u);
#endif
}

uint64_t mp_hal_time_ns(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000000ull + (uint64_t)tv.tv_usec * 1000ull;
}

/* Waiting for an event that may never come (select.poll, the asyncio loop
 * with every task asleep): sleep the shell task in slices of up to 10 ms.
 * The caller has just checked for Ctrl+C and calls again until its own
 * timeout has passed; -1 means no timeout. */
void td_py_wait_ms(int timeout_ms)
{
    if (timeout_ms != 0)
        tdsh_sleep_ms(timeout_ms < 0 || timeout_ms > 10 ? 10 : (uint32_t)timeout_ms);
}

/* Sleep in slices, so Ctrl+C stops time.sleep(60) at once. */
void mp_hal_delay_ms(mp_uint_t ms)
{
    uint64_t end = tdsh_monotonic_ms() + ms;
    for (;;)
    {
        mp_event_handle_nowait();
        uint64_t now = tdsh_monotonic_ms();
        if (now >= end)
            break;
        uint64_t left = end - now;
        tdsh_sleep_ms(left < 10 ? (uint32_t)left : 10);
    }
}

void mp_hal_delay_us(mp_uint_t us)
{
    if (us >= 1000)
        mp_hal_delay_ms(us / 1000);
#if defined(ESP_PLATFORM)
    int64_t end = esp_timer_get_time() + (us % 1000);
    while (esp_timer_get_time() < end)
    {
    }
#endif
}

void mp_hal_get_random(size_t n, uint8_t *buf)   /* os.urandom() */
{
    tdsh_random_bytes(buf, n);
}

uint32_t td_py_random_seed(void)
{
    uint32_t seed = 0;
    tdsh_random_bytes(&seed, sizeof(seed));
    return seed;
}

/* ------------------------------------------------- runtime glue */

void gc_collect(void)
{
    gc_collect_start();
#if defined(__XTENSA__)
    /* Registers of the callers' windows are not on the stack yet. */
    xthal_window_spill();
#endif
    gc_helper_collect_regs_and_stack();
    gc_collect_end();
}

void nlr_jump_fail(void *val)
{
    (void)val;
    printf("py: fatal error in the interpreter\n");
    abort();
}

/* ---------------------------------------------------- machine code */

/* Code from @micropython.native/viper and native .mpy files runs from
 * executable memory outside the Python heap: IRAM on the ESP32, an mmap()
 * on Linux. The blocks are freed when the script ends. Machine code can
 * read and write all of RAM, past the shell's users and file sandbox, so
 * only root may run it. */
typedef struct exec_block
{
    struct exec_block *next;
    size_t size;
    uint8_t code[];
} exec_block_t;

static exec_block_t *s_exec_blocks;

void *td_py_commit_exec(void *buf, size_t len, void *reloc)
{
#if MICROPY_EMIT_NATIVE
    if (!s_session || strcmp(s_session->username, "root") != 0)
        mp_raise_msg(&mp_type_RuntimeError, MP_ERROR_TEXT("machine code (native, viper, native .mpy) needs root"));
    len = (len + 3) & ~(size_t)3;
    size_t size = sizeof(exec_block_t) + len;
#if defined(ESP_PLATFORM)
    exec_block_t *b = heap_caps_malloc(size, MALLOC_CAP_EXEC);
#else
    exec_block_t *b = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (b == MAP_FAILED)
        b = NULL;
#endif
    if (!b)
        m_malloc_fail(size);
    b->size = size;
    b->next = s_exec_blocks;
    s_exec_blocks = b;
    if (reloc)
        mp_native_relocate(reloc, buf, (uintptr_t)b->code);
    memcpy(b->code, buf, len);
    return b->code;
#else
    (void)len;
    (void)reloc;
    return buf;   /* not called: no emitter here, and native .mpy files are refused */
#endif
}

static void exec_free_all(void)
{
    while (s_exec_blocks)
    {
        exec_block_t *b = s_exec_blocks;
        s_exec_blocks = b->next;
#if defined(ESP_PLATFORM)
        heap_caps_free(b);
#elif MICROPY_EMIT_NATIVE
        munmap(b, b->size);
#endif
    }
}

/* ------------------------------------------------------------ heap */

static void *heap_alloc(size_t want_kb, size_t *size)
{
#if defined(ESP_PLATFORM)
    size_t psram = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    if (psram >= HEAP_MIN_KB * 1024u)
    {
        size_t n = (want_kb ? want_kb : HEAP_PSRAM_KB) * 1024u;
        if (n > psram)
            n = psram;
        *size = n;
        return heap_caps_malloc(n, MALLOC_CAP_SPIRAM);
    }
    size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t total = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    size_t n = (want_kb ? want_kb : HEAP_RAM_KB) * 1024u;
    size_t room = total > RAM_RESERVE_KB * 1024u ? total - RAM_RESERVE_KB * 1024u : 0;
    if (n > room)
        n = room;
    if (n > largest)
        n = largest;
    if (n < HEAP_MIN_KB * 1024u)
    {
        printf("py: not enough RAM: %u KB free (largest block %u KB); Python needs %u KB and leaves %u KB "
               "for the system\n",
               (unsigned)(total / 1024), (unsigned)(largest / 1024), HEAP_MIN_KB, RAM_RESERVE_KB);
        return NULL;
    }
    *size = n & ~(size_t)15;
    return heap_caps_malloc(*size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
    *size = (want_kb ? want_kb : HEAP_PC_KB) * 1024u;
    return malloc(*size);
#endif
}

static void heap_free(void *heap)
{
#if defined(ESP_PLATFORM)
    heap_caps_free(heap);
#else
    free(heap);
#endif
}

/* The interpreter's C stack is the shell task's: frames may use all but
 * STACK_MARGIN of what is left below this function. */
static void set_stack(void *top)
{
    mp_stack_set_top(top);
#if defined(ESP_PLATFORM)
    size_t left = (size_t)((uint8_t *)top - pxTaskGetStackStart(NULL));
    mp_stack_set_limit(left > 2 * STACK_MARGIN ? left - STACK_MARGIN : left / 2);
#else
    mp_stack_set_limit(256 * 1024);   /* the PC shell thread has 1 MB or more */
#endif
}

/* ------------------------------------------------------------ run */

static void usage(void)
{
    printf("usage: py                    interactive prompt (Ctrl+D leaves)\n"
           "       py FILE [ARG...]      run a script (.py or .mpy)\n"
           "       py -c CODE [ARG...]   run CODE\n"
           "       py -m KB ...          heap size in KB\n");
}

/* sys.argv: first, then the arguments after it (as CPython: the script, or
 * "-c", or '' for the prompt). */
static void set_argv(const char *first, int argc, char **argv)
{
    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_argv), 0);
    mp_obj_list_append(mp_sys_argv, mp_obj_new_str(first, strlen(first)));
    for (int i = 0; i < argc; i++)
        mp_obj_list_append(mp_sys_argv, mp_obj_new_str(argv[i], strlen(argv[i])));
}

/* sys.path: the script's directory (or '' for the prompt), the frozen
 * modules (asyncio), then /lib. */
static void set_path(const char *script)
{
    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_path), 0);
    if (script)
    {
        const char *slash = strrchr(script, '/');
        size_t n = slash ? (size_t)(slash - script) : 0;
        mp_obj_list_append(mp_sys_path, n ? mp_obj_new_str(script, n) : MP_OBJ_NEW_QSTR(MP_QSTR__slash_));
    }
    else
    {
        mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_));
    }
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__dot_frozen));   /* asyncio */
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_lib));
}

static bool has_suffix(const char *s, const char *suffix)
{
    size_t n = strlen(s), m = strlen(suffix);
    return n >= m && !strcmp(s + n - m, suffix);
}

/* py FILE.mpy: load the compiled module and run it as __main__, with the
 * exit status pyexec_file() gives a .py file. */
static int run_mpy(const char *file)
{
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0)
    {
        qstr name = qstr_from_str(file);
        mp_store_global(MP_QSTR___file__, MP_OBJ_NEW_QSTR(name));
        mp_module_context_t *ctx = m_new_obj(mp_module_context_t);
        ctx->module.globals = mp_globals_get();
        mp_compiled_module_t cm;
        cm.context = ctx;
        mp_raw_code_load_file(name, &cm);
        mp_call_function_0(mp_make_function_from_proto_fun(cm.rc, ctx, NULL));
        mp_handle_pending(MP_HANDLE_PENDING_CALLBACKS_AND_EXCEPTIONS);
        nlr_pop();
        return 0;
    }
    mp_obj_t exc = MP_OBJ_FROM_PTR(nlr.ret_val);
    if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(mp_obj_get_type(exc)), MP_OBJ_FROM_PTR(&mp_type_SystemExit)))
    {
        mp_obj_t val = mp_obj_exception_get_value(exc);
        if (val == mp_const_none)
            return 0;
        if (mp_obj_is_int(val))
            return (int)mp_obj_int_get_truncated(val) & 0xff;
        mp_obj_print_helper(&mp_plat_print, val, PRINT_STR);
        mp_print_str(&mp_plat_print, "\n");
        return 1;
    }
    mp_obj_print_exception(&mp_plat_print, exc);
    if (mp_obj_is_subclass_fast(MP_OBJ_FROM_PTR(mp_obj_get_type(exc)), MP_OBJ_FROM_PTR(&mp_type_KeyboardInterrupt)))
        return PYEXEC_KEYBOARD_INTERRUPT & 0xff;
    return 1;
}

/* The REPL's history (Up/Down) lives on the heap of one run: keep it in
 * ~/.py_history between runs, as the shell keeps ~/.tdsh_history. One
 * entry per line, oldest first. */
#define HISTORY_FILE "~/.py_history"
#define HISTORY_LINE 256

static FILE *history_open(const char *mode)
{
    char real[TDSH_MAX_REAL_PATH];
    if (!s_session || tdsh_path_to_real(s_session, HISTORY_FILE, real, sizeof(real), NULL, 0) != 0)
        return NULL;
    return fopen(real, mode);
}

static void history_load(void)
{
    FILE *f = history_open("rb");
    if (!f)
        return;
    char line[HISTORY_LINE];
    while (fgets(line, sizeof(line), f))
    {
        size_t n = strcspn(line, "\r\n");
        if (line[n] == '\0' && !feof(f))
        {
            /* Longer than HISTORY_LINE: skip the rest of it. */
            int c;
            while ((c = fgetc(f)) != EOF && c != '\n')
                ;
            continue;
        }
        line[n] = '\0';
        readline_push_history(line);
    }
    fclose(f);
}

static void history_save(void)
{
    if (!MP_STATE_PORT(readline_hist)[0])
        return;
    FILE *f = history_open("wb");
    if (!f)
        return;
    for (int i = MICROPY_READLINE_HISTORY_SIZE - 1; i >= 0; i--)
    {
        const char *line = MP_STATE_PORT(readline_hist)[i];
        if (line && strlen(line) < HISTORY_LINE)
            fprintf(f, "%s\n", line);
    }
    fclose(f);
}

static int repl(void)
{
    for (;;)
    {
        int r = pyexec_mode_kind == PYEXEC_MODE_RAW_REPL ? pyexec_raw_repl() : pyexec_friendly_repl();
        if (r != 0)
            return r & PYEXEC_FORCED_EXIT ? r & 0xff : 0;
    }
}

/* Runs with the runtime up (see run_protected). argv holds the arguments
 * after FILE or CODE. Returns the shell exit status. */
static int run(const char *code, const char *file, int argc, char **argv)
{
    if (file)
    {
        set_path(file);
        set_argv(file, argc, argv);
        if (has_suffix(file, ".mpy"))
            return run_mpy(file);
        return pyexec_file(file) & 0xff;
    }
    set_path(NULL);
    if (code)
    {
        set_argv("-c", argc, argv);
        vstr_t src;
        vstr_init(&src, strlen(code));
        vstr_add_str(&src, code);
        return pyexec_vstr(&src, true) & 0xff;
    }
    set_argv("", 0, NULL);
    readline_init0();
    history_load();
    int status = repl();
    history_save();
    return status;
}

/* run() inside an nlr frame (kept out of cmd_py so its locals are not live
 * across the longjmp). */
static int run_protected(const char *cwd, const char *code, const char *file, int argc, char **argv)
{
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0)
    {
        td_py_vfs_mount_root(cwd);
        int status = run(code, file, argc, argv);
        nlr_pop();
        return status;
    }
    mp_obj_print_exception(&mp_plat_print, MP_OBJ_FROM_PTR(nlr.ret_val));
    return 1;
}

static int cmd_py(tdsh_session_t *session, int argc, char **argv)
{
    volatile int stack_top = 0;   /* the interpreter's frames start below this */
    size_t heap_kb = 0;
    const char *code = NULL;
    int i = 1;
    while (i < argc && argv[i][0] == '-' && argv[i][1])
    {
        if (!strcmp(argv[i], "-m") && i + 1 < argc)
        {
            char *end = NULL;
            long kb = strtol(argv[i + 1], &end, 10);
            if (!end || *end || kb < HEAP_MIN_KB || kb > 65536)
            {
                printf("py: the heap size is in KB, %d to 65536\n", HEAP_MIN_KB);
                return 2;
            }
            heap_kb = (size_t)kb;
            i += 2;
        }
        else if (!strcmp(argv[i], "-c") && i + 1 < argc)
        {
            code = argv[i + 1];
            i += 2;
            break;
        }
        else
        {
            usage();
            return 2;
        }
    }

    /* The script file, as a TinyDesk path (relative to the shell's cwd). */
    char file[TDSH_MAX_PATH];
    if (!code && i < argc)
    {
        char real[TDSH_MAX_REAL_PATH];
        struct stat st;
        if (tdsh_path_to_real(session, argv[i], real, sizeof(real), file, sizeof(file)) != 0)
        {
            printf("py: bad path %s\n", argv[i]);
            return 2;
        }
        if (stat(real, &st) != 0)
        {
            printf("py: %s: %s\n", argv[i], strerror(errno));
            return 2;
        }
        if (S_ISDIR(st.st_mode))
        {
            printf("py: %s: is a directory\n", argv[i]);
            return 2;
        }
    }
    bool have_file = !code && i < argc;

    if (atomic_flag_test_and_set(&s_busy))
    {
        printf("py: Python is already running (one script at a time)\n");
        return 1;
    }
    size_t heap_size = 0;
    void *heap = heap_alloc(heap_kb, &heap_size);
    if (!heap)
    {
        if (heap_size)
            printf("py: cannot allocate a %u KB heap\n", (unsigned)(heap_size / 1024));
        atomic_flag_clear(&s_busy);
        return 1;
    }
    s_session = session;

    set_stack((void *)&stack_top);
    gc_init(heap, (uint8_t *)heap + heap_size);
    mp_init();

    int status;
    if (code)
        status = run_protected(session->cwd, code, NULL, argc - i, argv + i);
    else if (have_file)
        status = run_protected(session->cwd, NULL, file, argc - i - 1, argv + i + 1);
    else
        status = run_protected(session->cwd, NULL, NULL, 0, NULL);

    td_machine_release();   /* pins, buses and interrupts the script opened */
    td_tdui_release();      /* its windows */
    gc_sweep_all();         /* runs finalisers: closes files the script left open */
    mp_deinit();
    exec_free_all();
    heap_free(heap);
    s_session = NULL;
    atomic_flag_clear(&s_busy);
    return status;
}

static const tdsh_command_t s_cmd = {
    "py",
    "py [-m KB] [FILE [ARG...] | -c CODE [ARG...]]",
    "MicroPython: run a script, or the interactive prompt",
    cmd_py,
    0,
};

int td_py_register_shell_command(void)
{
    return tdsh_register_command(&s_cmd);
}
