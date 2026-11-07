/*
 * td_py_desktop.h - TinyDesk's own Python modules on top of TinyDesk
 * Shell's MicroPython (third_party/tdsh/python): tinydesk (td_py_mod.c),
 * _tdui under tdui and tdapp (td_py_tdui.c).
 */
#ifndef TD_PY_DESKTOP_H
#define TD_PY_DESKTOP_H

#include <stdbool.h>

/* After td_py_register_shell_command(): a script's windows close when it
 * ends. desktop_check returns true when the
 * calling task is the desktop's own console, so a script there may open
 * windows (NULL: tdui says there is no desktop). */
void td_py_desktop_setup(bool (*desktop_check)(void));

/* For td_py_desktop_setup: the parts in td_py_tdui.c. */
void td_py_set_desktop_check(bool (*fn)(void));
void td_tdui_release(void);

#endif /* TD_PY_DESKTOP_H */
