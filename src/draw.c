/*
 * draw.c - drawing primitives. Everything is clipped to the current clip
 * rectangle so a window can never draw outside its client area.
 */
#include "tinydesk/td_screen.h"

static td_buffer_t *s_target;
static int s_ox, s_oy;          /* origin */
static td_rect_t s_clip;        /* absolute */
static bool s_ascii;

void td_draw_target(td_buffer_t *buf)
{
    s_target = buf;
    s_ox = 0;
    s_oy = 0;
    s_clip = td_rect(0, 0, buf ? buf->cols : 0, buf ? buf->rows : 0);
}

td_buffer_t *td_draw_get_target(void)
{
    return s_target;
}

void td_draw_origin(int x, int y)
{
    s_ox = x;
    s_oy = y;
}

void td_draw_clip(td_rect_t clip)
{
    if (!s_target)
        return;
    s_clip = td_rect_intersect(clip, td_rect(0, 0, s_target->cols, s_target->rows));
}

td_rect_t td_draw_get_clip(void)
{
    return s_clip;
}

void td_set_ascii_mode(bool on)
{
    s_ascii = on;
}
bool td_get_ascii_mode(void)
{
    return s_ascii;
}

/* Cell at origin-relative (x, y) if it is inside the clip, else NULL. */
static td_cell_t *clip_cell(int x, int y)
{
    if (!s_target)
        return NULL;
    x += s_ox;
    y += s_oy;
    if (!td_rect_contains(s_clip, x, y))
        return NULL;
    return &s_target->cells[y * s_target->cols + x];
}

void td_putc(int x, int y, uint32_t ch, uint8_t fg, uint8_t bg, uint8_t attr)
{
    td_cell_t *c = clip_cell(x, y);
    if (!c)
        return;
    c->ch = ch;
    c->fg = fg;
    c->bg = bg;
    c->attr = attr;
}

void td_fill(td_rect_t r, uint32_t ch, uint8_t fg, uint8_t bg)
{
    for (int y = r.y; y < r.y + r.h; y++)
        for (int x = r.x; x < r.x + r.w; x++)
            td_putc(x, y, ch, fg, bg, 0);
}

int td_textn(int x, int y, const char *str, int max_cols,
             uint8_t fg, uint8_t bg, uint8_t attr)
{
    int n = 0;
    if (!str)
        return 0;
    while (n < max_cols)
    {
        uint32_t cp = td_utf8_next(&str);
        if (cp == 0)
            break;
        if (cp < 0x20u)
            cp = ' ';   /* never put control characters on screen */
        td_putc(x + n, y, cp, fg, bg, attr);
        n++;
    }
    return n;
}

int td_textfit(int x, int y, const char *str, int max_cols,
               uint8_t fg, uint8_t bg, uint8_t attr)
{
    if (!str || max_cols < 1)
        return 0;
    int cols = 0, shown = 0;   /* code points, and up to the last non-space */
    for (const char *s = str; *s;)
    {
        bool space = *s == ' ';
        if (!td_utf8_next(&s))
            break;
        cols++;
        if (!space)
            shown = cols;
    }
    if (shown <= max_cols || max_cols < 2)
        return td_textn(x, y, str, max_cols, fg, bg, attr);
    int n = td_textn(x, y, str, max_cols - 1, fg, bg, attr);
    td_putc(x + n, y, 0x2026, fg, bg, attr);   /* … */
    return n + 1;
}

int td_text(int x, int y, const char *str, uint8_t fg, uint8_t bg, uint8_t attr)
{
    return td_textn(x, y, str, 0x7FFFFFFF, fg, bg, attr);
}

/* Corner and edge characters: tl, tr, bl, br, horizontal, vertical. */
static const uint32_t box_chars[3][6] = {
    {0x250C, 0x2510, 0x2514, 0x2518, 0x2500, 0x2502},  /* single */
    {0x2554, 0x2557, 0x255A, 0x255D, 0x2550, 0x2551},  /* double */
    {'+', '+', '+', '+', '-', '|'},                    /* ascii */
};

void td_box(td_rect_t r, td_box_style_t style, uint8_t fg, uint8_t bg)
{
    if (r.w < 2 || r.h < 2)
        return;
    const uint32_t *b = box_chars[style];
    int x1 = r.x + r.w - 1;
    int y1 = r.y + r.h - 1;
    for (int x = r.x + 1; x < x1; x++)
    {
        td_putc(x, r.y, b[4], fg, bg, 0);
        td_putc(x, y1, b[4], fg, bg, 0);
    }
    for (int y = r.y + 1; y < y1; y++)
    {
        td_putc(r.x, y, b[5], fg, bg, 0);
        td_putc(x1, y, b[5], fg, bg, 0);
    }
    td_putc(r.x, r.y, b[0], fg, bg, 0);
    td_putc(x1, r.y, b[1], fg, bg, 0);
    td_putc(r.x, y1, b[2], fg, bg, 0);
    td_putc(x1, y1, b[3], fg, bg, 0);
}

/* Shadow: keep the character underneath, but draw it dark grey on black. */
static void shade(int x, int y)
{
    td_cell_t *c = clip_cell(x, y);
    if (!c)
        return;
    c->fg = 8;
    c->bg = 0;
    c->attr = 0;
}

void td_shadow(td_rect_t r)
{
    for (int y = r.y + 1; y <= r.y + r.h; y++)
        shade(r.x + r.w, y);
    for (int x = r.x + 1; x < r.x + r.w; x++)
        shade(x, r.y + r.h);
}

void td_draw_vscroll(int x, int y, int h, int pos, int total, int page,
                     uint8_t fg, uint8_t bg)
{
    if (h < 3)
        return;
    td_putc(x, y, 0x25B2, fg, bg, 0);           /* up arrow */
    td_putc(x, y + h - 1, 0x25BC, fg, bg, 0);   /* down arrow */
    int track = h - 2;
    for (int i = 0; i < track; i++)
        td_putc(x, y + 1 + i, 0x2591, fg, bg, 0);

    if (total <= page || total <= 0)
        return;
    int thumb = track * page / total;
    if (thumb < 1)
        thumb = 1;
    int max_pos = total - page;
    if (pos > max_pos)
        pos = max_pos;
    if (pos < 0)
        pos = 0;
    int top = (track - thumb) * pos / max_pos;
    for (int i = 0; i < thumb; i++)
        td_putc(x, y + 1 + top + i, 0x2588, fg, bg, 0);
}

uint32_t td_ascii_fallback(uint32_t ch)
{
    if (ch < 0x80u)
        return ch;
    switch (ch)
    {
    /* single and double box drawing */
    case 0x2500:
    case 0x2550:
        return '-';
    case 0x2502:
    case 0x2551:
        return '|';
    case 0x250C:
    case 0x2510:
    case 0x2514:
    case 0x2518:
    case 0x2554:
    case 0x2557:
    case 0x255A:
    case 0x255D:
    case 0x251C:
    case 0x2524:
    case 0x252C:
    case 0x2534:
    case 0x253C:
        return '+';
    /* shades and blocks */
    case 0x2591:
        return '.';
    case 0x2592:
        return ':';
    case 0x2593:
    case 0x2588:
    case 0x25A0:
        return '#';
    case 0x2580:
    case 0x2584:
    case 0x258C:
    case 0x2590:
        return '#';
    case 0x00B7:
    case 0x2022:
        return '.';
    /* arrows and triangles */
    case 0x25B2:
        return '^';
    case 0x25BC:
        return 'v';
    case 0x25BA:
    case 0x2192:
        return '>';
    case 0x25C4:
    case 0x2190:
        return '<';
    case 0x2191:
        return '^';
    case 0x2193:
        return 'v';
    case 0x00D7:
        return 'x';
    case 0x2261:
        return '=';
    case 0x263C:
        return '*';
    default:
        return '?';
    }
}
