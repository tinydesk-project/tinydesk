/*
 * td_py_time.h - the port part of MicroPython's time module (included by
 * extmod/modtime.c through MICROPY_PY_TIME_INCLUDEFILE): local time from
 * the C library, which follows the clock TinyDesk sets (SNTP or `date`).
 */
#include <time.h>

#include "shared/timeutils/timeutils.h"

/* time.time(): seconds since the Epoch. */
static mp_obj_t mp_time_time_get(void)
{
    return mp_obj_new_int_from_ll((long long)time(NULL));
}

/* time.localtime() and time.gmtime() fill a tuple from timeutils. */
static void mp_time_localtime_get(timeutils_struct_time_t *tm)
{
    time_t now = time(NULL);
    struct tm lt;
#if defined(_WIN32)
    localtime_s(&lt, &now);
#else
    localtime_r(&now, &lt);
#endif
    tm->tm_year = lt.tm_year + 1900;
    tm->tm_mon = lt.tm_mon + 1;
    tm->tm_mday = lt.tm_mday;
    tm->tm_hour = lt.tm_hour;
    tm->tm_min = lt.tm_min;
    tm->tm_sec = lt.tm_sec;
    tm->tm_wday = (lt.tm_wday + 6) % 7;   /* MicroPython: Monday is 0 */
    tm->tm_yday = lt.tm_yday + 1;
}
