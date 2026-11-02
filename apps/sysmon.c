/*
 * sysmon.c - live heap, tasks, CPU (and its temperature where the chip has
 * a sensor), uptime and tinydesk's own statistics.
 * The bars show memory in use; internal RAM and PSRAM (if any) apart.
 */
#include <stdio.h>

#include "td_apps.h"

static td_window_t *s_win;
static td_widget_t *s_free_bar, *s_free_txt, *s_min_bar, *s_min_txt, *s_ps_bar, *s_ps_txt;
static td_widget_t *s_tasks, *s_uptime, *s_frame, *s_serial, *s_term;

static int percent(uint32_t part, uint32_t total)
{
    return total ? (int)((uint64_t)part * 100u / total) : 0;
}

static void update(void)
{
    const td_sysinfo_t *si = td_sysinfo();
    const td_stats_t *st = td_stats();
    uint32_t total = si->total_heap ? si->total_heap() : 0;

    if (si->free_heap && total)
    {
        uint32_t f = si->free_heap();
        td_progress_set(s_free_bar, percent(total - f, total));
        td_widget_printf(s_free_txt, "%u KB free of %u", (unsigned)(f / 1024u), (unsigned)(total / 1024u));
    }
    else
    {
        td_widget_printf(s_free_txt, "n/a");
    }
    if (si->min_free_heap && total)
    {
        uint32_t m = si->min_free_heap();
        td_progress_set(s_min_bar, percent(total - m, total));
        td_widget_printf(s_min_txt, "%u KB free at least", (unsigned)(m / 1024u));
    }
    else
    {
        td_widget_printf(s_min_txt, "n/a");
    }
    if (s_ps_bar)
    {
        uint32_t pt = si->psram_total(), pf = si->psram_free();
        td_progress_set(s_ps_bar, percent(pt - pf, pt));
        td_widget_printf(s_ps_txt, "%u KB free of %u", (unsigned)(pf / 1024u), (unsigned)(pt / 1024u));
    }

    char tasks[16] = "n/a", cpu[16] = "n/a";
    if (si->task_count)
        snprintf(tasks, sizeof(tasks), "%d", si->task_count());
    if (si->cpu_mhz)
        snprintf(cpu, sizeof(cpu), "%d MHz", si->cpu_mhz());
    int t;
    if (si->chip_temp && si->chip_temp(&t))
        td_widget_printf(s_tasks, "Tasks: %-8s  CPU: %-9s  Chip: %s%d.%d °C", tasks, cpu, t < 0 ? "-" : "",
                         (t < 0 ? -t : t) / 10, (t < 0 ? -t : t) % 10);
    else
        td_widget_printf(s_tasks, "Tasks: %-8s  CPU: %s", tasks, cpu);

    uint32_t s = td_uptime_ms() / 1000u;
    td_widget_printf(s_uptime, "Uptime: %02u:%02u:%02u",
                     (unsigned)(s / 3600u), (unsigned)(s / 60u % 60u), (unsigned)(s % 60u));
    td_widget_printf(s_frame, "Frame: %u ms  Loop: %u ms  Frames: %u",
                     (unsigned)st->frame_ms, (unsigned)st->loop_ms, (unsigned)st->frames);
    td_widget_printf(s_serial, "Serial out: %u B/s  total %u KB",
                     (unsigned)st->bytes_per_sec, (unsigned)(st->bytes_sent / 1024u));
    if (st->term_cols > st->cols || st->term_rows > st->rows)
        td_widget_printf(s_term, "Screen: %dx%d of %dx%d (max)  dropped: %u", st->cols, st->rows, st->term_cols,
                         st->term_rows, (unsigned)st->dropped_frames);
    else
        td_widget_printf(s_term, "Terminal: %dx%d  dropped frames: %u", st->cols, st->rows,
                         (unsigned)st->dropped_frames);
}

static void on_tick(td_window_t *win)
{
    (void)win;
    update();
}

static void on_close(td_window_t *win)
{
    (void)win;
    s_win = NULL;
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    const td_sysinfo_t *si = td_sysinfo();
    bool psram = si->psram_total && si->psram_total() > 0;
    int y = psram ? 1 : 0;    /* rows below the memory bars move down */
    td_window_desc_t d = {
        .title = "System Monitor",
        .rect = td_rect(4, 2, 56, 12 + y),
        .flags = TD_WIN_MOVABLE | TD_WIN_CLOSABLE,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = 500,
    };
    s_win = td_win_create(&d);
    if (!s_win)
        return;

    td_label(s_win, 1, 1, 13, psram ? "Internal RAM" : "RAM in use");
    s_free_bar = td_progress(s_win, 14, 1, 18);
    s_free_txt = td_label(s_win, 33, 1, 0, "");
    td_label(s_win, 1, 2, 13, "  peak use");
    s_min_bar = td_progress(s_win, 14, 2, 18);
    s_min_txt = td_label(s_win, 33, 2, 0, "");
    s_ps_bar = s_ps_txt = NULL;
    if (psram)
    {
        td_label(s_win, 1, 3, 13, "PSRAM");
        s_ps_bar = td_progress(s_win, 14, 3, 18);
        s_ps_txt = td_label(s_win, 33, 3, 0, "");
    }
    s_tasks = td_label(s_win, 1, 4 + y, 0, "");
    s_uptime = td_label(s_win, 1, 5 + y, 0, "");
    s_frame = td_label(s_win, 1, 6 + y, 0, "");
    s_serial = td_label(s_win, 1, 7 + y, 0, "");
    s_term = td_label(s_win, 1, 8 + y, 0, "");
    update();
}

static const td_app_t s_app = {"System Monitor", launch, "▄█"};

void td_sysmon_register(void)
{
    td_app_register(&s_app);
}
