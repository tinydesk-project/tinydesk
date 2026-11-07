/*
 * render.c - turn the difference between two cell buffers into as few
 * ANSI escape bytes as possible.
 */
#include <stdio.h>
#include <string.h>

#include "tinydesk/td_screen.h"

void td_render_init(td_renderer_t *r, const td_hal_t *hal)
{
    memset(r, 0, sizeof(*r));
    r->hal = hal;
    td_render_reset_state(r);
}

void td_render_reset_state(td_renderer_t *r)
{
    r->cur_x = -1;
    r->cur_y = -1;
    r->cur_fg = -1;
    r->cur_bg = -1;
    r->cur_attr = -1;
}

bool td_render_flush(td_renderer_t *r)
{
    int off = 0;
    while (off < r->out_len && !r->write_failed)
    {
        int n = r->hal->write(r->hal->ctx, r->out + off, r->out_len - off);
        if (n <= 0)
        {
            /* The link is not draining (no terminal attached?): drop the rest
             * of this frame instead of stalling the UI. */
            r->write_failed = true;
            break;
        }
        off += n;
        r->bytes_sent += (uint32_t)n;
    }
    r->out_len = 0;
    return !r->write_failed;
}

/* Append bytes, flushing whenever the buffer fills up. */
static void out_bytes(td_renderer_t *r, const void *data, int len)
{
    const uint8_t *p = data;
    while (len > 0)
    {
        if (r->out_len == TD_OUT_BUF_SIZE)
            td_render_flush(r);
        int room = TD_OUT_BUF_SIZE - r->out_len;
        int n = len < room ? len : room;
        memcpy(r->out + r->out_len, p, (size_t)n);
        r->out_len += n;
        p += n;
        len -= n;
    }
}

void td_render_write(td_renderer_t *r, const void *data, int len)
{
    out_bytes(r, data, len);
}

void td_render_raw(td_renderer_t *r, const char *s)
{
    out_bytes(r, s, (int)strlen(s));
}

/* Move the terminal cursor, unless it is already there. */
static void move_to(td_renderer_t *r, int x, int y)
{
    if (r->cur_x == x && r->cur_y == y)
        return;
    char seq[16];
    int n = snprintf(seq, sizeof(seq), "\x1b[%d;%dH", y + 1, x + 1);
    out_bytes(r, seq, n);
    r->cur_x = x;
    r->cur_y = y;
}

/* Append ";<code>" for a foreground or background colour. The 16 basic
 * colours use the short forms so every terminal understands them. */
static int colour_param(char *p, int value, bool background)
{
    int base = background ? 40 : 30;
    if (value < 8)
        return sprintf(p, ";%d", base + value);
    if (value < 16)
        return sprintf(p, ";%d", base + 60 + value - 8);
    return sprintf(p, ";%d;5;%d", background ? 48 : 38, value);
}

/* Emit SGR only for the parts that changed. Attributes can only be turned
 * off by a reset, so any attribute change resends everything. */
static void set_style(td_renderer_t *r, const td_cell_t *c)
{
    bool attr_changed = r->cur_attr != c->attr;
    bool fg_changed = attr_changed || r->cur_fg != c->fg;
    bool bg_changed = attr_changed || r->cur_bg != c->bg;
    if (!fg_changed && !bg_changed)
        return;

    char seq[48];
    char *p = seq;
    p += sprintf(p, "\x1b[");
    if (attr_changed)
    {
        p += sprintf(p, "0");
        if (c->attr & TD_BOLD)
            p += sprintf(p, ";1");
        if (c->attr & TD_UNDERLINE)
            p += sprintf(p, ";4");
        if (c->attr & TD_REVERSE)
            p += sprintf(p, ";7");
    }
    if (fg_changed)
        p += colour_param(p, c->fg, false);
    if (bg_changed)
        p += colour_param(p, c->bg, true);
    /* Remove the leading ';' when no reset was emitted. */
    if (!attr_changed)
    {
        memmove(seq + 2, seq + 3, (size_t)(p - seq - 3));
        p--;
    }
    *p++ = 'm';
    out_bytes(r, seq, (int)(p - seq));

    r->cur_attr = c->attr;
    r->cur_fg = c->fg;
    r->cur_bg = c->bg;
}

static void put_char(td_renderer_t *r, uint32_t ch, int x, int cols)
{
    if (ch < 0x20u || ch == 0x7Fu)
        ch = ' ';
    if (td_get_ascii_mode())
        ch = td_ascii_fallback(ch);
    uint8_t utf8[4];
    out_bytes(r, utf8, td_utf8_encode(ch, utf8));

    /* With auto-wrap off the cursor sticks at the last column; forget the
     * position there rather than guess what the terminal did. */
    if (x + 1 >= cols)
        r->cur_x = -1;
    else
        r->cur_x++;
}

bool td_render_diff_region(td_renderer_t *r, td_buffer_t *front, const td_buffer_t *back, int x0, int y0, int w, int h)
{
    int cols = back->cols;
    int rows = back->rows;
    r->write_failed = false;

    if (front->cols != cols || front->rows != rows)
    {
        front->cols = cols;
        front->rows = rows;
        td_buffer_invalidate(front);
    }
    if (x0 < 0)
        x0 = 0;
    if (w <= 0 || x0 + w > cols)
        w = cols - x0;
    if (y0 < 0 || y0 >= rows)
        y0 = 0;
    if (h <= 0 || y0 + h > rows)
        h = rows - y0;

    /* Screen column x is the terminal's column x - x0. */
    for (int y = 0; y < h && !r->write_failed; y++)
    {
        const td_cell_t *b = &back->cells[(y0 + y) * cols + x0];
        td_cell_t *f = &front->cells[(y0 + y) * cols + x0];
        for (int x = 0; x < w; x++)
        {
            if (memcmp(&b[x], &f[x], sizeof(td_cell_t)) == 0)
                continue;
            move_to(r, x, y);
            set_style(r, &b[x]);
            put_char(r, b[x].ch, x, w);
            f[x] = b[x];
        }
    }
    td_render_flush(r);
    return !r->write_failed;
}

bool td_render_diff(td_renderer_t *r, td_buffer_t *front, const td_buffer_t *back)
{
    return td_render_diff_region(r, front, back, 0, 0, back->cols, back->rows);
}

void td_render_setup_terminal(td_renderer_t *r)
{
    /* CAN aborts any half-sent escape sequence left over from before. */
    td_render_raw(r, "\x18");
    td_render_raw(r, "\x1b[?1049h");                 /* alternate screen */
    td_render_raw(r, "\x1b[?25l");                   /* hide cursor */
    td_render_raw(r, "\x1b[?7l");                    /* no auto-wrap */
    /* Mouse: clicks, drag, any motion (for hover highlights), SGR coords. */
    td_render_raw(r, "\x1b[?1000h\x1b[?1002h\x1b[?1003h\x1b[?1006h");
    td_render_raw(r, "\x1b[?2004h");                 /* bracketed paste */
    td_render_raw(r, "\x1b[0m\x1b[2J\x1b[H");
    td_render_reset_state(r);
    r->cur_x = 0;
    r->cur_y = 0;
}

void td_render_restore_terminal(td_renderer_t *r)
{
    td_render_raw(r, "\x1b[?1006l\x1b[?1003l\x1b[?1002l\x1b[?1000l\x1b[?2004l");
    td_render_raw(r, "\x1b[0m\x1b[2J\x1b[H");
    td_render_raw(r, "\x1b[?7h\x1b[?25h\x1b[?1049l");
    td_render_flush(r);
    td_render_reset_state(r);
}

void td_render_query_size(td_renderer_t *r)
{
    /* Save cursor, jump far past the corner (the terminal clamps), ask where
     * the cursor ended up, restore. The reply is ESC [ rows ; cols R. */
    td_render_raw(r, "\x1b"
                     "7\x1b[999;999H\x1b[6n\x1b"
                     "8");
}
