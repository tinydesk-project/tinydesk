/*
 * imagefile.c - BMP and PPM/PGM files read row by row (td_apps.h, the Image
 * Viewer). Only a window of the file (a few KB, at least one row) is in
 * memory, so an image larger than the RAM can still be shown scaled down.
 * PPM and PGM come in both forms: binary (P6, P5) and plain text (P3, P2),
 * which a terminal can type into a file.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "td_apps.h"

enum
{
    KIND_BMP,
    KIND_PPM,
    KIND_PGM
};

#define CHUNK 4096 /* file bytes kept, or one row if that is larger */

bool td_is_image(const char *name)
{
    const char *dot = name ? strrchr(name, '.') : NULL;
    if (!dot || strlen(dot) != 4)
        return false;
    char ext[4];
    for (int i = 0; i < 3; ++i)
        ext[i] = (char)tolower((unsigned char)dot[1 + i]);
    ext[3] = '\0';
    return strcmp(ext, "bmp") == 0 || strcmp(ext, "ppm") == 0 || strcmp(ext, "pgm") == 0;
}

static void say(char *err, int cap, const char *text)
{
    if (err && cap > 0)
    {
        strncpy(err, text, (size_t)cap - 1);
        err[cap - 1] = '\0';
    }
}

static uint32_t le16(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8;
}

static uint32_t le32(const uint8_t *p)
{
    return le16(p) | le16(p + 2) << 16;
}

/* Bytes [off, off + n) of the file, through the window; NULL on a short
 * read. n must not exceed buf_cap. */
static const uint8_t *bytes_at(td_image_t *img, uint32_t off, int n)
{
    if (off < img->buf_off || off + (uint32_t)n > img->buf_off + (uint32_t)img->buf_len)
    {
        int got = img->fs->read_at(img->path, off, (char *)img->buf, img->buf_cap);
        img->buf_off = off;
        img->buf_len = got > 0 ? got : 0;
        if (img->buf_len < n)
            return NULL;
    }
    return img->buf + (off - img->buf_off);
}

/* Lowest set bit and number of bits of a colour mask. */
static void mask_shape(uint32_t mask, int *shift, int *bits)
{
    *shift = 0;
    *bits = 0;
    if (!mask)
        return;
    while (!(mask & 1u))
    {
        mask >>= 1;
        (*shift)++;
    }
    while (mask & 1u)
    {
        mask >>= 1;
        (*bits)++;
    }
}

static uint8_t from_mask(const td_image_t *img, uint32_t px, int c)
{
    if (!img->bits[c])
        return 0;
    uint32_t max = img->bits[c] >= 32 ? 0xFFFFFFFFu : (1u << img->bits[c]) - 1u;
    uint64_t v = (px & img->mask[c]) >> img->shift[c];
    return (uint8_t)(v * 255u / max);
}

static bool load_bmp(td_image_t *img, const uint8_t *h, int n, char *err, int cap)
{
    if (n < 30)
    {
        say(err, cap, "The BMP file is cut short.");
        return false;
    }
    uint32_t dib = le32(h + 14);
    if (dib < 40 || n < 14 + 40)
    {
        say(err, cap, "This BMP format (an old OS/2 header) is not supported.");
        return false;
    }
    int32_t w = (int32_t)le32(h + 18), hh = (int32_t)le32(h + 22);
    uint32_t bpp = le16(h + 28), comp = le32(h + 30), colours = le32(h + 46);
    img->data = le32(h + 10);
    img->bottom_up = hh > 0;
    img->w = w;
    img->h = hh < 0 ? -hh : hh;
    img->bpp = (int)bpp;
    if (comp == 3 && (bpp == 16 || bpp == 32))
    {
        /* Masks in the header (V4/V5) or right after a 40-byte one. */
        uint32_t at = dib >= 52 ? 14 + 40 : 14 + dib;
        if ((int)at + 12 > n)
        {
            say(err, cap, "The BMP file is cut short.");
            return false;
        }
        img->mask[0] = le32(h + at);
        img->mask[1] = le32(h + at + 4);
        img->mask[2] = le32(h + at + 8);
    }
    else if (comp == 0 && bpp == 16)
    {
        img->mask[0] = 0x7C00u; /* 5-5-5 */
        img->mask[1] = 0x03E0u;
        img->mask[2] = 0x001Fu;
    }
    else if (comp == 0 && bpp == 32)
    {
        img->mask[0] = 0x00FF0000u;
        img->mask[1] = 0x0000FF00u;
        img->mask[2] = 0x000000FFu;
    }
    else if (comp != 0 || (bpp != 1 && bpp != 4 && bpp != 8 && bpp != 24))
    {
        say(err, cap, comp == 1 || comp == 2 ? "Compressed (RLE) BMP files are not supported." : "This kind of BMP file is not supported.");
        return false;
    }
    for (int c = 0; c < 3; ++c)
    {
        int shift, bits;
        mask_shape(img->mask[c], &shift, &bits);
        img->shift[c] = (uint8_t)shift;
        img->bits[c] = (uint8_t)bits;
    }
    if (bpp <= 8)
    {
        uint32_t count = colours ? colours : 1u << bpp;
        uint32_t at = 14 + dib;
        if (count > 256 || (int)(at + count * 4) > n)
        {
            say(err, cap, "The BMP colour table is damaged.");
            return false;
        }
        for (uint32_t i = 0; i < count; ++i)
        {
            img->palette[i][0] = h[at + i * 4 + 2];
            img->palette[i][1] = h[at + i * 4 + 1];
            img->palette[i][2] = h[at + i * 4];
        }
    }
    img->stride = ((uint32_t)img->w * bpp + 31u) / 32u * 4u;
    snprintf(img->format, sizeof(img->format), "BMP %u-bit", (unsigned)bpp);
    return true;
}

/* PPM/PGM header: magic, width, height, maxval, separated by white space
 * and # comments, then one white space byte (binary) or the samples as text
 * (plain). */
static bool load_pnm(td_image_t *img, const uint8_t *h, int n, char *err, int cap)
{
    int at = 2;
    long v[3];
    for (int i = 0; i < 3; ++i)
    {
        while (at < n && (isspace(h[at]) || h[at] == '#'))
        {
            if (h[at] == '#')
                while (at < n && h[at] != '\n')
                    at++;
            else
                at++;
        }
        if (at >= n || !isdigit(h[at]))
        {
            say(err, cap, "The image header is damaged.");
            return false;
        }
        v[i] = 0;
        while (at < n && isdigit(h[at]) && v[i] < 1000000L)
            v[i] = v[i] * 10 + (h[at++] - '0');
    }
    if (at >= n || !isspace(h[at]) || v[2] < 1 || v[2] > 65535)
    {
        say(err, cap, "The image header is damaged.");
        return false;
    }
    img->w = (int)v[0];
    img->h = (int)v[1];
    img->maxval = (int)v[2];
    img->data = (uint32_t)at + 1u;
    img->bottom_up = false;
    img->pos = img->data;
    int samples = img->kind == KIND_PPM ? 3 : 1, bytes = img->maxval > 255 ? 2 : 1;
    img->stride = img->plain ? 0 : (uint32_t)img->w * (uint32_t)(samples * bytes);
    snprintf(img->format, sizeof(img->format), "%s%s", img->kind == KIND_PPM ? "PPM" : "PGM",
             img->plain ? " text" : bytes == 2 ? " 16-bit"
                                               : "");
    return true;
}

bool td_image_load(td_image_t *img, const td_fs_ops_t *fs, const char *path, char *err, int errcap)
{
    memset(img, 0, sizeof(*img));
    if (!fs || !fs->read_at)
    {
        say(err, errcap, "This system cannot read files in parts.");
        return false;
    }
    img->fs = fs;
    snprintf(img->path, sizeof(img->path), "%s", path);
    img->buf_cap = CHUNK;
    img->buf = malloc(CHUNK);
    if (!img->buf)
    {
        say(err, errcap, "Not enough memory to read the image.");
        return false;
    }
    const uint8_t *h = img->buf;
    int n = fs->read_at(path, 0, (char *)img->buf, CHUNK);
    img->buf_len = n > 0 ? n : 0;
    if (n < 2)
    {
        say(err, errcap, n < 0 ? "The file cannot be read." : "The file is empty.");
        td_image_close(img);
        return false;
    }
    bool ok;
    if (h[0] == 'B' && h[1] == 'M')
    {
        img->kind = KIND_BMP;
        ok = load_bmp(img, h, n, err, errcap);
    }
    else if (h[0] == 'P' && h[1] >= '2' && h[1] <= '6' && h[1] != '4')
    {
        img->kind = h[1] == '6' || h[1] == '3' ? KIND_PPM : KIND_PGM;
        img->plain = h[1] <= '3';
        ok = load_pnm(img, h, n, err, errcap);
    }
    else
    {
        say(err, errcap, "Not a BMP, PPM or PGM image.");
        ok = false;
    }
    if (ok && (img->w < 1 || img->h < 1))
    {
        say(err, errcap, "The image has no pixels.");
        ok = false;
    }
    if (ok && (img->w > TD_IMAGE_MAX_W || img->h > TD_IMAGE_MAX_H))
    {
        say(err, errcap, "The image is too large (at most 4096 x 32768 pixels).");
        ok = false;
    }
    if (ok && img->stride > CHUNK)
    {
        /* A row larger than the window: the window becomes one row. */
        uint8_t *bigger = realloc(img->buf, img->stride);
        if (!bigger)
        {
            say(err, errcap, "Not enough memory to read the image.");
            ok = false;
        }
        else
        {
            img->buf = bigger;
            img->buf_cap = (int)img->stride;
        }
    }
    if (!ok)
        td_image_close(img);
    return ok;
}

void td_image_rewind(td_image_t *img)
{
    img->next = 0;
    img->pos = img->data;
}

void td_image_close(td_image_t *img)
{
    free(img->buf);
    img->buf = NULL;
}

/* Plain PPM/PGM: the next decimal sample, skipping white space and #
 * comments; -1 at the end of the file or on anything else. */
static long text_sample(td_image_t *img)
{
    const uint8_t *c;
    while ((c = bytes_at(img, img->pos, 1)) != NULL && (isspace(*c) || *c == '#'))
    {
        if (*c == '#')
            while ((c = bytes_at(img, img->pos, 1)) != NULL && *c != '\n')
                img->pos++;
        else
            img->pos++;
    }
    if (!c || !isdigit(*c))
        return -1;
    long v = 0;
    while ((c = bytes_at(img, img->pos, 1)) != NULL && isdigit(*c))
    {
        if (v < 1000000L)
            v = v * 10 + (*c - '0');
        img->pos++;
    }
    return v;
}

static bool text_row(td_image_t *img, uint8_t *rgb)
{
    bool colour = img->kind == KIND_PPM;
    for (int x = 0; x < img->w; ++x)
    {
        for (int c = 0; c < (colour ? 3 : 1); ++c)
        {
            long v = text_sample(img);
            if (v < 0)
                return false;
            if (v > img->maxval)
                v = img->maxval;
            rgb[x * 3 + c] = (uint8_t)((uint32_t)v * 255u / (uint32_t)img->maxval);
        }
        if (!colour)
            rgb[x * 3 + 1] = rgb[x * 3 + 2] = rgb[x * 3];
    }
    return true;
}

bool td_image_row(td_image_t *img, int *y, uint8_t *rgb)
{
    if (!img->buf || img->next >= img->h)
        return false;
    if (img->plain)
    {
        if (!text_row(img, rgb))
            return false;
        *y = img->next++;
        return true;
    }
    const uint8_t *p = bytes_at(img, img->data + (uint32_t)img->next * img->stride, (int)img->stride);
    if (!p)
        return false;
    *y = img->bottom_up ? img->h - 1 - img->next : img->next;
    img->next++;

    if (img->kind != KIND_BMP)
    {
        int wide = img->maxval > 255, samples = img->kind == KIND_PPM ? 3 : 1;
        for (int x = 0; x < img->w; ++x)
        {
            for (int c = 0; c < 3; ++c)
            {
                int s = samples == 3 ? c : 0;
                const uint8_t *q = p + (x * samples + s) * (wide ? 2 : 1);
                uint32_t v = wide ? (uint32_t)q[0] << 8 | q[1] : q[0];
                rgb[x * 3 + c] = (uint8_t)(v * 255u / (uint32_t)img->maxval);
            }
        }
        return true;
    }

    for (int x = 0; x < img->w; ++x)
    {
        uint8_t *o = rgb + x * 3;
        switch (img->bpp)
        {
        case 1:
        case 4:
        case 8:
        {
            int bit = x * img->bpp;
            int idx = (p[bit / 8] >> (8 - img->bpp - bit % 8)) & ((1 << img->bpp) - 1);
            memcpy(o, img->palette[idx], 3);
            break;
        }
        case 24:
            o[0] = p[x * 3 + 2];
            o[1] = p[x * 3 + 1];
            o[2] = p[x * 3];
            break;
        default: /* 16 or 32 with masks */
        {
            uint32_t px = img->bpp == 16 ? le16(p + x * 2) : le32(p + x * 4);
            o[0] = from_mask(img, px, 0);
            o[1] = from_mask(img, px, 1);
            o[2] = from_mask(img, px, 2);
            break;
        }
        }
    }
    return true;
}
