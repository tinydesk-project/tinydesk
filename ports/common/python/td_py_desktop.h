/*
 * td_py_desktop.h - TinyDesk's own Python modules on top of TinyDesk
 * Shell's MicroPython (third_party/tdsh/python): tinydesk (td_py_mod.c),
 * _tdui under tdui and tdapp (td_py_tdui.c).
 */
#ifndef TD_PY_DESKTOP_H
#define TD_PY_DESKTOP_H

#include <stdbool.h>
#include <stddef.h>

/* After td_py_register_shell_command(): a script's windows close when it
 * ends. desktop_check returns true when the
 * calling task is the desktop's own console, so a script there may open
 * windows (NULL: tdui says there is no desktop). */
void td_py_desktop_setup(bool (*desktop_check)(void));

/* Optional: where tdui.Image.load() may keep decoded pictures, so the next
 * load of the same file at the same size skips decoding, scaling and
 * colour matching. dir writes a real folder path (which may not exist yet)
 * and returns true while there is one (the ESP32 ports: the SD card's
 * /sd/TinyDesk/cache), false for none. NULL: no cache. */
void td_py_set_image_cache(bool (*dir)(char *out, size_t cap));

/* For td_py_desktop_setup: the parts in td_py_tdui.c. */
void td_py_set_desktop_check(bool (*fn)(void));
void td_tdui_release(void);

#endif /* TD_PY_DESKTOP_H */
