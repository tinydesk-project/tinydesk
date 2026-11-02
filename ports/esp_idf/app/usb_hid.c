/*
 * usb_hid.c - a USB keyboard and mouse on the ESP32-S3's own USB port.
 *
 * ESP-IDF's USB host library and Espressif's HID host driver
 * (espressif/usb_host_hid) find keyboards and mice. Keyboards send boot
 * reports; their keys become the bytes PuTTY sends, in the chosen layout
 * (usb_hid_layouts.h, made by tools/make_kbd_layouts.py). Mice keep
 * their own report format, read from their report descriptor, so that the
 * wheel works (boot reports have none), with boot reports as the fallback;
 * a mouse's movement moves a pointer cell by cell, and its buttons and
 * wheel become SGR mouse reports at the pointer. Both wait in a small ring for link_read().
 * ESP-IDF 5.3 has no hub support: one device on the port, such as a
 * wireless keyboard and mouse set with one receiver.
 *
 * Board configuration keys (tdsh_board.h; the Board configuration page of the docs):
 *   usb.host         1: USB host on GPIO19/20 (default 0)
 *   usb.mouse_speed  pointer speed in percent (default 100: 8 counts a
 *                    column, 16 a row, about one pixel of an 8x16 font)
 *   usb.mouse_accel  acceleration in percent (default 100: fast moves go
 *                    up to 4 times as far; 0: off)
 *   usb.mouse_precision  slow moves in percent (default 50: they go 0.7
 *                    times as far, for aiming; 100: 0.4 times; 0: off)
 *   usb.kbd_layout   us (default), uk, de, fr or es
 *   usb.kbd_delay    ms before a held key repeats (default 500)
 *   usb.kbd_rate     repeats a second (default 30)
 * The Mouse and Keyboard apps change these (usb_hid_mouse_ops(),
 * usb_hid_keyboard_ops()).
 */
#include "usb_hid.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "tdsh_board.h"
#include "tinydesk/td_wm.h"
#include "usb/hid_host.h"
#include "usb/usb_host.h"

#define RING_SIZE 1024
#define TICK_MS   20

static const char *TAG = "usb_hid";

/* ------------------------------------------------------------ the ring */

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static uint8_t s_ring[RING_SIZE];
static int s_head, s_tail;

/* A whole key or mouse report, or nothing: a sequence is never cut. */
static void put(const char *data, int len)
{
    taskENTER_CRITICAL(&s_mux);
    int free_bytes = (s_tail - s_head - 1 + RING_SIZE) % RING_SIZE;
    if (len <= free_bytes)
        for (int i = 0; i < len; i++)
        {
            s_ring[s_head] = (uint8_t)data[i];
            s_head = (s_head + 1) % RING_SIZE;
        }
    taskEXIT_CRITICAL(&s_mux);
}

int usb_hid_read(uint8_t *buf, int cap)
{
    int n = 0;
    taskENTER_CRITICAL(&s_mux);
    while (n < cap && s_tail != s_head)
    {
        buf[n++] = s_ring[s_tail];
        s_tail = (s_tail + 1) % RING_SIZE;
    }
    taskEXIT_CRITICAL(&s_mux);
    return n;
}

/* -------------------------------------------------------- the keyboard */

#define MOD_CTRL  0x11
#define MOD_SHIFT 0x22
#define MOD_ALT   0x44
#define MOD_LALT  0x04
#define MOD_RALT  0x40             /* AltGr in layouts that have one */
#define MOD_GUI   0x88             /* the Windows keys */

typedef struct
{
    const char *code, *name;       /* usb.kbd_layout value, shown name */
    const char *plain, *shift, *altgr;   /* 54 keys each; altgr "": none */
} usb_layout_t;

#include "usb_hid_layouts.h"

#define LAYOUTS ((int)(sizeof(s_layouts) / sizeof(s_layouts[0])))

static int s_kbd_layout;               /* index into s_layouts */
static int64_t s_delay_us = 500000, s_rate_us = 33333;
static td_keyboard_settings_t s_kbd_settings = {0, 500, 30};
static int s_kbds;                 /* keyboards attached */
static volatile uint8_t s_kbd_mods; /* modifier keys held, for mouse reports */
static bool s_gui_alone;            /* the Windows key is held, nothing else yet */
static bool s_caps, s_num = true;

/* The idx-th key's character (UTF-8) of a layout row; 0 bytes for none. */
static int key_char(const char *s, int idx, char *out)
{
    while (*s && idx > 0)
    {
        s++;
        while ((*s & 0xC0) == 0x80)
            s++;
        idx--;
    }
    if (!*s || *s == '\001')
        return 0;
    int n = 0;
    do
        out[n] = s[n];
    while ((s[++n] & 0xC0) == 0x80 && n < 4);
    return n;
}

/* A lower-case letter, which Caps Lock turns into its Shift character. */
static bool lower_letter(const char *c, int n)
{
    if (n == 1)
        return c[0] >= 'a' && c[0] <= 'z';
    /* U+00E0-U+00FE (a grave to thorn) but U+00F7 (division sign) */
    return n == 2 && (uint8_t)c[0] == 0xC3 && (uint8_t)c[1] >= 0xA0 && (uint8_t)c[1] <= 0xBE &&
           (uint8_t)c[1] != 0xB7;
}

/* xterm's modifier parameter: 1 + shift + 2 alt + 4 ctrl + 8 super. */
static int mod_param(uint8_t mods)
{
    return 1 + ((mods & MOD_SHIFT) ? 1 : 0) + ((mods & MOD_ALT) ? 2 : 0) + ((mods & MOD_CTRL) ? 4 : 0) +
           ((mods & MOD_GUI) ? 8 : 0);
}

/* ESC [ n ~ with modifiers (Insert, Delete, Home, End, pages, F keys). */
static int tilde(char *out, int n, uint8_t mods)
{
    int m = mod_param(mods);
    return m > 1 ? sprintf(out, "\x1b[%d;%d~", n, m) : sprintf(out, "\x1b[%d~", n);
}

/* ESC [ A-D with modifiers (the arrows). */
static int arrow(char *out, char letter, uint8_t mods)
{
    int m = mod_param(mods);
    return m > 1 ? sprintf(out, "\x1b[1;%d%c", m, letter) : sprintf(out, "\x1b[%c", letter);
}

/* The bytes for one key press; 0 for keys that send nothing. */
static int key_bytes(uint8_t usage, uint8_t mods, char *out)
{
    static const uint8_t f_keys[12] = {11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 23, 24};
    bool shift = mods & MOD_SHIFT, ctrl = mods & MOD_CTRL, alt = mods & MOD_ALT;
    char c = 0;

    /* The keypad: digits with Num Lock, otherwise the keys printed below them. */
    if (usage >= 0x54 && usage <= 0x63)
    {
        static const char pad[] = "/*-+\r1234567890.";
        static const uint8_t nav[] = {0x4D, 0x51, 0x4E, 0x50, 0, 0x4F, 0x4A, 0x52, 0x4B, 0x49, 0x4C};
        int i = usage - 0x54;
        if (i >= 5 && !s_num)
            return nav[i - 5] ? key_bytes(nav[i - 5], mods, out) : 0;
        c = pad[i];
    }
    else if (usage >= 0x28 && usage <= 0x2C)
    {
        if (usage == 0x2B && shift)      /* Shift+Tab; with Alt: the previous window */
            return mod_param(mods) > 2 ? sprintf(out, "\x1b[1;%dZ", mod_param(mods)) : sprintf(out, "\x1b[Z");
        if (usage == 0x29 && ctrl)       /* Ctrl+Esc: the Start menu (F10) */
            return sprintf(out, "\x1b[21~");
        c = "\r\x1b\x7f\t "[usage - 0x28];   /* Enter, Esc, Backspace, Tab, Space */
    }
    else if ((usage >= 0x04 && usage <= 0x38) || usage == 0x64)
    {
        /* Letters, digits and punctuation, from the layout. */
        const usb_layout_t *l = &s_layouts[s_kbd_layout];
        int idx = usage == 0x64 ? 53 : usage - 0x04;
        if (mods & MOD_GUI)
        {
            /* Windows key + letter: the desktop's shortcuts, as xterm's
             * modifyOtherKeys sequence; nothing for other keys. */
            char ch[4];
            int n = key_char(l->plain, idx, ch);
            if (n != 1 || ch[0] < 'a' || ch[0] > 'z')
                return 0;
            return sprintf(out, "\x1b[27;%d;%d~", mod_param(mods), ch[0]);
        }
        bool altgr = (mods & MOD_RALT) && l->altgr[0];
        char ch[4];
        int n;
        if (altgr)
            n = key_char(l->altgr, idx, ch);
        else
        {
            n = key_char(l->plain, idx, ch);
            bool upper = shift != (s_caps && lower_letter(ch, n));
            if (upper)
                n = key_char(l->shift, idx, ch);
        }
        if (n == 1 && ctrl && !altgr)
        {
            char k = ch[0];
            if ((k >= 'a' && k <= 'z') || (k >= 'A' && k <= 'Z'))
                ch[0] = (char)((k | 0x20) - 'a' + 1);
            else if (k == '[' || k == '{')
                ch[0] = 0x1B;
            else if (k == '\\' || k == '|')
                ch[0] = 0x1C;
            else if (k == ']' || k == '}')
                ch[0] = 0x1D;
        }
        if (n == 0)
            return 0;
        int k = 0;
        if ((mods & MOD_LALT) || ((mods & MOD_RALT) && !l->altgr[0]))
            out[k++] = 0x1B;
        memcpy(out + k, ch, (size_t)n);
        return k + n;
    }
    else if (usage >= 0x3A && usage <= 0x45)
        return tilde(out, f_keys[usage - 0x3A], mods);
    else
        switch (usage)
        {
        case 0x39:
            s_caps = !s_caps;
            return 0;
        case 0x53:
            s_num = !s_num;
            return 0;
        case 0x49:
            return tilde(out, 2, mods);         /* Insert */
        case 0x4A:
            return tilde(out, 1, mods);         /* Home */
        case 0x4B:
            return tilde(out, 5, mods);         /* Page Up */
        case 0x4C:
            return tilde(out, 3, mods);         /* Delete */
        case 0x4D:
            return tilde(out, 4, mods);         /* End */
        case 0x4E:
            return tilde(out, 6, mods);         /* Page Down */
        case 0x4F:
            return arrow(out, 'C', mods);
        case 0x50:
            return arrow(out, 'D', mods);
        case 0x51:
            return arrow(out, 'B', mods);
        case 0x52:
            return arrow(out, 'A', mods);
        default:
            return 0;
        }
    if (!c)
        return 0;
    int n = 0;
    if (alt)
        out[n++] = 0x1B;
    out[n++] = c;
    return n;
}

static uint8_t s_keys[6];          /* the last report's keys */
static uint8_t s_rep_usage, s_rep_mods;
static int64_t s_rep_at;

static void keyboard_report(const uint8_t *r, size_t len)
{
    if (len < 8 || r[2] == 0x01)   /* too short, or too many keys at once */
        return;
    char out[16];
    uint8_t mods = r[0];
    /* The Windows key pressed and let go on its own opens the Start menu. */
    if ((mods & MOD_GUI) && !(s_kbd_mods & MOD_GUI))
        s_gui_alone = true;
    if ((mods & ~MOD_GUI) || r[2])
        s_gui_alone = false;
    if (!(mods & MOD_GUI) && (s_kbd_mods & MOD_GUI) && s_gui_alone)
    {
        put("\x1b[21~", 5);
        s_gui_alone = false;
    }
    s_kbd_mods = mods;
    taskENTER_CRITICAL(&s_mux);
    bool repeat_held = false;
    for (int i = 2; i < 8; i++)
        if (r[i] && r[i] == s_rep_usage)
            repeat_held = true;
    if (!repeat_held)
        s_rep_usage = 0;
    s_rep_mods = mods;
    taskEXIT_CRITICAL(&s_mux);
    for (int i = 2; i < 8; i++)
    {
        uint8_t u = r[i];
        if (!u || memchr(s_keys, u, sizeof(s_keys)))
            continue;              /* none, or held since the last report */
        int n = key_bytes(u, mods, out);
        if (n > 0)
            put(out, n);
        taskENTER_CRITICAL(&s_mux);
        s_rep_usage = (n > 0 && u != 0x39 && u != 0x53) ? u : 0;
        s_rep_at = esp_timer_get_time() + s_delay_us;
        taskEXIT_CRITICAL(&s_mux);
    }
    memcpy(s_keys, r + 2, sizeof(s_keys));
}

static void keyboard_repeat(void)
{
    int64_t now = esp_timer_get_time();
    taskENTER_CRITICAL(&s_mux);
    uint8_t u = s_rep_usage, mods = s_rep_mods;
    bool due = u && now >= s_rep_at;
    if (due)
        s_rep_at = now + s_rate_us;
    taskEXIT_CRITICAL(&s_mux);
    if (due)
    {
        char out[16];
        int n = key_bytes(u, mods, out);
        if (n > 0)
            put(out, n);
    }
}

/* ----------------------------------------------------------- the mouse */

static int s_div_x = 8, s_div_y = 16;           /* mouse counts a cell */
static volatile int s_want_div_x = 8, s_want_div_y = 16;   /* after a speed change */
static float s_accel = 3.0f;       /* extra gain at full speed (usb.mouse_accel) */
static float s_slow = 0.7f;        /* gain of slow moves (usb.mouse_precision) */
static td_mouse_settings_t s_settings = {100, 100, 50};
static char s_mouse_name[40];      /* "USB mouse 04ca:00bd, with its wheel" */
static float s_rest_x, s_rest_y;   /* fractions of counts left over by the gain */
static int64_t s_last_us;
static int s_mx = -1, s_my;        /* the pointer, in mouse counts */
static int s_cx = -1, s_cy;        /* the cell reported last */
static bool s_slop;                /* a button pressed and the mouse not moved off yet */
static int s_px, s_py;             /* where it was pressed, in counts */
static uint8_t s_buttons;
static int s_mice;

static void mouse_send(int code, int cx, int cy, bool release)
{
    /* With the keyboard's Shift, Alt and Ctrl, as terminals report them
     * (Ctrl+wheel, Shift+click). */
    uint8_t mods = s_kbd_mods;
    code += ((mods & MOD_SHIFT) ? 4 : 0) + ((mods & MOD_ALT) ? 8 : 0) + ((mods & MOD_CTRL) ? 16 : 0);
    char out[24];
    int n = snprintf(out, sizeof(out), "\x1b[<%d;%d;%d%c", code, cx + 1, cy + 1, release ? 'm' : 'M');
    put(out, n);
}

/* Where a mouse puts its buttons, movement and wheel in its reports, from
 * its report descriptor (report protocol). */
typedef struct
{
    uint16_t pos;                  /* bit offset after the report ID */
    uint8_t size, id;              /* bits; report ID (0: none) */
    bool sign;
} field_t;

static struct
{
    bool ok, ids;                  /* fields found; reports start with an ID */
    field_t button[3], x, y, wheel;
} s_layout;

/* The fields of a mouse's report descriptor this code needs; false when X
 * and Y are not there (then boot reports are used). */
static bool parse_mouse(const uint8_t *d, size_t len)
{
    memset(&s_layout, 0, sizeof(s_layout));
    uint32_t page = 0, size = 0, count = 0, usage[16], umin = 0;
    int32_t lmin = 0;
    int nusage = 0;
    bool range = false;
    uint8_t id = 0;
    struct
    {
        uint8_t id;
        uint16_t bits;
    } pos[8] = {{0, 0}};
    int npos = 1;
    for (size_t i = 0; i < len;)
    {
        uint8_t b = d[i++];
        if (b == 0xFE)             /* a long item: skip it */
        {
            if (i >= len)
                break;
            i += 2u + d[i];
            continue;
        }
        int n = (b & 3) == 3 ? 4 : (b & 3);
        if (i + (size_t)n > len)
            break;
        uint32_t v = 0;
        for (int k = 0; k < n; k++)
            v |= (uint32_t)d[i + k] << (8 * k);
        int32_t sv = n == 1 ? (int8_t)v : n == 2 ? (int16_t)v
                                                 : (int32_t)v;
        i += (size_t)n;
        int type = (b >> 2) & 3, tag = b >> 4;
        if (type == 1)             /* global */
        {
            if (tag == 0)
                page = v;
            else if (tag == 1)
                lmin = sv;
            else if (tag == 7)
                size = v;
            else if (tag == 8)
            {
                id = (uint8_t)v;
                s_layout.ids = true;
            }
            else if (tag == 9)
                count = v;
            continue;
        }
        if (type == 2)             /* local */
        {
            if (tag == 0 && nusage < 16)
                usage[nusage++] = n == 4 ? v : (page << 16 | v);
            else if (tag == 1)
            {
                umin = n == 4 ? v : (page << 16 | v);
                range = true;
            }
            continue;
        }
        if (type == 0 && tag == 8) /* an Input item */
        {
            int p = 0;
            while (p < npos && pos[p].id != id)
                p++;
            if (p == npos)
            {
                if (npos == 8)
                    break;
                pos[npos].id = id;
                pos[npos++].bits = 0;
            }
            if (!(v & 1) && (v & 2) && size <= 32)   /* data, variable */
                for (uint32_t k = 0; k < count; k++)
                {
                    uint32_t u = range    ? umin + k
                                 : nusage ? usage[k < (uint32_t)nusage ? k : (uint32_t)nusage - 1]
                                          : 0;
                    field_t f = {.pos = (uint16_t)(pos[p].bits + k * size),
                                 .size = (uint8_t)size,
                                 .id = id,
                                 .sign = lmin < 0};
                    if (u == 0x10030)
                        s_layout.x = f;
                    else if (u == 0x10031)
                        s_layout.y = f;
                    else if (u == 0x10038)
                        s_layout.wheel = f;
                    else if (u >= 0x90001 && u <= 0x90003)
                        s_layout.button[u - 0x90001] = f;
                }
            pos[p].bits = (uint16_t)(pos[p].bits + size * count);
        }
        if (type == 0)             /* any main item ends the local items */
        {
            nusage = 0;
            range = false;
        }
    }
    s_layout.ok = s_layout.x.size && s_layout.y.size && s_layout.x.id == s_layout.y.id;
    return s_layout.ok;
}

/* A field's value from a report (0 when the report is another one). */
static int32_t field(const uint8_t *r, size_t len, const field_t *f)
{
    if (!f->size)
        return 0;
    size_t skip = 0;
    if (s_layout.ids)
    {
        if (len < 1 || r[0] != f->id)
            return 0;
        skip = 8;
    }
    if (skip + f->pos + f->size > len * 8)
        return 0;
    uint32_t v = 0;
    for (int k = 0; k < f->size; k++)
    {
        size_t bit = skip + f->pos + (size_t)k;
        if ((r[bit >> 3] >> (bit & 7)) & 1)
            v |= 1u << k;
    }
    if (f->sign && f->size < 32 && ((v >> (f->size - 1)) & 1))
        v |= ~0u << f->size;
    return (int32_t)v;
}

static void mouse_move(uint8_t b, int dx, int dy, int wheel);

static void mouse_report(const uint8_t *r, size_t len)
{
    if (!s_layout.ok)
    {
        if (len >= 3)              /* a boot report: buttons, X, Y */
            mouse_move(r[0] & 7, (int8_t)r[1], (int8_t)r[2], 0);
        return;
    }
    if (s_layout.ids && (len < 1 || r[0] != s_layout.x.id))
        return;                    /* another report of the interface (media keys, say) */
    uint8_t b = 0;
    for (int i = 0; i < 3; i++)
        if (field(r, len, &s_layout.button[i]))
            b |= (uint8_t)(1u << i);
    mouse_move(b, field(r, len, &s_layout.x), field(r, len, &s_layout.y), field(r, len, &s_layout.wheel));
}

/* b: HID button bits (left, right, middle); wheel > 0 scrolls up. */
static void mouse_move(uint8_t b, int dx, int dy, int wheel)
{
    int cols = 80, rows = 25;
    td_wm_screen_size(&cols, &rows);
    int wx = s_want_div_x, wy = s_want_div_y;
    if (wx != s_div_x || wy != s_div_y)
    {
        /* The speed changed (the Mouse app): the pointer stays in its cell,
         * now counted in the new cell size. */
        if (s_cx >= 0)
        {
            s_mx = s_px = s_cx * wx + wx / 2;
            s_my = s_py = s_cy * wy + wy / 2;
        }
        s_div_x = wx;
        s_div_y = wy;
    }
    if (s_mx < 0)
    {
        int x, y;
        if (td_wm_mouse_pos(&x, &y))
        {
            s_mx = x * s_div_x;
            s_my = y * s_div_y;
        }
        else
        {
            s_mx = cols / 2 * s_div_x;
            s_my = rows / 2 * s_div_y;
        }
    }
    /* A pointer curve, as desktop systems have: slow moves (aiming) go
     * s_slow times as far, normal ones one for one, faster ones further,
     * up to 1 + s_accel times at about 5 counts a millisecond (a flick). */
    int64_t now = esp_timer_get_time();
    float ms = (float)(now - s_last_us) / 1000.0f;
    s_last_us = now;
    if (ms <= 0.0f || ms > 50.0f)
        ms = 8.0f;                 /* the first report of a move */
    if (ms < 1.0f)
        ms = 1.0f;
    float v = sqrtf((float)(dx * dx + dy * dy)) / ms;
    float gain;
    if (v < 1.2f)
    {
        float t = (v - 0.3f) / 0.9f;
        gain = s_slow + (1.0f - s_slow) * (t < 0.0f ? 0.0f : t);
    }
    else
    {
        float t = (v - 1.2f) / 4.3f;
        gain = 1.0f + s_accel * (t > 1.0f ? 1.0f : t);
    }
    float fx = (float)dx * gain + s_rest_x, fy = (float)dy * gain + s_rest_y;
    s_rest_x = fx - (float)(int)fx;
    s_rest_y = fy - (float)(int)fy;
    s_mx += (int)fx;
    s_my += (int)fy;
    if (s_mx < 0)
        s_mx = 0;
    if (s_mx > cols * s_div_x - 1)
        s_mx = cols * s_div_x - 1;
    if (s_my < 0)
        s_my = 0;
    if (s_my > rows * s_div_y - 1)
        s_my = rows * s_div_y - 1;
    int cx = s_mx / s_div_x, cy = s_my / s_div_y;

    /* Sure clicks: a press snaps the pointer to the middle of its cell, and
     * while a button is held the pointer stays on that cell until the
     * mouse has moved a whole cell away from it. A hand that nudges the
     * mouse while clicking would otherwise move the release to the next
     * cell, and a button there would not count the click. */
    if (s_slop && (s_buttons & 7))
    {
        if (abs(s_mx - s_px) < s_div_x && abs(s_my - s_py) < s_div_y)
        {
            cx = s_cx;
            cy = s_cy;
        }
        else
        {
            s_slop = false;
        }
    }
    if ((b & 7) && !(s_buttons & 7))
    {
        s_mx = s_px = cx * s_div_x + s_div_x / 2;
        s_my = s_py = cy * s_div_y + s_div_y / 2;
        s_rest_x = s_rest_y = 0.0f;
        s_slop = true;
    }

    /* SGR button numbers: left 0, middle 1, right 2; HID bits: left, right, middle. */
    static const uint8_t bit[3] = {0x01, 0x04, 0x02};
    int held = -1;
    for (int i = 0; i < 3; i++)
        if (s_buttons & bit[i])
            held = held < 0 ? i : held;
    if (cx != s_cx || cy != s_cy)
        mouse_send(held >= 0 ? 32 + held : 35, cx, cy, false);
    s_cx = cx;
    s_cy = cy;
    for (int i = 0; i < 3; i++)
        if ((b ^ s_buttons) & bit[i])
            mouse_send(i, cx, cy, !(b & bit[i]));
    s_buttons = b;
    for (int n = 0; n < 3 && wheel; n++, wheel += wheel > 0 ? -1 : 1)
        mouse_send(wheel > 0 ? 64 : 65, cx, cy, false);   /* one per notch, at most 3 */
}

/* ------------------------------------------------------ the HID driver */

typedef struct
{
    hid_host_device_handle_t dev;
    hid_host_driver_event_t event;
} dev_event_t;

static QueueHandle_t s_events;

static void interface_cb(hid_host_device_handle_t dev, const hid_host_interface_event_t event, void *arg)
{
    (void)arg;
    hid_host_dev_params_t p;
    if (hid_host_device_get_params(dev, &p) != ESP_OK)
        return;
    switch (event)
    {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
    {
        uint8_t data[64];
        size_t len = 0;
        if (hid_host_device_get_raw_input_report_data(dev, data, sizeof(data), &len) != ESP_OK ||
            p.sub_class != HID_SUBCLASS_BOOT_INTERFACE)
            return;
        if (p.proto == HID_PROTOCOL_KEYBOARD)
            keyboard_report(data, len);
        else if (p.proto == HID_PROTOCOL_MOUSE)
            mouse_report(data, len);
        break;
    }
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "%s disconnected", p.proto == HID_PROTOCOL_MOUSE ? "mouse" : "keyboard");
        if (p.sub_class == HID_SUBCLASS_BOOT_INTERFACE && p.proto == HID_PROTOCOL_MOUSE && --s_mice <= 0)
        {
            s_mice = 0;
            td_wm_set_pointer(false);
        }
        if (p.sub_class == HID_SUBCLASS_BOOT_INTERFACE && p.proto == HID_PROTOCOL_KEYBOARD)
        {
            memset(s_keys, 0, sizeof(s_keys));
            s_rep_usage = 0;
            if (--s_kbds < 0)
                s_kbds = 0;
        }
        hid_host_device_close(dev);
        break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
        ESP_LOGW(TAG, "transfer error");
        break;
    default:
        break;
    }
}

static void device_cb(hid_host_device_handle_t dev, const hid_host_driver_event_t event, void *arg)
{
    (void)arg;
    dev_event_t e = {.dev = dev, .event = event};
    xQueueSend(s_events, &e, 0);
}

/* Opens a new interface (from hid_task: the class requests block). */
static void open_interface(hid_host_device_handle_t dev)
{
    hid_host_dev_params_t p;
    if (hid_host_device_get_params(dev, &p) != ESP_OK)
        return;
    const hid_host_device_config_t cfg = {.callback = interface_cb, .callback_arg = NULL};
    if (hid_host_device_open(dev, &cfg) != ESP_OK)
        return;
    bool boot = p.sub_class == HID_SUBCLASS_BOOT_INTERFACE;
    bool wheel = false;
    if (boot && p.proto == HID_PROTOCOL_MOUSE)
    {
        /* Its own reports (with the wheel) when its descriptor describes
         * them, boot reports otherwise. */
        size_t len = 0;
        const uint8_t *desc = hid_host_get_report_descriptor(dev, &len);
        bool own = desc && parse_mouse(desc, len);
        hid_class_request_set_protocol(dev, own ? HID_REPORT_PROTOCOL_REPORT : HID_REPORT_PROTOCOL_BOOT);
        wheel = own && s_layout.wheel.size;
        if (own)
            ESP_LOGI(TAG, "mouse reports: id %d, X %d+%d, Y %d+%d, wheel %d+%d", s_layout.x.id, s_layout.x.pos,
                     s_layout.x.size, s_layout.y.pos, s_layout.y.size, s_layout.wheel.pos, s_layout.wheel.size);
    }
    else if (boot)
    {
        hid_class_request_set_protocol(dev, HID_REPORT_PROTOCOL_BOOT);
        if (p.proto == HID_PROTOCOL_KEYBOARD)
            hid_class_request_set_idle(dev, 0, 0);
    }
    if (hid_host_device_start(dev) != ESP_OK)
    {
        hid_host_device_close(dev);
        return;
    }
    const char *what = !boot                              ? "other HID interface (not used)"
                       : p.proto == HID_PROTOCOL_KEYBOARD ? "keyboard"
                       : p.proto == HID_PROTOCOL_MOUSE    ? "mouse"
                                                          : "HID interface (not used)";
    ESP_LOGI(TAG, "%s connected (interface %d%s)", what, p.iface_num,
             !boot || p.proto != HID_PROTOCOL_MOUSE ? ""
             : wheel                                ? ", with its wheel"
             : s_layout.ok                          ? ", no wheel"
                                                    : ", boot reports");
    if (boot && p.proto == HID_PROTOCOL_KEYBOARD)
        s_kbds++;
    if (boot && p.proto == HID_PROTOCOL_MOUSE)
        snprintf(s_mouse_name, sizeof(s_mouse_name), "USB mouse, %s",
                 wheel ? "with its wheel" : "no wheel");
    if (boot && p.proto == HID_PROTOCOL_MOUSE && s_mice++ == 0)
    {
        s_mx = s_cx = -1;
        s_buttons = 0;
        s_slop = false;
        td_wm_set_pointer(true);
    }
}

static void hid_task(void *arg)
{
    (void)arg;
    for (;;)
    {
        dev_event_t e;
        if (xQueueReceive(s_events, &e, pdMS_TO_TICKS(TICK_MS)) == pdTRUE &&
            e.event == HID_HOST_DRIVER_EVENT_CONNECTED)
            open_interface(e.dev);
        keyboard_repeat();
    }
}

static void usb_lib_task(void *arg)
{
    (void)arg;
    for (;;)
    {
        uint32_t flags = 0;
        usb_host_lib_handle_events(portMAX_DELAY, &flags);
        if (flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS)
            usb_host_device_free_all();
    }
}

/* A client of its own that logs every device on the port (its IDs and
 * interface classes), HID or not, so the Log Viewer tells "nothing there"
 * (no power on the port, say) from "there, but not a keyboard or mouse". */
static usb_host_client_handle_t s_client;
static volatile uint8_t s_new_addr;

static void client_cb(const usb_host_client_event_msg_t *msg, void *arg)
{
    (void)arg;
    if (msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV)
        s_new_addr = msg->new_dev.address;
    else if (msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE)
        ESP_LOGI(TAG, "USB device removed");
}

static void log_device(uint8_t addr)
{
    usb_device_handle_t dev;
    if (usb_host_device_open(s_client, addr, &dev) != ESP_OK)
        return;
    const usb_device_desc_t *d;
    const usb_config_desc_t *cfg;
    if (usb_host_get_device_descriptor(dev, &d) == ESP_OK)
        ESP_LOGI(TAG, "USB device %04x:%04x, class %d", d->idVendor, d->idProduct, d->bDeviceClass);
    if (usb_host_get_active_config_descriptor(dev, &cfg) == ESP_OK)
    {
        int offset = 0;
        const usb_standard_desc_t *h = (const usb_standard_desc_t *)cfg;
        while ((h = usb_parse_next_descriptor_of_type(h, cfg->wTotalLength, USB_B_DESCRIPTOR_TYPE_INTERFACE,
                                                      &offset)) != NULL)
        {
            const usb_intf_desc_t *i = (const usb_intf_desc_t *)h;
            ESP_LOGI(TAG, "  interface %d: class %d, subclass %d, protocol %d", i->bInterfaceNumber,
                     i->bInterfaceClass, i->bInterfaceSubClass, i->bInterfaceProtocol);
        }
    }
    usb_host_device_close(s_client, dev);
}

static void client_task(void *arg)
{
    (void)arg;
    const usb_host_client_config_t cfg = {
        .is_synchronous = false,
        .max_num_event_msg = 5,
        .async = {.client_event_callback = client_cb, .callback_arg = NULL},
    };
    if (usb_host_client_register(&cfg, &s_client) != ESP_OK)
    {
        vTaskDelete(NULL);
        return;
    }
    for (;;)
    {
        usb_host_client_handle_events(s_client, portMAX_DELAY);
        uint8_t addr = s_new_addr;
        if (addr)
        {
            s_new_addr = 0;
            log_device(addr);
        }
    }
}

/* ------------------------------------------------------ the settings */

static void apply(const td_mouse_settings_t *s)
{
    s_settings = *s;
    int speed = s->speed < 10 ? 10 : s->speed > 1000 ? 1000
                                                     : s->speed;
    int accel = s->accel < 0 ? 0 : s->accel > 300 ? 300
                                                  : s->accel;
    int prec = s->precision < 0 ? 0 : s->precision > 100 ? 100
                                                         : s->precision;
    s_want_div_x = 800 / speed > 0 ? 800 / speed : 1;   /* mouse_move() takes them over */
    s_want_div_y = 1600 / speed > 0 ? 1600 / speed : 1;
    s_accel = 3.0f * (float)accel / 100.0f;
    s_slow = 1.0f - 0.6f * (float)prec / 100.0f;
}

static bool ops_status(char *buf, int cap)
{
    if (s_mice <= 0)
        return false;
    snprintf(buf, (size_t)cap, "%s", s_mouse_name[0] ? s_mouse_name : "USB mouse");
    return true;
}

static void ops_get(td_mouse_settings_t *s)
{
    *s = s_settings;
}

static bool ops_set(const td_mouse_settings_t *s)
{
    apply(s);
    char v[12];
    bool ok = true;
    snprintf(v, sizeof(v), "%d", s->speed);
    ok &= tdsh_board_set("usb.mouse_speed", v) == 0;
    snprintf(v, sizeof(v), "%d", s->accel);
    ok &= tdsh_board_set("usb.mouse_accel", v) == 0;
    snprintf(v, sizeof(v), "%d", s->precision);
    ok &= tdsh_board_set("usb.mouse_precision", v) == 0;
    return ok;
}

static const td_mouse_ops_t s_mouse_ops = {ops_status, ops_get, ops_set};

static void kbd_apply(const td_keyboard_settings_t *s)
{
    s_kbd_settings = *s;
    s_kbd_layout = s->layout >= 0 && s->layout < LAYOUTS ? s->layout : 0;
    int delay = s->delay_ms < 100 ? 100 : s->delay_ms > 2000 ? 2000
                                                             : s->delay_ms;
    int rate = s->rate < 1 ? 1 : s->rate > 60 ? 60
                                              : s->rate;
    s_delay_us = delay * 1000LL;
    s_rate_us = 1000000LL / rate;
}

static bool kbd_status(char *buf, int cap)
{
    if (s_kbds <= 0)
        return false;
    snprintf(buf, (size_t)cap, "USB keyboard");
    return true;
}

static void kbd_get(td_keyboard_settings_t *s)
{
    *s = s_kbd_settings;
}

static bool kbd_set(const td_keyboard_settings_t *s)
{
    kbd_apply(s);
    char v[12];
    bool ok = tdsh_board_set("usb.kbd_layout", s_layouts[s_kbd_layout].code) == 0;
    snprintf(v, sizeof(v), "%d", s->delay_ms);
    ok &= tdsh_board_set("usb.kbd_delay", v) == 0;
    snprintf(v, sizeof(v), "%d", s->rate);
    ok &= tdsh_board_set("usb.kbd_rate", v) == 0;
    return ok;
}

static const char *kbd_layout_name(int i)
{
    return i >= 0 && i < LAYOUTS ? s_layouts[i].name : NULL;
}

static const td_keyboard_ops_t s_kbd_ops = {kbd_status, kbd_get, kbd_set, kbd_layout_name};
static bool s_started;

const td_mouse_ops_t *usb_hid_mouse_ops(void)
{
    return s_started ? &s_mouse_ops : NULL;
}

const td_keyboard_ops_t *usb_hid_keyboard_ops(void)
{
    return s_started ? &s_kbd_ops : NULL;
}

void usb_hid_start(void)
{
    if (tdsh_board_int("usb.host", 0) != 1)
        return;
    const td_mouse_settings_t m = {
        .speed = tdsh_board_int("usb.mouse_speed", 100),
        .accel = tdsh_board_int("usb.mouse_accel", 100),
        .precision = tdsh_board_int("usb.mouse_precision", 50),
    };
    apply(&m);
    td_keyboard_settings_t k = {
        .layout = 0,
        .delay_ms = tdsh_board_int("usb.kbd_delay", 500),
        .rate = tdsh_board_int("usb.kbd_rate", 30),
    };
    const char *code = tdsh_board_get("usb.kbd_layout");
    for (int i = 0; code && i < LAYOUTS; i++)
        if (strcmp(code, s_layouts[i].code) == 0)
            k.layout = i;
    kbd_apply(&k);

    const usb_host_config_t host = {.skip_phy_setup = false, .intr_flags = ESP_INTR_FLAG_LEVEL1};
    esp_err_t err = usb_host_install(&host);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "USB host: %s", esp_err_to_name(err));
        return;
    }
    xTaskCreate(usb_lib_task, "usb_lib", 4096, NULL, 2, NULL);
    xTaskCreate(client_task, "usb_log", 3072, NULL, 3, NULL);
    s_events = xQueueCreate(8, sizeof(dev_event_t));
    const hid_host_driver_config_t drv = {
        .create_background_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 0,
        .callback = device_cb,
        .callback_arg = NULL,
    };
    err = hid_host_install(&drv);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "HID host: %s", esp_err_to_name(err));
        return;
    }
    xTaskCreate(hid_task, "usb_hid", 4096, NULL, 4, NULL);
    s_started = true;
    ESP_LOGI(TAG, "USB host on GPIO19/20: keyboard and mouse");
}
