/*
 * screen.c - cell buffers and rectangle helpers.
 */
#include "tinydesk/td_screen.h"

static int max_i(int a, int b)
{
    return a > b ? a : b;
}
static int min_i(int a, int b)
{
    return a < b ? a : b;
}

td_rect_t td_rect_intersect(td_rect_t a, td_rect_t b)
{
    int x0 = max_i(a.x, b.x);
    int y0 = max_i(a.y, b.y);
    int x1 = min_i(a.x + a.w, b.x + b.w);
    int y1 = min_i(a.y + a.h, b.y + b.h);
    td_rect_t r = {x0, y0, max_i(0, x1 - x0), max_i(0, y1 - y0)};
    return r;
}

bool td_rect_contains(td_rect_t r, int x, int y)
{
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

void td_buffer_init(td_buffer_t *buf, int cols, int rows)
{
    if (cols < 1)
        cols = 1;
    if (rows < 1)
        rows = 1;
    if (cols > TD_MAX_SCREEN_COLS)
        cols = TD_MAX_SCREEN_COLS;
    if (rows > TD_MAX_SCREEN_ROWS)
        rows = TD_MAX_SCREEN_ROWS;
    if (cols * rows > TD_MAX_COLS * TD_MAX_ROWS)
        rows = TD_MAX_COLS * TD_MAX_ROWS / cols;   /* a wide extended screen */
    buf->cols = cols;
    buf->rows = rows;

    td_cell_t blank = {' ', 7, 0, 0, 0};
    for (int i = 0; i < cols * rows; i++)
        buf->cells[i] = blank;
}

void td_buffer_invalidate(td_buffer_t *buf)
{
    for (int i = 0; i < buf->cols * buf->rows; i++)
        buf->cells[i].ch = TD_CH_INVALID;
}

td_cell_t *td_buffer_cell(td_buffer_t *buf, int x, int y)
{
    if (x < 0 || y < 0 || x >= buf->cols || y >= buf->rows)
        return NULL;
    return &buf->cells[y * buf->cols + x];
}
