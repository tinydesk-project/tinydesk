/*
 * poll.h - extmod/vfs_posix_file.c includes <poll.h>; ESP-IDF has it as
 * <sys/poll.h>. (poll() itself is only used with the select module, which
 * TinyDesk leaves out.)
 */
#pragma once
#include <sys/poll.h>
