/*
 * td_fs_stdio.h - Files-app backend using the standard C library.
 */
#ifndef TD_FS_STDIO_H
#define TD_FS_STDIO_H

#include "tinydesk/td_sysinfo.h"

/* Filesystem operations rooted at `root` (the string is copied). */
const td_fs_ops_t *td_fs_stdio(const char *root);
void td_fs_stdio_guard(bool (*allow)(const char *path, int action));

/* Serve the folder `from` (a path below the root) from `to` while
 * active() returns true: the ESP-IDF port shows a mounted SD card, which
 * lives at /sd in the VFS, as the folder "sd" (/fs/sd). The folder itself
 * cannot be removed or renamed through these operations. One redirect. */
void td_fs_stdio_redirect(const char *from, const char *to, bool (*active)(void));

#endif
