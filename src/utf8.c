/*
 * utf8.c - UTF-8 encoding and decoding.
 */
#include "tinydesk/td_screen.h"

#include <string.h>

#define REPLACEMENT 0xFFFDu

int td_utf8_encode(uint32_t cp, uint8_t out[4])
{
    if (cp > 0x10FFFFu || (cp >= 0xD800u && cp <= 0xDFFFu))
        cp = REPLACEMENT;

    if (cp < 0x80u)
    {
        out[0] = (uint8_t)cp;
        return 1;
    }
    if (cp < 0x800u)
    {
        out[0] = (uint8_t)(0xC0u | (cp >> 6));
        out[1] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 2;
    }
    if (cp < 0x10000u)
    {
        out[0] = (uint8_t)(0xE0u | (cp >> 12));
        out[1] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
        out[2] = (uint8_t)(0x80u | (cp & 0x3Fu));
        return 3;
    }
    out[0] = (uint8_t)(0xF0u | (cp >> 18));
    out[1] = (uint8_t)(0x80u | ((cp >> 12) & 0x3Fu));
    out[2] = (uint8_t)(0x80u | ((cp >> 6) & 0x3Fu));
    out[3] = (uint8_t)(0x80u | (cp & 0x3Fu));
    return 4;
}

/* Number of continuation bytes announced by a lead byte, or -1 if the byte
 * cannot start a sequence. */
static int lead_length(uint8_t b, uint32_t *initial)
{
    if (b < 0x80u)
    {
        *initial = b;
        return 0;
    }
    if (b >= 0xC2u && b <= 0xDFu)
    {
        *initial = b & 0x1Fu;
        return 1;
    }
    if (b >= 0xE0u && b <= 0xEFu)
    {
        *initial = b & 0x0Fu;
        return 2;
    }
    if (b >= 0xF0u && b <= 0xF4u)
    {
        *initial = b & 0x07u;
        return 3;
    }
    return -1;
}

/* Reject overlong forms, surrogates and values above U+10FFFF. */
static bool valid_result(uint32_t cp, int extra)
{
    static const uint32_t min_for_len[4] = {0, 0x80u, 0x800u, 0x10000u};
    if (cp < min_for_len[extra])
        return false;
    if (cp > 0x10FFFFu)
        return false;
    if (cp >= 0xD800u && cp <= 0xDFFFu)
        return false;
    return true;
}

uint32_t td_utf8_next(const char **s)
{
    const uint8_t *p = (const uint8_t *)*s;
    if (*p == 0)
        return 0;

    uint32_t cp;
    int extra = lead_length(*p, &cp);
    if (extra < 0)
    {
        *s += 1;
        return REPLACEMENT;
    }
    for (int i = 1; i <= extra; i++)
    {
        if ((p[i] & 0xC0u) != 0x80u)
        {
            *s += 1;
            return REPLACEMENT;
        }
        cp = (cp << 6) | (p[i] & 0x3Fu);
    }
    if (!valid_result(cp, extra))
    {
        *s += 1;
        return REPLACEMENT;
    }
    *s += 1 + extra;
    return cp;
}

int td_utf8_len(const char *s)
{
    int n = 0;
    while (td_utf8_next(&s) != 0)
        n++;
    return n;
}

/* Copy whole code points as td_utf8_next() reads them (a malformed byte is
 * one cell on its own), so the copy draws the same cells. */
static size_t copy_cells(char *out, size_t cap, const char *s, int cols, int *used)
{
    size_t n = 0;
    *used = 0;
    while (s && *used < cols)
    {
        const char *start = s;
        if (td_utf8_next(&s) == 0)
            break;
        size_t len = (size_t)(s - start);
        if (n + len >= cap)
            break;
        memcpy(out + n, start, len);
        n += len;
        (*used)++;
    }
    out[n] = '\0';
    return n;
}

int td_utf8_copy(char *out, size_t cap, const char *s, int cols)
{
    if (!out || cap == 0)
        return 0;
    int used;
    return (int)copy_cells(out, cap, s, cols, &used);
}

int td_utf8_pad(char *out, size_t cap, const char *s, int cols)
{
    if (!out || cap == 0)
        return 0;
    int used;
    size_t n = copy_cells(out, cap, s, cols, &used);
    while (used < cols && n + 1 < cap)
    {
        out[n++] = ' ';
        used++;
    }
    out[n] = '\0';
    return (int)n;
}

const char *td_utf8_skip(const char *s, int cols)
{
    if (!s)
        return s;
    for (int i = 0; i < cols; i++)
    {
        const char *before = s;
        if (td_utf8_next(&s) == 0)
            return before;
    }
    return s;
}

int td_wrap_line(const char *s, int cols, const char **next)
{
    const char *p = s ? s : "", *q = p, *space = NULL;
    if (cols < 1)
        cols = 1;
    for (int col = 0; *q && *q != '\n'; col++)
    {
        if (col == cols)
        {
            if (*q == ' ')   /* the line ends exactly before a space */
                space = q;
            if (space && space > p)
            {
                *next = space + 1;
                return (int)(space - p);
            }
            *next = q;   /* one word wider than the line: break inside it */
            return (int)(q - p);
        }
        if (*q == ' ')
            space = q;
        td_utf8_next(&q);
    }
    *next = *q == '\n' ? q + 1 : q;
    return (int)(q - p);
}

/* Start a new sequence with byte; returns 1 if it completed at once. */
static int start_sequence(td_utf8_decoder_t *d, uint8_t byte, uint32_t *out)
{
    uint32_t initial;
    int extra = lead_length(byte, &initial);
    if (extra < 0)
    {
        *out = REPLACEMENT;
        return 1;
    }
    if (extra == 0)
    {
        *out = byte;
        return 1;
    }
    d->cp = initial;
    d->need = (uint8_t)extra;
    d->len = (uint8_t)extra;
    return 0;
}

int td_utf8_feed(td_utf8_decoder_t *d, uint8_t byte, uint32_t out[2])
{
    if (d->need == 0)
        return start_sequence(d, byte, &out[0]);

    if ((byte & 0xC0u) == 0x80u)
    {
        d->cp = (d->cp << 6) | (byte & 0x3Fu);
        if (--d->need > 0)
            return 0;
        out[0] = valid_result(d->cp, d->len) ? d->cp : REPLACEMENT;
        return 1;
    }

    /* The sequence was cut short: report it, then treat byte afresh. */
    d->need = 0;
    out[0] = REPLACEMENT;
    return 1 + start_sequence(d, byte, &out[1]);
}
