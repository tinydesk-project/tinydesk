/*
 * td_py_hal.h - MicroPython's hardware abstraction for TinyDesk (included
 * through MICROPY_MPHALPORT_H). The functions are in td_py.c.
 */
#ifndef TD_PY_HAL_H
#define TD_PY_HAL_H

#include <errno.h>

#include "shared/runtime/interrupt_char.h"

#include <stddef.h>
#include <stdint.h>

#include "td_py_hw.h"

/* Pins are GPIO numbers (the machine module, td_py_machine.c). Getting one
 * from a script's argument checks that the script may use it. */
#define mp_hal_pin_obj_t int
int td_machine_pin_arg(void *obj, int output);   /* mp_obj_t */
#define mp_hal_get_pin_obj(o)    td_machine_pin_arg((void *)(o), 0)
#define mp_hal_pin_name(p)       ((unsigned)(p))
#define MP_HAL_PIN_FMT           "%u"
#define mp_hal_pin_input(p)      ((void)td_hw_pin_mode((p), TD_HW_IN, TD_HW_PULL_NONE))
#define mp_hal_pin_output(p)     ((void)td_hw_pin_mode((p), TD_HW_OUT, TD_HW_PULL_NONE))
#define mp_hal_pin_open_drain(p) ((void)td_hw_pin_mode((p), TD_HW_OPEN_DRAIN, TD_HW_PULL_NONE))
#define mp_hal_pin_od_low(p)     td_hw_pin_write((p), 0)
#define mp_hal_pin_od_high(p)    td_hw_pin_write((p), 1)
#define mp_hal_pin_read(p)       td_hw_pin_read(p)
#define mp_hal_pin_write(p, v)   td_hw_pin_write((p), (v))
#define mp_hal_delay_us_fast(us) mp_hal_delay_us(us)

#define mp_hal_ticks_cpu() (0)

/* os.urandom(): TinyDesk Shell's random source (td_py.c). */
void mp_hal_get_random(size_t n, uint8_t *buf);

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
