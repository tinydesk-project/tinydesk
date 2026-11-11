/*
 * modbus.c - the Modbus app: read and write a device's coils and registers
 * over Modbus TCP (IP[:port]) or RTU (rtu[:baud] on the board's RS485
 * port), optionally repeated at a set interval (10 ms to 1 hour), and
 * switch on the built-in Modbus TCP server. It shares the client and
 * server with the `modbus` shell command.
 *
 * Labels are drawn in on_draw to save widgets (a small pool on the ESP32).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "td_apps.h"
#include "td_modbus.h"
#include "td_sock.h"

#define LIST_Y            6
#define REPEAT_DEFAULT_MS 1000
#define REPEAT_MIN_MS     10
#define REPEAT_MAX_MS     3600000u
#define TICK_MS           10               /* how often the window looks for results and repeats */

static td_window_t *s_win;
static td_widget_t *s_target, *s_unit, *s_table_btn, *s_addr, *s_count, *s_repeat, *s_interval, *s_values, *s_list,
    *s_server;
static td_mb_table_t s_table = TD_MB_HOLDING;

/* Window state, allocated while the window is open (RAM is tight on the
 * ESP32). */
typedef struct
{
    td_mb_result_t res;              /* last read shown in the list */
    bool have;
    char note[80];
    int ticket;
    bool ticket_is_read;
    uint32_t interval_ms;            /* repeat period, start to start */
    uint32_t next_read_ms;
    uint32_t last_start_ms;          /* when the last repeated read went out */
    uint32_t period_ms;              /* measured start-to-start time of repeated reads */
    bool keep_note;                  /* the next answer must not hide an error note */
    bool srv_on;                     /* server state last drawn */
    int srv_clients;
    uint32_t srv_reqs;
    char item[80];
    uint16_t edit_addr;
} ui_t;

static ui_t *U;

static const char *text_of(const td_widget_t *w)
{
    return td_widget_text(w);
}

static void note(const char *msg)
{
    if (!U)
        return;
    snprintf(U->note, sizeof(U->note), "%.*s", (int)sizeof(U->note) - 1, msg);
    td_wm_invalidate();
}

static bool is_bits(td_mb_table_t t)
{
    return t == TD_MB_COILS || t == TD_MB_DISCRETE;
}

static const char *table_caption(td_mb_table_t t)
{
    switch (t)
    {
    case TD_MB_COILS:
        return "Coils (0x)";
    case TD_MB_DISCRETE:
        return "Discrete inputs (1x)";
    case TD_MB_INPUT:
        return "Input registers (3x)";
    default:
        return "Holding registers (4x)";
    }
}

/* Parse a number: decimal, 0x hex, or negative (16-bit two's complement). */
static bool parse_u16(const char *s, uint16_t *out)
{
    char *end = NULL;
    const char *d = s[0] == '-' ? s + 1 : s;
    int base = d[0] == '0' && (d[1] == 'x' || d[1] == 'X') ? 16 : 10;   /* 010 is ten, not octal */
    long v = strtol(s, &end, base);
    if (!s[0] || !end || *end || v < -32768 || v > 65535)
        return false;
    *out = (uint16_t)v;
    return true;
}

/* Fill target, unit and address from the fields. */
static bool base_request(td_mb_request_t *r)
{
    memset(r, 0, sizeof(*r));
    snprintf(r->target, sizeof(r->target), "%s", text_of(s_target));
    uint16_t unit;
    if (!parse_u16(text_of(s_unit), &unit) || unit > 255)
    {
        note("Unit must be 0..255 (TCP devices often use 1 or 255)");
        return false;
    }
    r->unit = (uint8_t)unit;
    if (!parse_u16(text_of(s_addr), &r->addr))
    {
        note("Address must be a number from 0 to 65535");
        return false;
    }
    return true;
}

/* Read the interval field; false (with a note) if it is not a number of ms
 * in range. */
static bool read_interval(void)
{
    char *end = NULL;
    const char *t = text_of(s_interval);
    unsigned long v = strtoul(t, &end, 10);
    if (!t[0] || !end || *end || v < REPEAT_MIN_MS || v > REPEAT_MAX_MS)
    {
        note("Interval: 10 to 3600000 ms");
        U->keep_note = U->ticket != 0;       /* a read still on its way */
        return false;
    }
    U->interval_ms = (uint32_t)v;
    return true;
}

static void submit(const td_mb_request_t *r, bool is_read)
{
    char err[80];
    int t = td_mb_submit(r, err, sizeof(err));
    if (!t)
    {
        note(err);
        return;
    }
    U->ticket = t;
    U->ticket_is_read = is_read;
    td_mb_poll();                /* send it now, not on the next timer tick */
    /* While repeating, keep the last result on screen instead of flashing
     * "Reading..." every cycle. */
    if (!(is_read && td_checkbox_get(s_repeat)))
        note(is_read ? "Reading..." : "Writing...");
}

static void do_read(void)
{
    td_mb_request_t r;
    if (!base_request(&r))
        return;
    uint16_t count;
    if (!parse_u16(text_of(s_count), &count) || count < 1 || count > TD_MB_MAX_READ)
    {
        note("Count must be 1..125");
        return;
    }
    r.fc = (uint8_t)s_table;
    r.count = count;
    submit(&r, true);
}

static void on_read(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    do_read();
}

/* Write the values field ("1, 2, 0x10") at the address. */
static void write_values(uint16_t addr, const char *text)
{
    if (s_table != TD_MB_HOLDING && s_table != TD_MB_COILS)
    {
        note("Only coils and holding registers can be written");
        return;
    }
    td_mb_request_t r;
    if (!base_request(&r))
        return;
    r.addr = addr;
    char buf[TD_TEXT_MAX + 1];
    snprintf(buf, sizeof(buf), "%s", text);
    for (char *tok = strtok(buf, ", "); tok; tok = strtok(NULL, ", "))
    {
        if (r.nvalues >= TD_MB_MAX_WRITE || !parse_u16(tok, &r.values[r.nvalues]))
        {
            note("Values: numbers separated by commas, e.g. 1, 2, 0x10");
            return;
        }
        r.nvalues++;
    }
    if (!r.nvalues)
    {
        note("Type the value(s) to write first");
        return;
    }
    if (s_table == TD_MB_COILS)
        r.fc = r.nvalues == 1 ? 5 : 15;
    else
        r.fc = r.nvalues == 1 ? 6 : 16;
    submit(&r, false);
}

static void on_write(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    uint16_t addr;
    if (!parse_u16(text_of(s_addr), &addr))
    {
        note("Address must be a number from 0 to 65535");
        return;
    }
    write_values(addr, text_of(s_values));
}

static void on_table(td_widget_t *w, void *user)
{
    (void)user;
    static const td_mb_table_t order[] = {TD_MB_HOLDING, TD_MB_INPUT, TD_MB_COILS, TD_MB_DISCRETE};
    int i = 0;
    while (order[i] != s_table)
        i++;
    s_table = order[(i + 1) % 4];
    td_widget_set_text(w, table_caption(s_table));
    U->have = false;
    td_list_set_count(s_list, 0);
}

static void on_repeat(td_widget_t *w, void *user)
{
    (void)user;
    U->period_ms = 0;
    U->last_start_ms = 0;
    if (!td_checkbox_get(w))
        return;
    if (!read_interval())
    {
        td_checkbox_set(w, false);
        return;
    }
    U->next_read_ms = td_proto_millis();     /* start now */
}

/* Enter in the interval field: take the new value. */
static void on_interval(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    if (read_interval())
    {
        char msg[96];
        snprintf(msg, sizeof(msg), "Repeat interval: %u ms", (unsigned)U->interval_ms);
        note(msg);
        U->period_ms = 0;
    }
    else
    {
        td_checkbox_set(s_repeat, false);
    }
}

static void on_server(td_widget_t *w, void *user)
{
    (void)user;
    char err[64];
    if (td_checkbox_get(w))
    {
        if (td_mb_server_start(TD_MB_TCP_PORT, err, sizeof(err)))
            note("Modbus TCP server started on port 502");
        else
        {
            note(err);
            td_checkbox_set(w, false);
        }
    }
    else
    {
        td_mb_server_stop();
        note("Modbus TCP server stopped");
    }
}

/* Double-click a register or coil to change it. */
static void edit_answer(const char *text, void *user)
{
    (void)user;
    if (!U)
        return;
    write_values(U->edit_addr, text);
}

static void on_activate(td_widget_t *w, void *user)
{
    (void)user;
    int i = td_list_selected(w);
    if (!U->have || i < 0 || i >= U->res.count)
        return;
    if (s_table != TD_MB_HOLDING && s_table != TD_MB_COILS)
    {
        note("Only coils and holding registers can be written");
        return;
    }
    U->edit_addr = (uint16_t)(U->res.addr + i);
    char prompt[48], cur[16];
    snprintf(prompt, sizeof(prompt), "New value for address %u:", (unsigned)U->edit_addr);
    snprintf(cur, sizeof(cur), "%u", (unsigned)U->res.values[i]);
    td_inputbox("Modbus write", prompt, cur, edit_answer, NULL);
}

/* ------------------------------------------------------------ display */

static const char *get_item(td_widget_t *w, int index, int *fg, void *user)
{
    (void)w;
    (void)fg;
    (void)user;
    if (!U->have || index >= U->res.count)
        return "";
    unsigned a = (unsigned)(U->res.addr + index), v = U->res.values[index];
    if (is_bits((td_mb_table_t)U->res.fc))
        snprintf(U->item, sizeof(U->item), "%5u   %s", a, v ? "1  ON" : "0  off");
    else
        snprintf(U->item, sizeof(U->item), "%5u   %6u   0x%04X   %6d", a, v, v, (int)(int16_t)v);
    return U->item;
}

static void on_tick(td_window_t *win)
{
    (void)win;
    bool repeat = td_checkbox_get(s_repeat);
    if (U->ticket)
    {
        /* The protocol timer polls every 20 ms; while a request is out,
         * poll at the window's pace too so short intervals are not
         * stretched by that timer. */
        td_mb_poll();
        td_mb_result_t res;
        if (td_mb_result(U->ticket, &res))
        {
            U->ticket = 0;
            char msg[96];
            if (repeat && U->ticket_is_read && U->period_ms)
                snprintf(msg, sizeof(msg), "%.40s  (%u ms, every %u ms)", res.text, (unsigned)res.ms,
                         (unsigned)U->period_ms);
            else
                snprintf(msg, sizeof(msg), "%s  (%u ms)", res.text, (unsigned)res.ms);
            if (U->keep_note)
                U->keep_note = false;
            else
                note(msg);
            if (U->ticket_is_read && res.status == 0)
            {
                U->res = res;
                U->have = true;
                td_list_set_count(s_list, res.count);
            }
            else if (!U->ticket_is_read && res.status == 0)
            {
                do_read();                         /* show the new values */
            }
        }
    }
    /* A repeat that is due starts in the same tick as the answer before it. */
    if (!U->ticket && repeat && (int32_t)(td_proto_millis() - U->next_read_ms) >= 0)
    {
        uint32_t now = td_proto_millis();
        if (U->last_start_ms)
            U->period_ms = now - U->last_start_ms;
        U->last_start_ms = now;
        /* Fixed period from start to start; a device slower than the
         * interval is simply read again as soon as it has answered. */
        U->next_read_ms += U->interval_ms;
        if ((int32_t)(now - U->next_read_ms) >= 0)
            U->next_read_ms = now + U->interval_ms;
        do_read();
        if (!U->ticket)
            td_checkbox_set(s_repeat, false);   /* bad field: stop and show why */
    }

    /* Redraw only for a change: the tick is fast. */
    uint16_t port;
    int clients = 0;
    uint32_t reqs = 0;
    bool on = td_mb_server_status(&port, &clients, &reqs);
    if (on != U->srv_on || clients != U->srv_clients || reqs != U->srv_reqs)
    {
        U->srv_on = on;
        U->srv_clients = clients;
        U->srv_reqs = reqs;
        td_checkbox_set(s_server, on);
        td_wm_invalidate();
    }
}

static void on_draw(td_window_t *win, int w, int h)
{
    (void)win;
    const td_theme_t *t = td_theme();
    td_text(0, 0, "Device", t->win_fg, t->win_bg, 0);
    td_text(30, 0, "Unit", t->win_fg, t->win_bg, 0);
    const td_mb_serial_t *rtu = td_mb_serial();
    int lines = rtu ? rtu->ports : 0;
    td_text(42, 0, lines >= 2 ? "IP[:port], rtu1 or rtu2" : lines == 1 ? "IP[:port] or rtu"
                                                                       : "IP[:port] (Modbus TCP)",
            t->dim, t->win_bg, 0);
    td_text(0, 1, "Table", t->win_fg, t->win_bg, 0);
    td_text(0, 2, "Address", t->win_fg, t->win_bg, 0);
    td_text(16, 2, "Count", t->win_fg, t->win_bg, 0);
    td_text(0, 3, "Values", t->win_fg, t->win_bg, 0);
    td_text(63, 2, "ms", t->win_fg, t->win_bg, 0);
    td_textfit(0, 4, U->note[0] ? U->note : "Double-click a value in the list to change it.", w,
               U->note[0] ? t->accent : t->dim, t->win_bg, 0);
    td_textn(0, 5, is_bits(s_table) ? " Addr   Value" : " Addr   Decimal  Hex      Signed", w, t->dim, t->win_bg, 0);

    uint16_t port;
    int clients;
    uint32_t reqs;
    char line[80];
    if (td_mb_server_status(&port, &clients, &reqs))
    {
        snprintf(line, sizeof(line), "port %u, %d client%s, %u requests", (unsigned)port, clients,
                 clients == 1 ? "" : "s", (unsigned)reqs);
        td_textfit(22, h - 1, line, w - 22, t->accent, t->win_bg, 0);
    }
}

static void on_close(td_window_t *win)
{
    (void)win;
    s_win = NULL;
    free(U);
    U = NULL;
}

static void on_enter_read(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    do_read();
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    td_window_desc_t d = {
        .title = "Modbus",
        .rect = td_rect(-1, -1, 72, 21),
        .flags = TD_WIN_DEFAULT,
        .min_w = 68,
        .min_h = 12,
        .on_draw = on_draw,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = TICK_MS,
    };
    U = calloc(1, sizeof(*U));
    if (!U)
    {
        td_msgbox("Modbus", "Not enough memory. Close a window, then try again.", "OK", NULL, NULL);
        return;
    }
    s_win = td_win_create(&d);
    if (!s_win)
    {
        free(U);
        U = NULL;
        return;
    }

    s_target = td_textbox(s_win, 8, 0, 20, TD_TEXT_MAX - 1, on_enter_read, NULL);
    s_unit = td_textbox(s_win, 35, 0, 5, 3, on_enter_read, NULL);
    s_table_btn = td_button(s_win, 8, 1, table_caption(s_table), on_table, NULL);
    s_addr = td_textbox(s_win, 8, 2, 6, 5, on_enter_read, NULL);
    s_count = td_textbox(s_win, 22, 2, 4, 3, on_enter_read, NULL);
    td_button(s_win, 28, 2, "Read", on_read, NULL);
    s_repeat = td_checkbox(s_win, 37, 2, "Repeat every", false, on_repeat, NULL);
    s_interval = td_textbox(s_win, 54, 2, 8, 7, on_interval, NULL);
    s_values = td_textbox(s_win, 8, 3, 30, TD_TEXT_MAX - 1, on_write, NULL);
    td_button(s_win, 40, 3, "Write", on_write, NULL);
    s_list = td_list(s_win, td_rect(0, LIST_Y, -1, -1), get_item, on_activate, NULL);
    td_scrollbar(s_win, -1, LIST_Y, -1, s_list);
    s_server = td_checkbox(s_win, 0, -1, "TCP server", td_mb_server_status(NULL, NULL, NULL), on_server, NULL);

    td_widget_set_text(s_target, "127.0.0.1");
    td_widget_set_text(s_unit, "1");
    td_widget_set_text(s_addr, "0");
    td_widget_set_text(s_count, "10");
    td_widget_set_text(s_interval, "1000");
    U->interval_ms = REPEAT_DEFAULT_MS;
    td_widget_focus(s_target);
    (void)s_table_btn;
}

static const td_app_t s_app = {"Modbus", launch, "MB"};

void td_modbus_register(void)
{
    td_app_register(&s_app);
}
