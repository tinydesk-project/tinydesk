/*
 * td_py_vfs.h - the TinyDesk file system for MicroPython (td_py_vfs.c).
 */
#ifndef TD_PY_VFS_H
#define TD_PY_VFS_H

#include "py/obj.h"

/* Mount the TinyDesk file system at "/" and change to cwd (a TinyDesk
 * path such as /home/root). Paths go through TinyDesk Shell's rules, so a
 * script reaches the same files as the shell's commands, and no others. */
void td_py_vfs_mount_root(const char *cwd);

#endif /* TD_PY_VFS_H */
