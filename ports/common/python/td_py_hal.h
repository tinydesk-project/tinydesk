/*
 * td_py_hal.h - MicroPython's hardware abstraction for TinyDesk (included
 * through MICROPY_MPHALPORT_H). The functions are in td_py.c.
 */
#ifndef TD_PY_HAL_H
#define TD_PY_HAL_H

#include <errno.h>

#include "shared/runtime/interrupt_char.h"

/* No pins: scripts use the tinydesk module, not hardware (and this keeps
 * extmod/virtpin.h out). */
#define mp_hal_pin_obj_t

#define mp_hal_ticks_cpu() (0)

/* For extmod/vfs_posix_file.c (as in MicroPython's Unix port, without its
 * GIL): retry a system call that a signal interrupted, else raise. */
#define MP_HAL_RETRY_SYSCALL(ret, syscall, raise)                                  \
    {                                                                              \
        for (;;)                                                                   \
        {                                                                          \
            ret = syscall;                                                         \
            if (ret == -1)                                                         \
            {                                                                      \
                int err = errno;                                                   \
                if (err == EINTR)                                                  \
                {                                                                  \
                    mp_handle_pending(MP_HANDLE_PENDING_CALLBACKS_AND_EXCEPTIONS); \
                    continue;                                                      \
                }                                                                  \
                raise;                                                             \
            }                                                                      \
            break;                                                                 \
        }                                                                          \
    }

#endif /* TD_PY_HAL_H */
