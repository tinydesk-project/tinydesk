/*
 * td_py.h - the `py` command: MicroPython scripts in TinyDesk Shell.
 */
#ifndef TD_PY_H
#define TD_PY_H

#include <stdbool.h>

#include "tdsh.h"

/* Register `py` with TinyDesk Shell (call once, before sessions start). */
int td_py_register_shell_command(void);

/* Ctrl+C: a function that returns true once the user pressed it on the
 * console running the script (the same check `modbus read -i` uses). Without
 * one, a running script cannot be stopped from the keyboard. */
void td_py_set_break_check(bool (*fn)(void));

/* Windows (the tdui module): a function that returns true when the calling
 * task is the desktop's own console, so a script there may open windows.
 * Without one, tdui says there is no desktop. */
void td_py_set_desktop_check(bool (*fn)(void));

/* For td_py_vfs.c and td_py_mod.c: the session running the script. */
tdsh_session_t *td_py_session(void);

/* For td_py.c: close the windows a script opened (td_py_tdui.c). */
void td_tdui_release(void);

#endif /* TD_PY_H */
