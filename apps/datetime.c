/*
 * datetime.c - the taskbar clock and the "Date & time" window.
 *
 * The clock text follows the current user's preferences (12/24-hour, date
 * order, shown or hidden) and time zone. Clicking it opens the window;
 * right-clicking opens a small menu, like on Windows.
 *
 * The time zone is per user (TinyDesk Shell's `tz` file). The system clock and the
 * automatic (network) time are device-wide, so only root may change them.
 */
#include <stdio.h>
#include <string.h>

#include "td_apps.h"

/* ------------------------------------------------------ calendar maths */

static const char *const s_months[12] = {
    "January",
    "February",
    "March",
    "April",
    "May",
    "June",
    "July",
    "August",
    "September",
    "October",
    "November",
    "December",
};
static const char *const s_weekdays[7] = {
    "Thursday",
    "Friday",
    "Saturday",
    "Sunday",
    "Monday",
    "Tuesday",
    "Wednesday",
};

typedef struct
{
    int year, month, day;     /* month 1..12 */
    int hour, min, sec;
    int weekday;              /* index into s_weekdays (0 = Thursday) */
} civil_t;

/* Days since 1970-01-01 -> y/m/d (H. Hinnant's algorithm, no <time.h>). */
static void civil_from_days(long long z, int *y, int *m, int *d)
{
    z += 719468;
    long long era = (z >= 0 ? z : z - 146096) / 146097;
    long long doe = z - era * 146097;
    long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    long long mp = (5 * doy + 2) / 153;
    *d = (int)(doy - (153 * mp + 2) / 5 + 1);
    *m = (int)(mp < 10 ? mp + 3 : mp - 9);
    *y = (int)(yoe + era * 400 + (*m <= 2));
}

static long long days_from_civil(int y, int m, int d)
{
    y -= m <= 2;
    long long era = (y >= 0 ? y : y - 399) / 400;
    long long yoe = y - era * 400;
    long long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

static void civil_from_seconds(long long t, civil_t *c)
{
    long long days = t >= 0 ? t / 86400 : -((-t + 86399) / 86400);
    long long rem = t - days * 86400;
    civil_from_days(days, &c->year, &c->month, &c->day);
    c->hour = (int)(rem / 3600);
    c->min = (int)(rem / 60 % 60);
    c->sec = (int)(rem % 60);
    c->weekday = (int)(((days % 7) + 7) % 7);
}

static long user_tz(void)
{
    long tz = 0;
    const td_sysinfo_t *si = td_sysinfo();
    if (si->get_tz)
        si->get_tz(&tz);
    return tz;
}

/* The local time now; false while the clock is not set. */
static bool local_now(civil_t *c)
{
    const td_sysinfo_t *si = td_sysinfo();
    int64_t utc;
    if (!si->time_now || !si->time_now(&utc))
        return false;
    civil_from_seconds((long long)utc + user_tz(), c);
    return true;
}

static void format_time(const civil_t *c, bool seconds, char *buf, size_t cap)
{
    const td_clock_prefs_t *p = td_clock_prefs();
    if (p->clock_12h)
    {
        int h = c->hour % 12 ? c->hour % 12 : 12;
        if (seconds)
            snprintf(buf, cap, "%d:%02d:%02d %s", h, c->min, c->sec, c->hour < 12 ? "AM" : "PM");
        else
            snprintf(buf, cap, "%d:%02d %s", h, c->min, c->hour < 12 ? "AM" : "PM");
    }
    else if (seconds)
    {
        snprintf(buf, cap, "%02d:%02d:%02d", c->hour, c->min, c->sec);
    }
    else
    {
        snprintf(buf, cap, "%02d:%02d", c->hour, c->min);
    }
}

static const char *const s_date_formats[TD_DATE_FORMATS] = {"DD-MM-YYYY", "YYYY-MM-DD", "MM/DD/YYYY"};

static void format_date(const civil_t *c, char *buf, size_t cap)
{
    switch (td_clock_prefs()->date_format)
    {
    case TD_DATE_YMD:
        snprintf(buf, cap, "%04d-%02d-%02d", c->year, c->month, c->day);
        break;
    case TD_DATE_MDY:
        snprintf(buf, cap, "%02d/%02d/%04d", c->month, c->day, c->year);
        break;
    default:
        snprintf(buf, cap, "%02d-%02d-%04d", c->day, c->month, c->year);
        break;
    }
}

/* ---------------------------------------------------------- time zones */

typedef struct
{
    int minutes;              /* east of UTC */
    const char *places;
} zone_t;

/* Fixed offsets (TinyDesk Shell's tz has no daylight saving). */
static const zone_t s_zones[] = {
    {-720, "International Date Line West"},
    {-660, "Coordinated Universal Time-11"},
    {-600, "Hawaii"},
    {-540, "Alaska"},
    {-480, "Pacific Time (US & Canada)"},
    {-420, "Mountain Time (US & Canada)"},
    {-360, "Central Time (US & Canada), Mexico City"},
    {-300, "Eastern Time (US & Canada), Bogota, Lima"},
    {-240, "Atlantic Time (Canada), Caracas, La Paz"},
    {-210, "Newfoundland"},
    {-180, "Brasilia, Buenos Aires, Montevideo"},
    {-120, "Coordinated Universal Time-02"},
    {-60, "Azores, Cabo Verde Is."},
    {0, "Coordinated Universal Time, London, Lisbon"},
    {60, "Berlin, Paris, Rome, Madrid, Lagos"},
    {120, "Athens, Cairo, Helsinki, Johannesburg"},
    {180, "Moscow, Istanbul, Riyadh, Nairobi"},
    {210, "Tehran"},
    {240, "Abu Dhabi, Muscat, Baku, Tbilisi"},
    {270, "Kabul"},
    {300, "Islamabad, Karachi, Tashkent"},
    {330, "Chennai, Kolkata, Mumbai, New Delhi"},
    {345, "Kathmandu"},
    {360, "Dhaka, Astana"},
    {390, "Yangon (Rangoon)"},
    {420, "Bangkok, Hanoi, Jakarta"},
    {480, "Beijing, Hong Kong, Singapore, Perth"},
    {540, "Tokyo, Seoul, Osaka"},
    {570, "Adelaide, Darwin"},
    {600, "Sydney, Melbourne, Brisbane, Guam"},
    {660, "Solomon Is., New Caledonia"},
    {720, "Auckland, Wellington, Fiji"},
    {780, "Nuku'alofa, Samoa"},
    {840, "Kiritimati Island"},
};
#define ZONE_COUNT ((int)(sizeof(s_zones) / sizeof(s_zones[0])))

static void format_offset(long secs, char *buf, size_t cap)
{
    unsigned long a = secs < 0 ? 0UL - (unsigned long)secs : (unsigned long)secs;
    unsigned h = (unsigned)(a / 3600 % 100), m = (unsigned)(a / 60 % 60);   /* zones are within +-14 h */
    snprintf(buf, cap, "UTC%c%02u:%02u", secs < 0 ? '-' : '+', h, m);
}

static int zone_index(long secs)
{
    for (int i = 0; i < ZONE_COUNT; i++)
        if (s_zones[i].minutes * 60L == secs)
            return i;
    return -1;
}

/* "(UTC+05:30) Chennai, Kolkata, Mumbai, New Delhi" */
static void zone_text(long secs, char *buf, size_t cap)
{
    char off[16];
    format_offset(secs, off, sizeof(off));
    int i = zone_index(secs);
    snprintf(buf, cap, "(%s) %s", off, i >= 0 ? s_zones[i].places : "");
}

/* The time of day of a UTC moment in the user's zone and clock format
 * ("14:05:09" or "2:05:09 PM"); "" for 0 (clock not set). */
void td_time_of_day(int64_t utc, char *buf, int cap)
{
    buf[0] = '\0';
    if (utc <= 0)
        return;
    civil_t c;
    civil_from_seconds((long long)utc + user_tz(), &c);
    format_time(&c, true, buf, (size_t)cap);
}

void td_date_time(int64_t utc, char *buf, int cap)
{
    buf[0] = '\0';
    if (utc <= 0)
        return;
    civil_t c;
    char d[16], t[16];
    civil_from_seconds((long long)utc + user_tz(), &c);
    format_date(&c, d, sizeof(d));
    format_time(&c, false, t, sizeof(t));
    snprintf(buf, (size_t)cap, "%s %s", d, t);
}

/* ------------------------------------------------------ taskbar clock */

static bool clock_text(char *buf, int cap)
{
    if (td_clock_prefs()->hide_clock)
        return false;
    civil_t c;
    if (!local_now(&c))
    {
        snprintf(buf, (size_t)cap, "--:-- no clock");
        return true;
    }
    char t[16], d[16];
    format_time(&c, false, t, sizeof(t));
    format_date(&c, d, sizeof(d));
    snprintf(buf, (size_t)cap, "%s %s", t, d);
    return true;
}

/* The same, apart (small and large taskbars). */
static bool clock_parts(char *time, int tcap, char *date, int dcap)
{
    if (td_clock_prefs()->hide_clock)
        return false;
    civil_t c;
    if (!local_now(&c))
    {
        snprintf(time, (size_t)tcap, "--:--");
        snprintf(date, (size_t)dcap, "no clock");
        return true;
    }
    format_time(&c, false, time, (size_t)tcap);
    format_date(&c, date, (size_t)dcap);
    return true;
}

static void open_zone_picker(void);
static void sync_now(void);

static void clock_menu_chosen(int item, void *user)
{
    (void)user;
    switch (item)
    {
    case 0:
        td_datetime_open();
        break;
    case 1:
        open_zone_picker();
        break;
    case 2:
        sync_now();
        break;
    case 4:
        td_app_launch("Settings");
        break;
    default:
        break;
    }
}

static void clock_click(int button, int x, int y)
{
    if (button == TD_BUTTON_RIGHT)
    {
        static const char *const items[] = {
            "Adjust date and time",
            "Change time zone...",
            "Sync time now",
            "-",
            "Settings",
        };
        td_menu_popup(x, y, items, 5, clock_menu_chosen, NULL);
    }
    else
    {
        td_datetime_open();
    }
}

static const td_clock_provider_t s_clock = {clock_text, clock_click, clock_parts};

void td_datetime_install_clock(void)
{
    td_wm_set_clock(&s_clock);
}

/* ------------------------------------------------------ Date & time */

static td_window_t *s_win, *s_picker;
static td_widget_t *s_zone_btn, *s_auto, *s_sync_btn, *s_manual_btn, *s_show, *s_h24, *s_h12, *s_fmt_btn;
static td_widget_t *s_zone_info, *s_msg, *s_zone_list;

/* 3x3 block digits for the big clock. */
static const char *const s_big[11][3] = {
    {"\xE2\x96\x88\xE2\x96\x80\xE2\x96\x88", "\xE2\x96\x88 \xE2\x96\x88", "\xE2\x96\x80\xE2\x96\x80\xE2\x96\x80"},
    {"\xE2\x96\x80\xE2\x96\x88 ", " \xE2\x96\x88 ", "\xE2\x96\x80\xE2\x96\x80\xE2\x96\x80"},
    {"\xE2\x96\x80\xE2\x96\x80\xE2\x96\x88", "\xE2\x96\x88\xE2\x96\x80\xE2\x96\x80", "\xE2\x96\x80\xE2\x96\x80\xE2\x96\x80"},
    {"\xE2\x96\x80\xE2\x96\x80\xE2\x96\x88", " \xE2\x96\x80\xE2\x96\x88", "\xE2\x96\x80\xE2\x96\x80\xE2\x96\x80"},
    {"\xE2\x96\x88 \xE2\x96\x88", "\xE2\x96\x80\xE2\x96\x80\xE2\x96\x88", "  \xE2\x96\x80"},
    {"\xE2\x96\x88\xE2\x96\x80\xE2\x96\x80", "\xE2\x96\x80\xE2\x96\x80\xE2\x96\x88", "\xE2\x96\x80\xE2\x96\x80\xE2\x96\x80"},
    {"\xE2\x96\x88\xE2\x96\x80\xE2\x96\x80", "\xE2\x96\x88\xE2\x96\x80\xE2\x96\x88", "\xE2\x96\x80\xE2\x96\x80\xE2\x96\x80"},
    {"\xE2\x96\x80\xE2\x96\x80\xE2\x96\x88", "  \xE2\x96\x88", "  \xE2\x96\x80"},
    {"\xE2\x96\x88\xE2\x96\x80\xE2\x96\x88", "\xE2\x96\x88\xE2\x96\x80\xE2\x96\x88", "\xE2\x96\x80\xE2\x96\x80\xE2\x96\x80"},
    {"\xE2\x96\x88\xE2\x96\x80\xE2\x96\x88", "\xE2\x96\x80\xE2\x96\x80\xE2\x96\x88", "\xE2\x96\x80\xE2\x96\x80\xE2\x96\x80"},
    {"\xE2\x96\x84", "\xE2\x96\x84", " "},   /* ':' */
};

static int draw_big(int x, int y, int glyph, uint8_t fg, uint8_t bg)
{
    for (int r = 0; r < 3; r++)
        td_text(x, y + r, s_big[glyph][r], fg, bg, 0);
    return glyph == 10 ? 2 : 4;
}

static void on_draw(td_window_t *win, int w, int h)
{
    (void)win;
    (void)w;
    (void)h;
    const td_theme_t *t = td_theme();
    civil_t c;
    if (!local_now(&c))
    {
        td_text(1, 1, "The clock is not set yet.", t->dim, t->win_bg, 0);
        td_text(1, 2, "It is set from the network once Wi-Fi is connected.", t->dim, t->win_bg, 0);
        return;
    }
    const td_clock_prefs_t *p = td_clock_prefs();
    int hour = p->clock_12h ? (c.hour % 12 ? c.hour % 12 : 12) : c.hour;
    int x = 1;
    if (!p->clock_12h || hour >= 10)
        x += draw_big(x, 0, hour / 10, t->accent, t->win_bg);
    x += draw_big(x, 0, hour % 10, t->accent, t->win_bg);
    x += draw_big(x, 0, 10, t->accent, t->win_bg);
    x += draw_big(x, 0, c.min / 10, t->accent, t->win_bg);
    x += draw_big(x, 0, c.min % 10, t->accent, t->win_bg);
    char small[16];
    snprintf(small, sizeof(small), ":%02d%s", c.sec, p->clock_12h ? (c.hour < 12 ? " AM" : " PM") : "");
    td_text(x, 1, small, t->win_fg, t->win_bg, 0);

    char date[48];
    snprintf(date, sizeof(date), "%s, %d %s %d", s_weekdays[c.weekday], c.day, s_months[c.month - 1], c.year);
    td_text(1, 3, date, t->win_fg, t->win_bg, 0);
}

static void refresh(void)
{
    if (!td_win_is_open(s_win))
        return;
    const td_sysinfo_t *si = td_sysinfo();
    const td_clock_prefs_t *p = td_clock_prefs();
    char zone[64], off[16];
    long tz = user_tz();
    zone_text(tz, zone, sizeof(zone));
    format_offset(tz, off, sizeof(off));
    td_widget_printf(s_zone_info, "%s, the time zone of %.24s", off, td_session_user());
    td_widget_set_text(s_zone_btn, zone);
    if (s_auto)
        td_checkbox_set(s_auto, si->time_auto_get && si->time_auto_get());
    td_checkbox_set(s_show, !p->hide_clock);
    td_checkbox_set(s_h24, !p->clock_12h);
    td_checkbox_set(s_h12, p->clock_12h);
    td_widget_set_text(s_fmt_btn, s_date_formats[p->date_format % TD_DATE_FORMATS]);
    td_win_invalidate(s_win);
    td_wm_invalidate();
}

static void say(const char *text)
{
    if (td_win_is_open(s_win))
        td_widget_set_text(s_msg, text);
}

static void changed_prefs(void)
{
    say(td_settings_save() ? "Saved." : "Changed (saving is not available here).");
    refresh();
}

/* --- time zone picker */

static const char *zone_item(td_widget_t *w, int index, int *fg, void *user)
{
    (void)w;
    (void)fg;
    (void)user;
    static char buf[64];
    zone_text(s_zones[index].minutes * 60L, buf, sizeof(buf));
    return buf;
}

static void picker_apply(void)
{
    int i = td_list_selected(s_zone_list);
    const td_sysinfo_t *si = td_sysinfo();
    if (i < 0 || i >= ZONE_COUNT)
        return;
    bool ok = si->set_tz && si->set_tz(s_zones[i].minutes * 60L);
    td_win_close(s_picker);
    say(ok ? "Time zone changed." : "Could not change the time zone.");
    refresh();
    td_wm_invalidate();
}

static void on_zone_activate(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    picker_apply();
}
static void on_zone_ok(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    picker_apply();
}
static void on_zone_cancel(td_widget_t *w, void *user)
{
    (void)user;
    td_win_close(w->win);
}
static void on_picker_close(td_window_t *win)
{
    (void)win;
    s_picker = NULL;
}

static void open_zone_picker(void)
{
    if (!td_sysinfo()->set_tz)
    {
        td_msgbox("Time zone", "The time zone cannot be changed here.", "OK", NULL, NULL);
        return;
    }
    if (td_win_is_open(s_picker))
    {
        td_win_focus(s_picker);
        return;
    }
    td_window_desc_t d = {
        .title = "Time zone",
        .rect = td_rect(-1, -1, 58, 18),
        .flags = TD_WIN_MOVABLE | TD_WIN_CLOSABLE | TD_WIN_MODAL,
        .on_close = on_picker_close,
    };
    s_picker = td_win_create(&d);
    if (!s_picker)
        return;
    td_label(s_picker, 1, 0, 0, "Fixed offsets, no daylight saving (like 'tz').");
    s_zone_list = td_list(s_picker, td_rect(0, 1, -1, -2), zone_item, on_zone_activate, NULL);
    td_scrollbar(s_picker, -1, 1, -2, s_zone_list);
    td_list_set_count(s_zone_list, ZONE_COUNT);
    int cur = zone_index(user_tz());
    td_list_select(s_zone_list, cur >= 0 ? cur : zone_index(0));
    td_button(s_picker, 1, -1, "OK", on_zone_ok, NULL);
    td_button(s_picker, 9, -1, "Cancel", on_zone_cancel, NULL);
    td_widget_focus(s_zone_list);
}

/* --- automatic time and manual setting */

static void sync_now(void)
{
    const td_sysinfo_t *si = td_sysinfo();
    if (!si->time_sync)
    {
        td_msgbox("Date & time", "This device takes its time from the host.", "OK", NULL, NULL);
        return;
    }
    say(si->time_sync() ? "Synchronising with pool.ntp.org..." : "Cannot sync now (no network?).");
}

static void on_sync(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    sync_now();
}

static void on_auto(td_widget_t *w, void *user)
{
    (void)user;
    const td_sysinfo_t *si = td_sysinfo();
    bool want = td_checkbox_get(w);
    if (!td_session_is_root())
    {
        say("Only root can change how the device sets its clock.");
    }
    else if (!si->time_auto_set || !si->time_auto_set(want))
    {
        say("Could not change automatic time.");
    }
    else
    {
        say(want ? "The clock is set from the network." : "Automatic time is off.");
    }
    refresh();
}

static bool parse_datetime(const char *s, civil_t *c)
{
    memset(c, 0, sizeof(*c));
    int n = sscanf(s, "%d-%d-%d %d:%d:%d", &c->year, &c->month, &c->day, &c->hour, &c->min, &c->sec);
    if (n < 5)
        return false;
    if (c->year < 2020 || c->year > 2099 || c->month < 1 || c->month > 12 || c->day < 1 || c->day > 31)
        return false;
    if (c->hour < 0 || c->hour > 23 || c->min < 0 || c->min > 59 || c->sec < 0 || c->sec > 59)
        return false;
    civil_t check;
    long long days = days_from_civil(c->year, c->month, c->day);
    civil_from_days(days, &check.year, &check.month, &check.day);
    return check.month == c->month;   /* rejects 31 April and the like */
}

static void manual_answer(const char *text, void *user)
{
    (void)user;
    civil_t c;
    if (!parse_datetime(text, &c))
    {
        say("Use YYYY-MM-DD HH:MM (24-hour), e.g. 2026-09-24 14:05.");
        return;
    }
    long long local = days_from_civil(c.year, c.month, c.day) * 86400LL + c.hour * 3600LL + c.min * 60LL + c.sec;
    const td_sysinfo_t *si = td_sysinfo();
    bool ok = si->time_set && si->time_set((int64_t)(local - user_tz()));
    say(ok ? "The clock was set." : "Could not set the clock.");
    refresh();
}

static void on_manual(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    const td_sysinfo_t *si = td_sysinfo();
    if (!si->time_set)
    {
        say("This device takes its time from the host.");
        return;
    }
    if (!td_session_is_root())
    {
        say("Only root can change the system clock.");
        return;
    }
    if (si->time_auto_get && si->time_auto_get())
    {
        say("Turn off 'Set time automatically' first.");
        return;
    }
    char initial[32] = "";
    civil_t c;
    if (local_now(&c))
        snprintf(initial, sizeof(initial), "%04d-%02d-%02d %02d:%02d", c.year, c.month, c.day, c.hour, c.min);
    td_inputbox("Change date and time", "Local date and time (YYYY-MM-DD HH:MM):", initial, manual_answer, NULL);
}

/* --- per-user preferences */

static void on_show(td_widget_t *w, void *user)
{
    (void)user;
    td_clock_prefs()->hide_clock = td_checkbox_get(w) ? 0 : 1;
    changed_prefs();
}

static void on_hours(td_widget_t *w, void *user)
{
    (void)w;
    td_clock_prefs()->clock_12h = (uint8_t)(intptr_t)user;
    changed_prefs();
}

static void on_format(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    td_clock_prefs_t *p = td_clock_prefs();
    p->date_format = (uint8_t)((p->date_format + 1) % TD_DATE_FORMATS);
    changed_prefs();
}

static void on_zone(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    open_zone_picker();
}
static void on_close_btn(td_widget_t *w, void *user)
{
    (void)user;
    td_win_close(w->win);
}
static void on_close(td_window_t *win)
{
    (void)win;
    s_win = NULL;
}
static void on_tick(td_window_t *win)
{
    td_win_invalidate(win);
}

void td_datetime_open(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    const td_sysinfo_t *si = td_sysinfo();
    td_window_desc_t d = {
        .title = "Date & time",
        .rect = td_rect(-1, -1, 68, 21),
        .flags = TD_WIN_MOVABLE | TD_WIN_CLOSABLE,
        .on_draw = on_draw,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = 1000,
    };
    s_win = td_win_create(&d);
    if (!s_win)
        return;

    s_zone_info = td_label(s_win, 1, 4, 64, "");
    td_label(s_win, 1, 6, 0, "Time zone");
    s_zone_btn = td_button(s_win, 12, 6, "", on_zone, NULL);

    s_auto = NULL;
    if (si->time_auto_get)
    {
        s_auto = td_checkbox(s_win, 1, 8, "Set time automatically", false, on_auto, NULL);
        s_sync_btn = td_button(s_win, 40, 8, "Sync now", on_sync, NULL);
    }
    else
    {
        td_label(s_win, 1, 8, 0, "The time comes from the host computer.");
    }
    td_label(s_win, 1, 9, 0, "Set the date and time manually");
    s_manual_btn = td_button(s_win, 40, 9, "Change...", on_manual, NULL);

    s_show = td_checkbox(s_win, 1, 11, "Show time and date in the taskbar", true, on_show, NULL);
    td_label(s_win, 1, 12, 0, "Time format");
    s_h24 = td_checkbox(s_win, 16, 12, "24-hour", true, on_hours, (void *)(intptr_t)0);
    s_h12 = td_checkbox(s_win, 30, 12, "12-hour", false, on_hours, (void *)(intptr_t)1);
    td_label(s_win, 1, 13, 0, "Date format");
    s_fmt_btn = td_button(s_win, 16, 13, "DD-MM-YYYY", on_format, NULL);
    td_label(s_win, 1, 15, 0, td_session_is_root() ? "Clock settings apply to the whole device." : "Only root can set the clock; the rest is yours.");

    s_msg = td_label(s_win, 1, -2, 64, "");
    td_button(s_win, 1, -1, "Close", on_close_btn, NULL);
    refresh();
}
