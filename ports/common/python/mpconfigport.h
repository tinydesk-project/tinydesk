/*
 * mpconfigport.h - MicroPython configuration for TinyDesk's `py` command.
 *
 * One configuration for every TinyDesk target (ESP32-C6, ESP32, Linux,
 * Windows), so a script behaves the same on a board and on a PC, and the
 * generated headers in third_party/micropython/genhdr fit all of them.
 * After changing anything here, or a MP_QSTR_ / MP_REGISTER_ in this
 * directory, regenerate them: see tools/micropython/README.md.
 */
#ifndef TD_PY_MPCONFIGPORT_H
#define TD_PY_MPCONFIGPORT_H

#include <stdint.h>

#if defined(_WIN32)
#include <malloc.h>   /* alloca */
#else
#include <alloca.h>
#endif

/* The feature level MicroPython's own ESP32 port starts from. */
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES)

/* Portable building blocks: setjmp for exceptions and for finding the
 * registers the garbage collector scans (Xtensa also spills its register
 * windows first, see gc_collect() in td_py.c). */
#define MICROPY_NLR_SETJMP       (1)
#define MICROPY_GCREGS_SETJMP    (1)
#define MICROPY_ENABLE_GC        (1)
#define MICROPY_ENABLE_FINALISER (1)   /* files close when collected */
#define MICROPY_STACK_CHECK      (1)
#define MICROPY_ENABLE_COMPILER  (1)
#define MICROPY_ALLOC_PATH_MAX   (256)

/* Numbers: big integers, single-precision floats (the ESP32 has a
 * single-precision FPU; the same on every target keeps results equal). */
#define MICROPY_LONGINT_IMPL (MICROPY_LONGINT_IMPL_MPZ)
#define MICROPY_FLOAT_IMPL   (MICROPY_FLOAT_IMPL_FLOAT)

/* No native code emitters, no threads, no hardware modules: TinyDesk owns
 * the hardware, and scripts reach it through the tinydesk module. */
#define MICROPY_PY_THREAD            (0)
#define MICROPY_PY_MACHINE           (0)
#define MICROPY_PY_NETWORK           (0)
#define MICROPY_PY_SOCKET            (0)
#define MICROPY_PY_SELECT            (0)
#define MICROPY_PY_ASYNCIO           (0)
#define MICROPY_PY_DEFLATE           (0)
#define MICROPY_PY_FRAMEBUF          (0)
#define MICROPY_PY_HASHLIB           (0)
#define MICROPY_PY_CRYPTOLIB         (0)
#define MICROPY_PY_UCTYPES           (0)
#define MICROPY_PY_BLUETOOTH         (0)
#define MICROPY_PY_WEBSOCKET         (0)
#define MICROPY_PY_WEBREPL           (0)
#define MICROPY_PY_BTREE             (0)
#define MICROPY_PY_PLATFORM          (0)
#define MICROPY_PY_VFS               (0)   /* no vfs module: scripts cannot mount */
#define MICROPY_PY_IO_BUFFEREDWRITER (0)
#define MICROPY_PY_SYS_STDIO_BUFFER  (0)
#define MICROPY_PY_SYS_EXIT          (1)
#define MICROPY_PY_SYS_ARGV          (1)
#define MICROPY_PY_SYS_PLATFORM      "tinydesk"
#define MICROPY_PERSISTENT_CODE_LOAD (0)
#define MICROPY_ENABLE_SCHEDULER     (0)
#define MICROPY_KBD_EXCEPTION        (1)   /* Ctrl+C raises KeyboardInterrupt */

/* The REPL (shared/runtime/pyexec.c with shared/readline). */
#define MICROPY_HELPER_REPL                      (1)
#define MICROPY_PYEXEC_ENABLE_EXIT_CODE_HANDLING (1)   /* sys.exit(n) is the status */
#define MICROPY_REPL_AUTO_INDENT                 (1)
#define MICROPY_READLINE_HISTORY_SIZE            (8)
#define MICROPY_PY_BUILTINS_INPUT                (1)
#define MICROPY_PY_BUILTINS_HELP                 (1)
#define MICROPY_PY_BUILTINS_HELP_TEXT            td_py_help_text
#define MICROPY_PY_BUILTINS_HELP_MODULES         (1)
#define MICROPY_PY_MICROPYTHON_MEM_INFO          (1)
#define MICROPY_HW_BOARD_NAME                    "TinyDesk"
/* A string literal (the banner pastes it): the chip TinyDesk runs on. */
#if defined(__XTENSA__)
#define MICROPY_HW_MCU_NAME "ESP32"
#elif defined(__riscv)
#define MICROPY_HW_MCU_NAME "ESP32-C6"
#elif defined(_WIN32)
#define MICROPY_HW_MCU_NAME "Windows"
#else
#define MICROPY_HW_MCU_NAME "Linux"
#endif

/* Files: the TinyDesk file system through TinyDesk Shell's path rules
 * (td_py_vfs.c). The POSIX file objects of extmod/vfs_posix_file.c do the
 * reading and writing; VfsPosix itself is replaced by the sandboxed type. */
#define MICROPY_VFS                          (1)
#define MICROPY_VFS_POSIX                    (1)
#define MICROPY_VFS_POSIX_WRITABLE           (1)
#define MICROPY_READER_VFS                   (1)
#define MICROPY_PY_OS                        (1)
#define MICROPY_PY_OS_STATVFS                (0)
#define MICROPY_PY_OS_UNAME                  (1)
#define MICROPY_PY_OS_URANDOM                (0)
#define MICROPY_PY_OS_DUPTERM                (0)
#define MICROPY_PY_OS_SYNC                   (0)
#define MICROPY_PY_OS_GETENV_PUTENV_UNSETENV (0)
#define MICROPY_PY_OS_SYSTEM                 (0)

/* Modules from extmod/ that TinyDesk builds. */
#define MICROPY_PY_TIME                         (1)
#define MICROPY_PY_TIME_TIME_TIME_NS            (1)
#define MICROPY_PY_TIME_GMTIME_LOCALTIME_MKTIME (1)
#define MICROPY_PY_TIME_INCLUDEFILE             "td_py_time.h"
#define MICROPY_EPOCH_IS_1970                   (1)   /* time.time() matches the C library */
#define MICROPY_PY_JSON                         (1)
#define MICROPY_PY_RE                           (1)
#define MICROPY_PY_BINASCII                     (1)
#define MICROPY_PY_HEAPQ                        (1)
#define MICROPY_PY_RANDOM                       (1)
#define MICROPY_PY_RANDOM_EXTRA_FUNCS           (1)
#define MICROPY_PY_RANDOM_SEED_INIT_FUNC        (td_py_random_seed())
extern uint32_t td_py_random_seed(void);

/* Ctrl+C: the VM asks TinyDesk every few hundred bytecodes. */
#define MICROPY_VM_HOOK_COUNT (64)
#define MICROPY_VM_HOOK_INIT  static unsigned int vm_hook_divisor = MICROPY_VM_HOOK_COUNT;
#define MICROPY_VM_HOOK_POLL                     \
    if (--vm_hook_divisor == 0)                  \
    {                                            \
        vm_hook_divisor = MICROPY_VM_HOOK_COUNT; \
        td_py_poll();                            \
    }
#define MICROPY_VM_HOOK_LOOP        MICROPY_VM_HOOK_POLL
#define MICROPY_VM_HOOK_RETURN      MICROPY_VM_HOOK_POLL
#define MICROPY_INTERNAL_EVENT_HOOK td_py_poll()
extern void td_py_poll(void);

#define MICROPY_MPHALPORT_H "td_py_hal.h"
#define MP_STATE_PORT       MP_STATE_VM

/* wolfSSL (TinyDesk Shell's SSH server) has a big-number mp_init() too;
 * every MicroPython file includes this header, so rename MicroPython's. */
#define mp_init td_mp_runtime_init

typedef long mp_off_t;
/* newlib does not define SSIZE_MAX by default; mp_int_t is pointer-sized. */
#define MP_SSIZE_MAX (INTPTR_MAX)

#endif /* TD_PY_MPCONFIGPORT_H */
