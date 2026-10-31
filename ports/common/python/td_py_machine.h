/*
 * td_py_machine.h - shared by the machine module's files (td_py_machine.c
 * and the td_py_machine_*.h files extmod includes).
 */
#ifndef TD_PY_MACHINE_H
#define TD_PY_MACHINE_H

#include "py/obj.h"
#include "td_py_hw.h"

/* Hardware is for root (as the shell's gpio command): raises otherwise. */
void td_machine_need_root(void);

/* Raises OSError(err) unless err is NULL. */
void td_machine_check(const char *err);

/* A GPIO number from a Pin or an int, after td_machine_need_root() and
 * td_hw_pin_use() (output: the pin must be able to drive). */
int td_machine_pin_arg(void *obj, int output);

/* The script ended: interrupt handlers are forgotten, the hardware reset. */
void td_machine_release(void);

extern const mp_obj_type_t machine_pin_type;

#endif /* TD_PY_MACHINE_H */
