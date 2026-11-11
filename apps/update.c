/*
 * update.c - the Software Update app: shows the installed firmware and
 * installs a new one from a URL or a file on the device, with progress.
 * Everything goes through td_sysinfo()->ota (the ESP32 port provides it).
 *
 * "Check for official updates" asks the port for the newest official
 * release and puts its image into the URL field. With "Check daily and
 * notify me" (kept by the port) a background check runs a few minutes
 * after start-up and then once a day, and a newer version is announced
 * once.
 *
 * Only root may install or roll back; everyone can look.
 */
#include <stdio.h>
#include <string.h>

#include "td_apps.h"

static td_window_t *s_win;
static td_widget_t *s_source, *s_check, *s_install, *s_cancel, *s_progress, *s_restart, *s_rollback;
static td_widget_t *s_official, *s_auto;
static char s_note[96];
static char s_url[200];          /* the URL field's text (longer than a widget's own) */
static bool s_fill_url;          /* put the next official result into the URL field */
static uint32_t s_seen_checks;   /* official() checks already looked at */

static const td_ota_ops_t *ota(void)
{
    return td_sysinfo()->ota;
}
static bool has_official(void)
{
    return ota() && ota()->check_official && ota()->official;
}

static void note(const char *msg)
{
    snprintf(s_note, sizeof(s_note), "%s", msg);
    td_wm_invalidate();
}

static bool allowed(void)
{
    if (td_session_is_root())
        return true;
    note("Only root can install updates.");
    return false;
}

static bool is_url(const char *s)
{
    return !strncmp(s, "http://", 7) || !strncmp(s, "https://", 8);
}

static bool start(bool check_only)
{
    if (!ota() || !allowed())
        return false;
    const char *text = td_widget_text(s_source);
    char real[200];
    const char *src = text;
    if (!text[0] || !strcmp(text, "https://"))
    {
        note("Type a URL (http:// or https://) or a .bin file, or check for official updates.");
        return false;
    }
    if (!is_url(text))
    {
        if (!td_session_real_path(text, real, sizeof(real)))
        {
            note("That file is outside your home folder.");
            return false;
        }
        src = real;
    }
    if (!ota()->start(src, check_only))
    {
        note("An update is already running.");
        return false;
    }
    s_note[0] = '\0';
    return true;
}

static void on_check(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    start(true);
}

/* Save and install | Install anyway | Cancel */
static void install_answer(int button, void *user)
{
    (void)user;
    if (button == 0)
    {
        char msg[96];
        if (!ota()->save_settings(msg, (int)sizeof(msg)))
        {
            note(msg);
            return;
        }
        if (start(false))
            note(msg);
    }
    else if (button == 1)
    {
        start(false);
    }
}

static void on_install(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    /* Pins built into this firmware only would be lost by firmware built
     * without them (an official release): offer to save them first. */
    int unsaved = ota() && allowed() && ota()->unsaved_settings && ota()->save_settings ? ota()->unsaved_settings() : 0;
    if (unsaved > 0)
    {
        char text[320];
        snprintf(text, sizeof(text),
                 "%d board setting%s (pins) %s built into the\n"
                 "firmware this board runs. Firmware built without\n"
                 "them, such as an official release, starts without\n"
                 "them: no Ethernet, SD card or RS-485 until they are\n"
                 "set again. Save them on the board first?",
                 unsaved, unsaved == 1 ? "" : "s", unsaved == 1 ? "is" : "are");
        td_msgbox("Software Update", text, "Save and install|Install anyway|Cancel", install_answer, NULL);
        return;
    }
    start(false);
}
static void on_enter(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    start(true);
}

static void on_official(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    if (!has_official())
        return;
    if (!ota()->check_official(false))
    {
        note("An update is already running.");
        return;
    }
    s_note[0] = '\0';
    s_fill_url = true;
}

static void on_auto(td_widget_t *w, void *user)
{
    (void)user;
    if (!ota() || !ota()->set_auto_check)
        return;
    if (!td_session_is_root())
    {                 /* a device setting */
        td_checkbox_set(w, ota()->auto_check());
        note("Only root can change this.");
        return;
    }
    ota()->set_auto_check(td_checkbox_get(w));
}

static void on_cancel(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    if (ota() && allowed())
        ota()->cancel();
}

static void restart_answer(int button, void *user)
{
    (void)user;
    if (button == 0 && ota())
        ota()->restart();
}

static void on_restart(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    if (!ota() || !allowed())
        return;
    td_msgbox("Software update", "Restart now? Unsaved work is lost.", "Restart|Later",
              restart_answer, NULL);
}

static void rollback_answer(int button, void *user)
{
    (void)user;
    if (button == 0 && ota() && !ota()->roll_back())
        note("Could not go back to the previous version.");
}

static void on_rollback(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    if (!ota() || !allowed())
        return;
    td_ota_info_t i;
    ota()->info(&i);
    char text[96];
    snprintf(text, sizeof(text), "Go back to version %s and restart?", i.on_trial ? "(previous)" : i.other_version);
    td_msgbox("Software update", text, "Roll back|Cancel", rollback_answer, NULL);
}

/* "1.02 MB" / "640 KB" */
static void size_text(uint32_t bytes, char *buf, size_t cap)
{
    if (bytes >= 1024u * 1024u)
        snprintf(buf, cap, "%u.%02u MB", (unsigned)(bytes >> 20), (unsigned)((bytes % (1u << 20)) * 100 >> 20));
    else
        snprintf(buf, cap, "%u KB", (unsigned)(bytes / 1024));
}

static void on_tick(td_window_t *win)
{
    (void)win;
    if (!ota())
        return;
    td_ota_status_t st;
    ota()->status(&st);
    td_ota_info_t i;
    ota()->info(&i);
    bool running = st.state == TD_OTA_CHECKING || st.state == TD_OTA_INSTALLING;
    td_progress_set(s_progress, st.state == TD_OTA_DONE ? 100 : st.percent > 0 ? st.percent
                                                                               : 0);
    td_widget_set_visible(s_cancel, running);
    td_widget_set_visible(s_restart, st.state == TD_OTA_DONE);
    td_widget_set_visible(s_rollback, !running && st.state != TD_OTA_DONE && (i.on_trial || i.can_roll_back));
    td_widget_set_visible(s_check, !running);
    td_widget_set_visible(s_install, !running);
    td_widget_set_visible(s_official, !running && has_official());
    if (has_official())
    {
        td_ota_release_t r;
        ota()->official(&r);
        if (r.checks != s_seen_checks)
        {
            s_seen_checks = r.checks;
            if (s_fill_url && r.valid && !r.error[0])
                td_widget_set_text(s_source, r.url);
            s_fill_url = false;
        }
    }
    td_wm_invalidate();
}

static void on_draw(td_window_t *win, int w, int h)
{
    (void)win;
    const td_theme_t *t = td_theme();
    if (!ota())
    {
        const char *why = td_sysinfo()->no_ota_text;
        if (!why)
        {   /* a PC program */
            td_textn(1, 1, "Updates over the network are for the boards.", w - 1, t->win_fg, t->win_bg, 0);
            td_textn(1, 2, "On a PC, download the new release and replace this program.", w - 1, t->dim, t->win_bg, 0);
            return;
        }
        int y = 1;
        for (const char *p = why; *p && y < h; y++)
        {      /* one line per '\n' */
            const char *nl = strchr(p, '\n');
            int n = nl ? (int)(nl - p) : (int)strlen(p);
            char line[96];
            snprintf(line, sizeof(line), "%.*s", n, p);
            td_textfit(1, y, line, w - 1, y == 1 ? t->win_fg : t->dim, t->win_bg, 0);
            p += n + (nl ? 1 : 0);
        }
        return;
    }
    td_ota_info_t i;
    ota()->info(&i);
    td_ota_status_t st;
    ota()->status(&st);
    char line[240];

    snprintf(line, sizeof(line), "Installed   TinyDesk %s  (built %s, ESP-IDF %s)", i.version, i.built, i.sdk);
    td_textfit(1, 0, line, w - 1, t->win_fg, t->win_bg, TD_BOLD);
    snprintf(line, sizeof(line), "Running     from %s%s", i.running,
             i.on_trial ? ", on trial: confirms itself 30 s after start-up" : ", confirmed");
    td_textfit(1, 1, line, w - 1, i.on_trial ? t->accent : t->win_fg, t->win_bg, 0);
    if (i.other_version[0])
        snprintf(line, sizeof(line), "Other slot  %s holds version %s", i.next, i.other_version);
    else
        snprintf(line, sizeof(line), "Other slot  %s is empty (updates go there)", i.next);
    td_textfit(1, 2, line, w - 1, t->dim, t->win_bg, 0);

    td_text(1, 4, "Update from", t->win_fg, t->win_bg, 0);
    td_textn(1, 5, "http:// or https:// URL of a TinyDesk .bin, or a file here (e.g. ~/tinydesk.bin)", w - 1,
             t->dim, t->win_bg, 0);

    /* The newest official release, once a check found it. */
    if (has_official())
    {
        td_ota_release_t r;
        ota()->official(&r);
        if (r.valid)
        {
            char size[16] = "";
            if (r.size)
                size_text(r.size, size, sizeof(size));
            snprintf(line, sizeof(line), "Official    TinyDesk %s%s%s%s%s  %s", r.version, r.date[0] ? ", " : "", r.date,
                     size[0] ? ", " : "", size, r.newer ? "(newer)" : !strcmp(r.version, i.version) ? "(installed)"
                                                                                                    : "");
            td_textfit(1, 9, line, w - 1, r.newer ? t->accent : t->win_fg, t->win_bg, r.newer ? TD_BOLD : 0);
            if (r.notes[0])
            {
                snprintf(line, sizeof(line), "Notes       %s", r.notes);
                td_textfit(1, 10, line, w - 1, t->dim, t->win_bg, 0);
            }
        }
        else if (r.error[0] && strcmp(r.error, st.message) != 0)
        {   /* a background check failed */
            snprintf(line, sizeof(line), "Official    %s", r.error);
            td_textfit(1, 9, line, w - 1, t->dim, t->win_bg, 0);
        }
    }

    if (st.new_version[0])
    {
        snprintf(line, sizeof(line), "Image       TinyDesk %s  (built %s)", st.new_version, st.new_built);
        td_textfit(1, 11, line, w - 1, t->win_fg, t->win_bg, 0);
    }
    if (st.state == TD_OTA_INSTALLING || st.state == TD_OTA_DONE || (st.state == TD_OTA_FAILED && st.done))
    {
        char done[16], total[16];
        size_text(st.done, done, sizeof(done));
        size_text(st.total, total, sizeof(total));
        if (st.total && st.bytes_per_s && st.state == TD_OTA_INSTALLING)
        {
            uint32_t left = (st.total - st.done) / st.bytes_per_s;
            snprintf(line, sizeof(line), "%3d%%   %s of %s   %u KB/s   about %u s left", st.percent, done, total,
                     (unsigned)(st.bytes_per_s / 1024), (unsigned)left);
        }
        else
        {
            snprintf(line, sizeof(line), "%3d%%   %s of %s", st.state == TD_OTA_DONE ? 100 : st.percent, done, total);
        }
        td_textfit(1, 13, line, w - 1, t->win_fg, t->win_bg, 0);
    }
    const char *msg = s_note[0] ? s_note : st.message;
    uint8_t fg = st.state == TD_OTA_FAILED && !s_note[0] ? t->accent : t->win_fg;
    td_textfit(1, 14, msg, w - 1, fg, t->win_bg, st.state == TD_OTA_DONE ? TD_BOLD : 0);
    if (!td_session_is_root())
        td_textn(1, 15, "Only root can install updates or roll back.", w - 1, t->dim, t->win_bg, 0);
}

static void on_close(td_window_t *win)
{
    (void)win;
    s_win = NULL;
    s_source = s_check = s_install = s_cancel = s_progress = s_restart = s_rollback = NULL;
    s_official = s_auto = NULL;
    s_fill_url = false;
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    td_window_desc_t d = {
        .title = "Software Update",
        .rect = td_rect(-1, -1, 78, 20),
        .flags = TD_WIN_MOVABLE | TD_WIN_CLOSABLE,
        .on_draw = on_draw,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = 250,
    };
    s_win = td_win_create(&d);
    if (!s_win)
        return;
    s_note[0] = '\0';
    if (!ota())
        return;
    s_source = td_textbox(s_win, 13, 4, 62, TD_TEXT_MAX - 1, on_enter, NULL);
    snprintf(s_url, sizeof(s_url), "https://");
    td_textbox_set_buffer(s_source, s_url, (int)sizeof(s_url));
    s_check = td_button(s_win, 1, 6, "Check", on_check, NULL);
    s_install = td_button(s_win, 11, 6, "Install", on_install, NULL);
    s_cancel = td_button(s_win, 23, 6, "Cancel", on_cancel, NULL);
    if (has_official())
    {
        s_official = td_button(s_win, 34, 6, "Check for official updates", on_official, NULL);
        if (ota()->auto_check)
            s_auto = td_checkbox(s_win, 1, 7, "Check for official updates daily and notify me",
                                 ota()->auto_check(), on_auto, NULL);
        td_ota_release_t r;
        ota()->official(&r);
        s_seen_checks = r.checks;
        if (r.valid && r.newer)
            td_widget_set_text(s_source, r.url);   /* known from the daily check */
    }
    s_progress = td_progress(s_win, 1, 12, 74);
    s_restart = td_button(s_win, 1, -1, "Restart now", on_restart, NULL);
    s_rollback = td_button(s_win, 19, -1, "Roll back", on_rollback, NULL);
    on_tick(s_win);
    td_widget_focus(s_source);
}

/* ------------------------------------------------- the daily check */

#define AUTO_TICK_MS 60000u      /* look once a minute whether a check is due */
#define AUTO_FIRST_S 120u        /* first check 2 minutes after start-up */
#define AUTO_EVERY_S 86400u      /* then once a day */
#define AUTO_RETRY_S 1800u       /* or in 30 minutes when it failed (no network yet) */

static uint32_t s_next_check_s = AUTO_FIRST_S;
static uint32_t s_auto_seen;         /* official() checks the notifier has looked at */
static bool s_waiting;               /* a background check is running */

static void notice_answer(int button, void *user)
{
    (void)user;
    if (button == 0)
        td_app_launch("Software Update");
}

static void auto_tick(void *user)
{
    (void)user;
    if (!has_official() || !ota()->auto_check || !ota()->auto_check())
        return;
    uint32_t now_s = td_millis() / 1000u;
    td_ota_release_t r;
    ota()->official(&r);
    if (s_waiting && r.checks != s_auto_seen)
    {      /* the background check finished */
        s_waiting = false;
        s_auto_seen = r.checks;
        s_next_check_s = now_s + (r.error[0] ? AUTO_RETRY_S : AUTO_EVERY_S);
        char told[24] = "";
        if (ota()->notified)
            ota()->notified(told, (int)sizeof(told));
        if (r.valid && r.newer && strcmp(told, r.version) != 0)
        {
            if (ota()->set_notified)
                ota()->set_notified(r.version);   /* tell once per version */
            td_ota_info_t i;
            ota()->info(&i);
            char text[96];
            snprintf(text, sizeof(text), "TinyDesk %s is available (installed: %s).", r.version, i.version);
            td_msgbox("Software Update", text, "Open|Later", notice_answer, NULL);
        }
        return;
    }
    if (!s_waiting && now_s >= s_next_check_s)
    {
        if (ota()->check_official(true))
        {
            s_waiting = true;
            s_auto_seen = r.checks;
        }
        else
        {
            s_next_check_s = now_s + 60u;            /* an update is running: soon again */
        }
    }
}

static const td_app_t s_app = {"Software Update", launch, "\xE2\x86\x91 "};   /* ↑ */

void td_update_register(void)
{
    td_app_register(&s_app);
    if (has_official())
        td_timer_start(AUTO_TICK_MS, true, auto_tick, NULL, td_millis());
}
