/*
 * vtshot.c - replay a byte capture through td_vterm and print the screen as
 * text, or render it as an SVG picture with its colours. Pairs with
 * tools/serial_probe.py (captures from real hardware) and tdsim.
 *
 *   vtshot capture.bin [cols rows]
 *   vtshot capture.bin cols rows --svg out.svg [--rows FIRST-LAST] [--title TEXT]
 *
 * --rows keeps only screen rows FIRST..LAST (0-based) in the picture;
 * --title draws a window frame with that title around it.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tinydesk/td_vterm.h"

static td_vterm_t s_vt;

/* The 256-colour palette as RGB: Windows Terminal's "Campbell" scheme for
 * the 16 basic colours (what a current terminal shows), then the standard
 * xterm cube and grey ramp. */
static void palette(int c, int *r, int *g, int *b)
{
    static const unsigned char base[16][3] = {
        {12, 12, 12},
        {197, 15, 31},
        {19, 161, 14},
        {193, 156, 0},
        {0, 55, 218},
        {136, 23, 152},
        {58, 150, 221},
        {204, 204, 204},
        {118, 118, 118},
        {231, 72, 86},
        {22, 198, 12},
        {249, 241, 165},
        {59, 120, 255},
        {180, 0, 158},
        {97, 214, 214},
        {242, 242, 242},
    };
    if (c < 16)
    {
        *r = base[c][0], *g = base[c][1], *b = base[c][2];
    }
    else if (c < 232)
    {
        static const int level[6] = {0, 95, 135, 175, 215, 255};
        c -= 16;
        *r = level[c / 36], *g = level[(c / 6) % 6], *b = level[c % 6];
    }
    else
    {
        *r = *g = *b = 8 + (c - 232) * 10;
    }
}

static void colour(FILE *o, int c)
{
    int r, g, b;
    palette(c, &r, &g, &b);
    fprintf(o, "#%02x%02x%02x", r, g, b);
}

static void xml_char(FILE *o, uint16_t ch)
{
    if (ch == '<')
        fputs("&lt;", o);
    else if (ch == '>')
        fputs("&gt;", o);
    else if (ch == '&')
        fputs("&amp;", o);
    else
    {
        uint8_t u[4];
        int len = td_utf8_encode(ch ? ch : ' ', u);
        fwrite(u, 1, (size_t)len, o);
    }
}

/* The quarters a block character fills (bit 3 upper left, 2 upper right,
 * 1 lower left, 0 lower right), or 0 for any other character. */
static int block_mask(uint32_t ch)
{
    static const uint32_t glyph[16] = {0, 0x2597, 0x2596, 0x2584, 0x259D, 0x2590, 0x259E, 0x259F,
                                       0x2598, 0x259A, 0x258C, 0x2599, 0x2580, 0x259C, 0x259B, 0x2588};
    for (int m = 1; m < 16; ++m)
        if (glyph[m] == ch)
            return m;
    return 0;
}

/* The filled quarters of a block character as rectangles: exact, whatever
 * fonts the viewer of the picture has (canvases use them for pixels). */
static void block_rects(FILE *o, int mask, int x, int y, int cw, int ch, int fg)
{
    double hw = cw / 2.0;
    int hh = ch / 2;
    for (int half = 0; half < 2; ++half) /* upper, lower */
    {
        int bits = (mask >> (2 - 2 * half)) & 3; /* 2 left, 1 right */
        if (!bits)
            continue;
        double rx = bits == 1 ? x + hw : x, rw = bits == 3 ? cw : hw;
        fprintf(o, "<rect x=\"%g\" y=\"%d\" width=\"%g\" height=\"%d\" fill=\"", rx, y + half * hh, rw,
                half ? ch - hh : hh);
        colour(o, fg);
        fputs("\"/>\n", o);
    }
}

/* One rectangle per run of equal background, block characters as
 * rectangles, one <text> per run of equal foreground; textLength pins every
 * run to its cells, so box drawing characters from a fallback font cannot
 * shift the columns. */
static int write_svg(const char *path, int y0, int y1, const char *title)
{
    const int cw = 9, ch = 18, pad = 10;
    int cols = s_vt.cols, rows = y1 - y0 + 1;
    int top = title ? 30 : 0;
    int w = cols * cw + 2 * pad, h = rows * ch + 2 * pad + top;
    FILE *o = fopen(path, "wb");
    if (!o)
    {
        perror(path);
        return 1;
    }
    fprintf(o, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %d %d\" width=\"%d\" height=\"%d\" "
               "font-family=\"'DejaVu Sans Mono', Consolas, Menlo, 'Courier New', monospace\" font-size=\"15\">\n",
            w, h, w, h);
    fprintf(o, "<rect width=\"%d\" height=\"%d\" rx=\"10\" fill=\"#1b1f27\"/>\n", w, h);
    if (title)
    {
        fprintf(o, "<circle cx=\"20\" cy=\"15\" r=\"5.5\" fill=\"#ff5f57\"/><circle cx=\"38\" cy=\"15\" r=\"5.5\" fill=\"#febc2e\"/>"
                   "<circle cx=\"56\" cy=\"15\" r=\"5.5\" fill=\"#28c840\"/>\n");
        fprintf(o, "<text x=\"%d\" y=\"20\" fill=\"#9aa4ad\" font-size=\"13\" text-anchor=\"middle\">", w / 2);
        for (const char *p = title; *p; p++)
            xml_char(o, (uint16_t)(unsigned char)*p);
        fputs("</text>\n", o);
    }
    /* crispEdges: the cell backgrounds meet without seams between rows. */
    fprintf(o, "<g transform=\"translate(%d %d)\" shape-rendering=\"crispEdges\">\n", pad, pad + top);
    for (int y = y0; y <= y1; y++)
    {
        const td_vcell_t *row = &s_vt.cells[y * TD_VT_MAX_COLS];
        int py = (y - y0) * ch;
        for (int x = 0; x < cols;)
        {
            int x2 = x;
            while (x2 < cols && row[x2].bg == row[x].bg)
                x2++;
            fprintf(o, "<rect x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" fill=\"", x * cw, py, (x2 - x) * cw, ch);
            colour(o, row[x].bg);
            fputs("\"/>\n", o);
            x = x2;
        }
        for (int x = 0; x < cols; x++)
        {
            int mask = block_mask(row[x].ch);
            if (mask)
                block_rects(o, mask, x * cw, py, cw, ch, row[x].fg);
        }
        for (int x = 0; x < cols;)
        {
            int x2 = x;
            bool blank = true;
            while (x2 < cols && row[x2].fg == row[x].fg)
            {
                if (row[x2].ch && row[x2].ch != ' ' && !block_mask(row[x2].ch))
                    blank = false;
                x2++;
            }
            if (!blank)
            {
                fprintf(o, "<text x=\"%d\" y=\"%d\" textLength=\"%d\" lengthAdjust=\"spacingAndGlyphs\" "
                           "xml:space=\"preserve\" fill=\"",
                        x * cw, py + 14, (x2 - x) * cw);
                colour(o, row[x].fg);
                fputs("\">", o);
                for (int i = x; i < x2; i++)
                    xml_char(o, block_mask(row[i].ch) ? ' ' : row[i].ch);
                fputs("</text>\n", o);
            }
            x = x2;
        }
    }
    fputs("</g>\n</svg>\n", o);
    fclose(o);
    printf("%s: %dx%d cells\n", path, cols, rows);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: vtshot capture.bin [cols rows] [--svg out.svg [--rows A-B] [--title TEXT]]\n");
        return 2;
    }
    int cols = argc > 3 && argv[2][0] != '-' ? atoi(argv[2]) : 80;
    int rows = argc > 3 && argv[2][0] != '-' ? atoi(argv[3]) : 25;
    const char *svg = NULL, *title = NULL;
    int y0 = 0, y1 = -1;
    for (int i = 2; i < argc; i++)
    {
        if (!strcmp(argv[i], "--svg") && i + 1 < argc)
            svg = argv[++i];
        else if (!strcmp(argv[i], "--title") && i + 1 < argc)
            title = argv[++i];
        else if (!strcmp(argv[i], "--rows") && i + 1 < argc)
            sscanf(argv[++i], "%d-%d", &y0, &y1);
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f)
    {
        perror(argv[1]);
        return 1;
    }
    td_vterm_init(&s_vt, cols, rows);
    uint8_t buf[4096];
    size_t n;
    unsigned long total = 0;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
    {
        td_vterm_write(&s_vt, buf, (int)n);
        total += (unsigned long)n;
    }
    fclose(f);

    if (svg)
    {
        if (y1 < 0 || y1 >= s_vt.rows)
            y1 = s_vt.rows - 1;
        if (y0 < 0 || y0 > y1)
            y0 = 0;
        return write_svg(svg, y0, y1, title);
    }
    for (int y = 0; y < s_vt.rows; y++)
    {
        putchar('|');
        for (int x = 0; x < s_vt.cols; x++)
        {
            uint8_t u[4];
            int len = td_utf8_encode(s_vt.cells[y * TD_VT_MAX_COLS + x].ch, u);
            fwrite(u, 1, (size_t)len, stdout);
        }
        printf("|\n");
    }
    printf("(%lu bytes replayed)\n", total);
    return 0;
}
