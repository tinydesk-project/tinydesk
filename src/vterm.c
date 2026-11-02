/*
 * vterm.c - small VT100/ANSI emulator (see td_vterm.h for the subset).
 */
#include <stdio.h>
#include <string.h>

#include "tinydesk/td_vterm.h"

enum
{
    VS_GROUND,
    VS_ESC,
    VS_CSI,
    VS_OSC,
    VS_OSC_ESC,
    VS_DCS,
    VS_DCS_ESC,
    VS_SKIP1
};

#define CELL(vt, x, y) ((vt)->cells[(y)*TD_VT_MAX_COLS + (x)])

static int clamp(int v, int lo, int hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

/* Blank cell in the current background colour (erase uses it). */
static td_vcell_t blank(const td_vterm_t *vt)
{
    td_vcell_t c = {' ', vt->def_fg, (vt->attr & TD_REVERSE) ? vt->fg : vt->bg};
    return c;
}

static void clear_cells(td_vterm_t *vt, int x0, int x1, int y)
{
    td_vcell_t b = blank(vt);
    for (int x = x0; x < x1; x++)
        CELL(vt, x, y) = b;
}

static void reset_state(td_vterm_t *vt)
{
    vt->def_fg = 7;
    vt->def_bg = 0;
    vt->fg = vt->def_fg;
    vt->bg = vt->def_bg;
    vt->attr = 0;
    vt->cx = vt->cy = 0;
    vt->saved_cx = vt->saved_cy = 0;
    vt->saved_fg = vt->fg;
    vt->saved_bg = vt->bg;
    vt->saved_attr = 0;
    vt->top = 0;
    vt->bottom = vt->rows - 1;
    vt->wrap_pending = false;
    vt->autowrap = true;
    vt->cursor_visible = true;
    vt->newline_mode = true;
    vt->state = VS_GROUND;
    vt->plen = 0;
    memset(&vt->utf8, 0, sizeof(vt->utf8));
}

void td_vterm_init(td_vterm_t *vt, int cols, int rows)
{
    td_vterm_reply_fn reply = vt->reply;
    void *reply_user = vt->reply_user;
    memset(vt, 0, sizeof(*vt));
    vt->reply = reply;
    vt->reply_user = reply_user;
    vt->cols = clamp(cols, 1, TD_VT_MAX_COLS);
    vt->rows = clamp(rows, 1, TD_VT_MAX_ROWS);
    reset_state(vt);
    for (int y = 0; y < TD_VT_MAX_ROWS; y++)
        clear_cells(vt, 0, TD_VT_MAX_COLS, y);
    vt->dirty = true;
}

int td_vterm_scrollback_lines(const td_vterm_t *vt)
{
    return vt->sb_count;
}

static void push_scrollback(td_vterm_t *vt, int y)
{
    if (TD_VT_SCROLLBACK <= 0)
        return;
    memcpy(&vt->sb[vt->sb_head * TD_VT_MAX_COLS], &CELL(vt, 0, y), sizeof(td_vcell_t) * TD_VT_MAX_COLS);
    vt->sb_head = (vt->sb_head + 1) % TD_VT_SCROLLBACK;
    if (vt->sb_count < TD_VT_SCROLLBACK)
        vt->sb_count++;
}

/* Scroll rows top..bottom up by n (new blank lines at the bottom). */
static void scroll_up(td_vterm_t *vt, int top, int bottom, int n)
{
    int height = bottom - top + 1;
    if (n > height)
        n = height;
    /* Only lines leaving a full-screen region are worth keeping. */
    if (top == 0 && bottom == vt->rows - 1)
        for (int i = 0; i < n; i++)
            push_scrollback(vt, top + i);
    memmove(&CELL(vt, 0, top), &CELL(vt, 0, top + n),
            sizeof(td_vcell_t) * TD_VT_MAX_COLS * (size_t)(height - n));
    for (int y = bottom - n + 1; y <= bottom; y++)
        clear_cells(vt, 0, TD_VT_MAX_COLS, y);
}

/* Scroll rows top..bottom down by n (new blank lines at the top). */
static void scroll_down(td_vterm_t *vt, int top, int bottom, int n)
{
    int height = bottom - top + 1;
    if (n > height)
        n = height;
    memmove(&CELL(vt, 0, top + n), &CELL(vt, 0, top),
            sizeof(td_vcell_t) * TD_VT_MAX_COLS * (size_t)(height - n));
    for (int y = top; y < top + n; y++)
        clear_cells(vt, 0, TD_VT_MAX_COLS, y);
}

void td_vterm_resize(td_vterm_t *vt, int cols, int rows)
{
    cols = clamp(cols, 1, TD_VT_MAX_COLS);
    rows = clamp(rows, 1, TD_VT_MAX_ROWS);
    if (cols == vt->cols && rows == vt->rows)
        return;

    /* Keep the cursor line on screen by scrolling older lines away. */
    if (vt->cy >= rows)
    {
        int n = vt->cy - rows + 1;
        vt->top = 0;
        vt->bottom = vt->rows - 1;
        scroll_up(vt, 0, vt->rows - 1, n);
        vt->cy -= n;
    }
    /* Newly exposed cells start blank. */
    for (int y = 0; y < TD_VT_MAX_ROWS; y++)
    {
        if (y >= rows)
            clear_cells(vt, 0, TD_VT_MAX_COLS, y);
        else if (cols < TD_VT_MAX_COLS)
            clear_cells(vt, cols, TD_VT_MAX_COLS, y);
    }
    vt->cols = cols;
    vt->rows = rows;
    vt->top = 0;
    vt->bottom = rows - 1;
    vt->cx = clamp(vt->cx, 0, cols - 1);
    vt->cy = clamp(vt->cy, 0, rows - 1);
    vt->wrap_pending = false;
    vt->dirty = true;
}

/* ------------------------------------------------------------ output */

static void line_feed(td_vterm_t *vt)
{
    if (vt->cy == vt->bottom)
        scroll_up(vt, vt->top, vt->bottom, 1);
    else if (vt->cy < vt->rows - 1)
        vt->cy++;
}

static void put_char(td_vterm_t *vt, uint32_t cp)
{
    if (vt->wrap_pending)
    {
        vt->wrap_pending = false;
        vt->cx = 0;
        line_feed(vt);
    }
    uint8_t fg = vt->fg, bg = vt->bg;
    if ((vt->attr & TD_BOLD) && fg < 8)
        fg = (uint8_t)(fg + 8);
    if (vt->attr & TD_REVERSE)
    {
        uint8_t t = fg;
        fg = bg;
        bg = t;
    }
    td_vcell_t *c = &CELL(vt, vt->cx, vt->cy);
    c->ch = (cp > 0xFFFFu) ? (uint16_t)'?' : (uint16_t)cp;
    c->fg = fg;
    c->bg = bg;

    if (vt->cx < vt->cols - 1)
        vt->cx++;
    else if (vt->autowrap)
        vt->wrap_pending = true;
}

static void control_char(td_vterm_t *vt, uint8_t b)
{
    switch (b)
    {
    case '\r':
        vt->cx = 0;
        vt->wrap_pending = false;
        break;
    case '\n':
    case 0x0B:
    case 0x0C:
        if (vt->newline_mode)
            vt->cx = 0;
        vt->wrap_pending = false;
        line_feed(vt);
        break;
    case '\b':
        if (vt->cx > 0)
            vt->cx--;
        vt->wrap_pending = false;
        break;
    case '\t':
        vt->cx = clamp((vt->cx / 8 + 1) * 8, 0, vt->cols - 1);
        break;
    default:
        break;   /* BEL and others: ignore */
    }
}

/* ------------------------------------------------------------- CSI */

static int parse_params(const td_vterm_t *vt, int *out, int max)
{
    int n = 0, value = 0;
    bool have = false;
    for (int i = 0; i < vt->plen; i++)
    {
        char c = vt->params[i];
        if (c >= '0' && c <= '9')
        {
            value = value * 10 + (c - '0');
            if (value > 9999)
                value = 9999;
            have = true;
        }
        else if (c == ';' || c == ':')
        {
            if (n < max)
                out[n++] = value;
            value = 0;
            have = false;
        }
    }
    if ((have || n > 0) && n < max)
        out[n++] = value;
    return n;
}

/* Approximate a 24-bit colour with the xterm 6x6x6 cube. */
static uint8_t rgb_to_256(int r, int g, int b)
{
    int ri = clamp((r + 25) / 51, 0, 5);
    int gi = clamp((g + 25) / 51, 0, 5);
    int bi = clamp((b + 25) / 51, 0, 5);
    return (uint8_t)(16 + 36 * ri + 6 * gi + bi);
}

static void sgr(td_vterm_t *vt, const int *p, int n)
{
    if (n == 0)
    {
        vt->fg = vt->def_fg;
        vt->bg = vt->def_bg;
        vt->attr = 0;
        return;
    }
    for (int i = 0; i < n; i++)
    {
        int v = p[i];
        if (v == 0)
        {
            vt->fg = vt->def_fg;
            vt->bg = vt->def_bg;
            vt->attr = 0;
        }
        else if (v == 1)
            vt->attr |= TD_BOLD;
        else if (v == 4)
            vt->attr |= TD_UNDERLINE;
        else if (v == 7)
            vt->attr |= TD_REVERSE;
        else if (v == 22)
            vt->attr &= (uint8_t)~TD_BOLD;
        else if (v == 24)
            vt->attr &= (uint8_t)~TD_UNDERLINE;
        else if (v == 27)
            vt->attr &= (uint8_t)~TD_REVERSE;
        else if (v >= 30 && v <= 37)
            vt->fg = (uint8_t)(v - 30);
        else if (v == 39)
            vt->fg = vt->def_fg;
        else if (v >= 40 && v <= 47)
            vt->bg = (uint8_t)(v - 40);
        else if (v == 49)
            vt->bg = vt->def_bg;
        else if (v >= 90 && v <= 97)
            vt->fg = (uint8_t)(v - 90 + 8);
        else if (v >= 100 && v <= 107)
            vt->bg = (uint8_t)(v - 100 + 8);
        else if ((v == 38 || v == 48) && i + 1 < n)
        {
            uint8_t colour;
            if (p[i + 1] == 5 && i + 2 < n)
            {
                colour = (uint8_t)clamp(p[i + 2], 0, 255);
                i += 2;
            }
            else if (p[i + 1] == 2 && i + 4 < n)
            {
                colour = rgb_to_256(p[i + 2], p[i + 3], p[i + 4]);
                i += 4;
            }
            else
            {
                break;
            }
            if (v == 38)
                vt->fg = colour;
            else
                vt->bg = colour;
        }
    }
}

static void reply(td_vterm_t *vt, const char *s)
{
    if (vt->reply)
        vt->reply(vt->reply_user, s, (int)strlen(s));
}

static void set_mode(td_vterm_t *vt, const int *p, int n, bool on)
{
    if (!vt->priv)
        return;
    for (int i = 0; i < n; i++)
    {
        switch (p[i])
        {
        case 7:
            vt->autowrap = on;
            break;
        case 25:
            vt->cursor_visible = on;
            break;
        case 47:
        case 1047:
        case 1049:
            /* Alternate screen: approximate by clearing. */
            for (int y = 0; y < vt->rows; y++)
                clear_cells(vt, 0, TD_VT_MAX_COLS, y);
            if (on)
                vt->cx = vt->cy = 0;
            break;
        default:
            break;
        }
    }
}

static void erase_display(td_vterm_t *vt, int mode)
{
    if (mode == 0)
    {
        clear_cells(vt, vt->cx, vt->cols, vt->cy);
        for (int y = vt->cy + 1; y < vt->rows; y++)
            clear_cells(vt, 0, vt->cols, y);
    }
    else if (mode == 1)
    {
        for (int y = 0; y < vt->cy; y++)
            clear_cells(vt, 0, vt->cols, y);
        clear_cells(vt, 0, vt->cx + 1, vt->cy);
    }
    else
    {
        for (int y = 0; y < vt->rows; y++)
            clear_cells(vt, 0, vt->cols, y);
        if (mode == 3)
            vt->sb_count = 0;
    }
}

static void erase_line(td_vterm_t *vt, int mode)
{
    if (mode == 0)
        clear_cells(vt, vt->cx, vt->cols, vt->cy);
    else if (mode == 1)
        clear_cells(vt, 0, vt->cx + 1, vt->cy);
    else
        clear_cells(vt, 0, vt->cols, vt->cy);
}

static void csi(td_vterm_t *vt, char final)
{
    int p[16];
    int n = parse_params(vt, p, 16);
    int p0 = n > 0 ? p[0] : 0;
    int count = p0 > 0 ? p0 : 1;     /* movement counts default to 1 */
    char buf[32];

    if (final != 'm' && final != 'n' && final != 'c')
        vt->wrap_pending = false;

    switch (final)
    {
    case 'A':
        vt->cy = clamp(vt->cy - count, 0, vt->rows - 1);
        break;
    case 'B':
    case 'e':
        vt->cy = clamp(vt->cy + count, 0, vt->rows - 1);
        break;
    case 'C':
    case 'a':
        vt->cx = clamp(vt->cx + count, 0, vt->cols - 1);
        break;
    case 'D':
        vt->cx = clamp(vt->cx - count, 0, vt->cols - 1);
        break;
    case 'E':
        vt->cy = clamp(vt->cy + count, 0, vt->rows - 1);
        vt->cx = 0;
        break;
    case 'F':
        vt->cy = clamp(vt->cy - count, 0, vt->rows - 1);
        vt->cx = 0;
        break;
    case 'G':
    case '`':
        vt->cx = clamp(count - 1, 0, vt->cols - 1);
        break;
    case 'd':
        vt->cy = clamp(count - 1, 0, vt->rows - 1);
        break;
    case 'H':
    case 'f':
    {
        int row = (n > 0 && p[0] > 0) ? p[0] : 1;
        int col = (n > 1 && p[1] > 0) ? p[1] : 1;
        vt->cy = clamp(row - 1, 0, vt->rows - 1);
        vt->cx = clamp(col - 1, 0, vt->cols - 1);
        break;
    }
    case 'J':
        erase_display(vt, p0);
        break;
    case 'K':
        erase_line(vt, p0);
        break;
    case 'L':
        if (vt->cy >= vt->top && vt->cy <= vt->bottom)
            scroll_down(vt, vt->cy, vt->bottom, count);
        break;
    case 'M':
        if (vt->cy >= vt->top && vt->cy <= vt->bottom)
        {
            /* Deleted lines are not scrollback material. */
            int height = vt->bottom - vt->cy + 1;
            int k = count > height ? height : count;
            memmove(&CELL(vt, 0, vt->cy), &CELL(vt, 0, vt->cy + k),
                    sizeof(td_vcell_t) * TD_VT_MAX_COLS * (size_t)(height - k));
            for (int y = vt->bottom - k + 1; y <= vt->bottom; y++)
                clear_cells(vt, 0, TD_VT_MAX_COLS, y);
        }
        break;
    case 'P':
    {
        int k = clamp(count, 0, vt->cols - vt->cx);
        memmove(&CELL(vt, vt->cx, vt->cy), &CELL(vt, vt->cx + k, vt->cy),
                sizeof(td_vcell_t) * (size_t)(vt->cols - vt->cx - k));
        clear_cells(vt, vt->cols - k, vt->cols, vt->cy);
        break;
    }
    case '@':
    {
        int k = clamp(count, 0, vt->cols - vt->cx);
        memmove(&CELL(vt, vt->cx + k, vt->cy), &CELL(vt, vt->cx, vt->cy),
                sizeof(td_vcell_t) * (size_t)(vt->cols - vt->cx - k));
        clear_cells(vt, vt->cx, vt->cx + k, vt->cy);
        break;
    }
    case 'X':
        clear_cells(vt, vt->cx, clamp(vt->cx + count, 0, vt->cols), vt->cy);
        break;
    case 'S':
        scroll_up(vt, vt->top, vt->bottom, count);
        break;
    case 'T':
        scroll_down(vt, vt->top, vt->bottom, count);
        break;
    case 'm':
        sgr(vt, p, n);
        break;
    case 'r':
    {
        int top = (n > 0 && p[0] > 0) ? p[0] - 1 : 0;
        int bottom = (n > 1 && p[1] > 0) ? p[1] - 1 : vt->rows - 1;
        if (top < bottom && bottom < vt->rows)
        {
            vt->top = top;
            vt->bottom = bottom;
            vt->cx = vt->cy = 0;
        }
        break;
    }
    case 's':
        vt->saved_cx = vt->cx;
        vt->saved_cy = vt->cy;
        break;
    case 'u':
        vt->cx = vt->saved_cx;
        vt->cy = vt->saved_cy;
        break;
    case 'h':
        set_mode(vt, p, n, true);
        break;
    case 'l':
        set_mode(vt, p, n, false);
        break;
    case 'n':
        if (p0 == 5)
            reply(vt, "\x1b[0n");
        else if (p0 == 6)
        {
            snprintf(buf, sizeof(buf), "\x1b[%d;%dR", vt->cy + 1, vt->cx + 1);
            reply(vt, buf);
        }
        break;
    case 'c':
        if (!vt->priv && p0 == 0)
            reply(vt, vt->sixel ? "\x1b[?62;4c" : "\x1b[?1;2c");
        break;
    case 't':
        if (!vt->priv && p0 == 16 && vt->cell_w > 0 && vt->cell_h > 0)
        {
            snprintf(buf, sizeof(buf), "\x1b[6;%d;%dt", vt->cell_h, vt->cell_w);
            reply(vt, buf);
        }
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------ feeding */

static void esc_final(td_vterm_t *vt, uint8_t b)
{
    vt->state = VS_GROUND;
    switch (b)
    {
    case '[':
        vt->state = VS_CSI;
        vt->plen = 0;
        vt->priv = false;
        break;
    case ']':
        vt->state = VS_OSC;
        break;
    case 'P':
        vt->state = VS_DCS;
        if (vt->dcs)
            vt->dcs(vt->dcs_user, -1);
        break;
    case '(':
    case ')':
    case '*':
    case '+':
    case '#':
        vt->state = VS_SKIP1;
        break;
    case '7':
        vt->saved_cx = vt->cx;
        vt->saved_cy = vt->cy;
        vt->saved_fg = vt->fg;
        vt->saved_bg = vt->bg;
        vt->saved_attr = vt->attr;
        break;
    case '8':
        vt->cx = vt->saved_cx;
        vt->cy = vt->saved_cy;
        vt->fg = vt->saved_fg;
        vt->bg = vt->saved_bg;
        vt->attr = vt->saved_attr;
        vt->wrap_pending = false;
        break;
    case 'D':
        line_feed(vt);
        break;
    case 'E':
        vt->cx = 0;
        line_feed(vt);
        break;
    case 'M':
        if (vt->cy == vt->top)
            scroll_down(vt, vt->top, vt->bottom, 1);
        else if (vt->cy > 0)
            vt->cy--;
        break;
    case 'c':
    {
        int cols = vt->cols, rows = vt->rows;
        reset_state(vt);
        vt->cols = cols;
        vt->rows = rows;
        vt->bottom = rows - 1;
        erase_display(vt, 2);
        break;
    }
    default:
        break;
    }
}

static void feed_byte(td_vterm_t *vt, uint8_t b)
{
    switch (vt->state)
    {
    case VS_GROUND:
        if (b == 0x1B)
        {
            vt->state = VS_ESC;
        }
        else if (b < 0x20 || b == 0x7F)
        {
            control_char(vt, b);
        }
        else
        {
            uint32_t cps[2];
            int n = td_utf8_feed(&vt->utf8, b, cps);
            for (int i = 0; i < n; i++)
                put_char(vt, cps[i]);
        }
        break;
    case VS_ESC:
        esc_final(vt, b);
        break;
    case VS_CSI:
        if (b == 0x1B)
        {
            vt->state = VS_ESC;
            break;
        }
        if (b < 0x20)
        {
            control_char(vt, b);
            break;
        }   /* allowed mid-sequence */
        if (b == '?' && vt->plen == 0)
        {
            vt->priv = true;
            break;
        }
        if (b >= 0x40 && b <= 0x7E)
        {
            csi(vt, (char)b);
            vt->state = VS_GROUND;
        }
        else if (vt->plen < (int)sizeof(vt->params))
        {
            vt->params[vt->plen++] = (char)b;
        }
        break;
    case VS_OSC:
        if (b == 0x07)
            vt->state = VS_GROUND;
        else if (b == 0x1B)
            vt->state = VS_OSC_ESC;
        break;
    case VS_OSC_ESC:
        vt->state = (b == '\\') ? VS_GROUND : VS_OSC;
        break;
    case VS_DCS:
        if (b == 0x1B)
            vt->state = VS_DCS_ESC;
        else if (vt->dcs)
            vt->dcs(vt->dcs_user, b);
        break;
    case VS_DCS_ESC:
        /* ESC \ ends the string; any other ESC sequence ends it too. */
        if (vt->dcs)
            vt->dcs(vt->dcs_user, -2);
        if (b == '\\')
            vt->state = VS_GROUND;
        else
            esc_final(vt, b);
        break;
    case VS_SKIP1:
    default:
        vt->state = VS_GROUND;
        break;
    }
}

void td_vterm_write(td_vterm_t *vt, const uint8_t *data, int len)
{
    for (int i = 0; i < len; i++)
        feed_byte(vt, data[i]);
    if (len > 0)
        vt->dirty = true;
}

/* ------------------------------------------------------------ drawing */

void td_vterm_draw(const td_vterm_t *vt, int x, int y, int scroll_back, bool show_cursor)
{
    scroll_back = clamp(scroll_back, 0, vt->sb_count);
    for (int row = 0; row < vt->rows; row++)
    {
        int line = row - scroll_back;      /* < 0: scrollback */
        const td_vcell_t *src;
        if (line < 0)
        {
            int idx = (vt->sb_head + TD_VT_SCROLLBACK + line) % TD_VT_SCROLLBACK;
            src = &vt->sb[idx * TD_VT_MAX_COLS];
        }
        else
        {
            src = &vt->cells[line * TD_VT_MAX_COLS];
        }
        for (int col = 0; col < vt->cols; col++)
            td_putc(x + col, y + row, src[col].ch, src[col].fg, src[col].bg, 0);
    }
    if (show_cursor && vt->cursor_visible && scroll_back == 0)
    {
        const td_vcell_t *c = &vt->cells[vt->cy * TD_VT_MAX_COLS + vt->cx];
        td_putc(x + vt->cx, y + vt->cy, c->ch, c->fg, c->bg, TD_REVERSE);
    }
}
