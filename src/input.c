/*
 * input.c - byte-at-a-time parser for terminal keyboard and mouse input.
 *
 * States:
 *   GROUND  plain bytes (ASCII, control characters, UTF-8)
 *   ESC     after ESC; waiting to see what follows
 *   CSI     after ESC [ ; collecting parameters until a final byte
 *   SS3     after ESC O ; one more byte selects the key
 *   X10     legacy mouse: three raw bytes after ESC [ M
 *   PASTE   inside a bracketed paste, until ESC [ 2 0 1 ~
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tinydesk/td_input.h"
#include "tinydesk/td.h"

enum
{
    ST_GROUND,
    ST_ESC,
    ST_CSI,
    ST_SS3,
    ST_X10,
    ST_PASTE
};

static const char PASTE_END[] = "\x1b[201~";
#define PASTE_END_LEN 6

static td_input_t *s_paste_owner;   /* parser whose paste is being handled */

void td_input_init(td_input_t *p)
{
    free(p->paste);
    free(p->pasted);
    memset(p, 0, sizeof(*p));
    p->state = ST_GROUND;
}

const char *td_paste_text(int *len)
{
    td_input_t *p = s_paste_owner;
    if (len)
        *len = p ? p->pasted_len : 0;
    return p ? p->pasted : NULL;
}

void td_input_paste_done(td_input_t *p)
{
    free(p->pasted);
    p->pasted = NULL;
    p->pasted_len = 0;
    if (s_paste_owner == p)
        s_paste_owner = NULL;
}

static void paste_add(td_input_t *p, const char *s, int n)
{
    if (p->paste_len + n > TD_PASTE_MAX)
    {
        n = TD_PASTE_MAX - p->paste_len;
        p->paste_cut = true;
    }
    if (n <= 0)
        return;
    if (p->paste_len + n + 1 > p->paste_cap)
    {
        int cap = p->paste_cap ? p->paste_cap : 256;
        while (cap < p->paste_len + n + 1)
            cap *= 2;
        if (cap > TD_PASTE_MAX + 1)
            cap = TD_PASTE_MAX + 1;
        char *grown = realloc(p->paste, (size_t)cap);
        if (!grown)
        {
            p->paste_cut = true;
            return;
        }
        p->paste = grown;
        p->paste_cap = cap;
    }
    memcpy(p->paste + p->paste_len, s, (size_t)n);
    p->paste_len += n;
    p->paste[p->paste_len] = '\0';
}

/* The paste is complete (or stalled): hand it over as one event. */
static void paste_finish(td_input_t *p, uint32_t now)
{
    p->state = ST_GROUND;
    free(p->pasted);                 /* an older paste nobody took */
    p->pasted = p->paste;
    p->pasted_len = p->paste_len;
    p->paste = NULL;
    p->paste_len = p->paste_cap = 0;
    s_paste_owner = p;
    if (!p->pasted)
        return;
    td_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = TD_EV_PASTE;
    ev.x = p->paste_cut ? 1 : 0;
    ev.time_ms = now;
    td_event_push(&ev);
}

static void emit_resize(int rows, int cols, uint32_t now);
static int parse_params(const uint8_t *s, int len, int *out, int max);

/* What the bytes held back during a paste (in p->seq) amount to. */
enum
{
    HELD_TEXT,        /* neither of the below: plain text after all */
    HELD_PARTIAL,     /* the start of one of the below */
    HELD_END,         /* ESC [ 2 0 1 ~ */
    HELD_REPORT       /* ESC [ rows ; cols R */
};

static int paste_held(const uint8_t *h, int n)
{
    if (n <= PASTE_END_LEN && memcmp(h, PASTE_END, (size_t)n) == 0)
        return n == PASTE_END_LEN ? HELD_END : HELD_PARTIAL;
    /* A cursor position report: the reply to the periodic size query can
     * arrive in the middle of a paste. Up to 4 digits per number. */
    if (n < 2 || h[0] != 0x1B || h[1] != '[')
        return HELD_TEXT;
    int i = 2;
    for (int part = 0; part < 2; part++)
    {
        int digits = 0;
        while (i < n && h[i] >= '0' && h[i] <= '9' && digits < 4)
        {
            i++;
            digits++;
        }
        if (i == n)
            return HELD_PARTIAL;
        if (digits == 0 || h[i] != (part == 0 ? ';' : 'R'))
            return HELD_TEXT;
        i++;
    }
    return i == n ? HELD_REPORT : HELD_TEXT;
}

/* A byte inside a bracketed paste: text until ESC [ 2 0 1 ~. A cursor
 * position report inside it is taken out and handled as usual; text pasted
 * from a terminal does not contain raw escape sequences. */
static void paste_byte(td_input_t *p, uint8_t b, uint32_t now)
{
    if (p->paste_match == 0 && b != 0x1B)
    {
        char c = (char)b;
        paste_add(p, &c, 1);
        return;
    }
    p->seq[p->paste_match++] = b;
    switch (paste_held(p->seq, p->paste_match))
    {
    case HELD_PARTIAL:
        return;
    case HELD_END:
        p->paste_match = 0;
        paste_finish(p, now);
        return;
    case HELD_REPORT:
    {
        int prm[2];
        int n = parse_params(p->seq + 2, p->paste_match - 3, prm, 2);
        p->paste_match = 0;
        if (n == 2 && prm[0] > 1)
            emit_resize(prm[0], prm[1], now);
        return;
    }
    default:
        break;
    }
    /* Not a marker or report after all: the bytes held back were text, and
     * the last one is looked at again (it may be a new ESC). */
    paste_add(p, (const char *)p->seq, p->paste_match - 1);
    p->paste_match = 0;
    paste_byte(p, b, now);
}

bool td_input_pasting(const td_input_t *p)
{
    return p->state == ST_PASTE;
}

/* ---------------------------------------------------------- emitting */

static void emit_key(uint32_t key, uint8_t mods, uint32_t now)
{
    td_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = TD_EV_KEY;
    ev.key = key;
    ev.mods = mods;
    ev.time_ms = now;
    td_event_push(&ev);
}

static void emit_mouse(int b, int x, int y, bool release, uint32_t now)
{
    td_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = TD_EV_MOUSE;
    ev.time_ms = now;
    ev.x = (int16_t)(x - 1);   /* terminals report 1-based cells */
    ev.y = (int16_t)(y - 1);
    if (b & 4)
        ev.mods |= TD_MOD_SHIFT;
    if (b & 8)
        ev.mods |= TD_MOD_ALT;
    if (b & 16)
        ev.mods |= TD_MOD_CTRL;

    if (b & 64)
    {
        /* Wheel: 64 = up, 65 = down. Only presses are meaningful. */
        if (release)
            return;
        ev.button = (b & 1) ? TD_BUTTON_WHEEL_DOWN : TD_BUTTON_WHEEL_UP;
        ev.action = TD_MOUSE_PRESS;
    }
    else if (b & 32)
    {
        ev.button = (uint8_t)(b & 3);
        ev.action = (ev.button == TD_BUTTON_NONE) ? TD_MOUSE_MOVE : TD_MOUSE_DRAG;
    }
    else
    {
        ev.button = (uint8_t)(b & 3);
        ev.action = release ? TD_MOUSE_RELEASE : TD_MOUSE_PRESS;
        /* Legacy encoding reports every release as "button 3". */
        if (ev.button == TD_BUTTON_NONE && !release)
            ev.action = TD_MOUSE_RELEASE;
    }
    td_event_push(&ev);
}

static void emit_resize(int rows, int cols, uint32_t now)
{
    td_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = TD_EV_RESIZE;
    ev.x = (int16_t)cols;
    ev.y = (int16_t)rows;
    ev.time_ms = now;
    td_event_push(&ev);
}

/* A byte received in ground state (or after ESC, with alt set). */
static void ground_byte(td_input_t *p, uint8_t b, uint8_t mods, uint32_t now)
{
    switch (b)
    {
    case 0x0D:
    case 0x0A:
        emit_key(TD_KEY_ENTER, mods, now);
        return;
    case 0x09:
        emit_key(TD_KEY_TAB, mods, now);
        return;
    case 0x7F:
    case 0x08:
        emit_key(TD_KEY_BACKSPACE, mods, now);
        return;
    case 0x00:
        emit_key(' ', mods | TD_MOD_CTRL, now);
        return;
    default:
        break;
    }
    if (b >= 0x01 && b <= 0x1A)
    {
        emit_key('a' + (b - 1), mods | TD_MOD_CTRL, now);
        return;
    }
    if (b >= 0x1C && b <= 0x1F)
    {
        emit_key("\\]^_"[b - 0x1C], mods | TD_MOD_CTRL, now);
        return;
    }

    uint32_t cps[2];
    int n = td_utf8_feed(&p->utf8, b, cps);
    for (int i = 0; i < n; i++)
        emit_key(cps[i], mods, now);
}

/* ------------------------------------------------------ CSI decoding */

/* Split "1;5" style parameters. Returns the count (missing values are 0). */
static int parse_params(const uint8_t *s, int len, int *out, int max)
{
    int n = 0;
    int value = 0;
    bool have = false;
    for (int i = 0; i < len; i++)
    {
        if (s[i] >= '0' && s[i] <= '9')
        {
            value = value * 10 + (s[i] - '0');
            if (value > 9999)
                value = 9999;
            have = true;
        }
        else if (s[i] == ';')
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

/* xterm modifier parameter: 1 + (shift | alt << 1 | ctrl << 2). */
static uint8_t mods_from_param(int m)
{
    if (m < 2)
        return 0;
    m -= 1;
    uint8_t mods = 0;
    if (m & 1)
        mods |= TD_MOD_SHIFT;
    if (m & 2)
        mods |= TD_MOD_ALT;
    if (m & 4)
        mods |= TD_MOD_CTRL;
    return mods;
}

/* Keys for ESC [ n ~ */
static uint32_t tilde_key(int n)
{
    switch (n)
    {
    case 1:
    case 7:
        return TD_KEY_HOME;
    case 2:
        return TD_KEY_INSERT;
    case 3:
        return TD_KEY_DELETE;
    case 4:
    case 8:
        return TD_KEY_END;
    case 5:
        return TD_KEY_PGUP;
    case 6:
        return TD_KEY_PGDN;
    case 11:
        return TD_KEY_F1;
    case 12:
        return TD_KEY_F2;
    case 13:
        return TD_KEY_F3;
    case 14:
        return TD_KEY_F4;
    case 15:
        return TD_KEY_F5;
    case 17:
        return TD_KEY_F6;
    case 18:
        return TD_KEY_F7;
    case 19:
        return TD_KEY_F8;
    case 20:
        return TD_KEY_F9;
    case 21:
        return TD_KEY_F10;
    case 23:
        return TD_KEY_F11;
    case 24:
        return TD_KEY_F12;
    default:
        return 0;
    }
}

/* Keys selected by a final letter (CSI or SS3). */
static uint32_t letter_key(uint8_t c)
{
    switch (c)
    {
    case 'A':
        return TD_KEY_UP;
    case 'B':
        return TD_KEY_DOWN;
    case 'C':
        return TD_KEY_RIGHT;
    case 'D':
        return TD_KEY_LEFT;
    case 'H':
        return TD_KEY_HOME;
    case 'F':
        return TD_KEY_END;
    case 'P':
        return TD_KEY_F1;
    case 'Q':
        return TD_KEY_F2;
    case 'R':
        return TD_KEY_F3;
    case 'S':
        return TD_KEY_F4;
    default:
        return 0;
    }
}

/* A complete CSI sequence: p->seq holds the bytes after "ESC [", the last
 * one being the final byte. */
static void finish_csi(td_input_t *p, uint32_t now)
{
    uint8_t final = p->seq[p->seq_len - 1];
    const uint8_t *body = p->seq;
    int body_len = p->seq_len - 1;
    int prm[8];

    /* SGR mouse: ESC [ < b ; x ; y M|m */
    if (body_len > 0 && body[0] == '<' && (final == 'M' || final == 'm'))
    {
        int n = parse_params(body + 1, body_len - 1, prm, 8);
        if (n == 3)
            emit_mouse(prm[0], prm[1], prm[2], final == 'm', now);
        return;
    }
    if (body_len > 0 && (body[0] < '0' || body[0] > ';'))
        return; /* private: ignore */

    int n = parse_params(body, body_len, prm, 8);

    if (final == '~' && n > 0 && prm[0] == TD_SEQ_REDRAW)
    {
        /* tinydesk extension: whatever attached a new terminal (e.g.
         * tools/serial_bridge.py) asks for the setup and a full redraw. */
        td_full_redraw();
        return;
    }
    if (final == '~' && n > 0 && prm[0] == 200)
    {
        /* Bracketed paste starts; everything up to ESC [ 201 ~ is text. */
        p->state = ST_PASTE;
        free(p->paste);
        p->paste = NULL;
        p->paste_len = p->paste_cap = 0;
        p->paste_match = 0;
        p->paste_cut = false;
        return;
    }
    if (final == '~')
    {
        uint32_t key = tilde_key(n > 0 ? prm[0] : 0);
        if (key)
            emit_key(key, mods_from_param(n > 1 ? prm[1] : 0), now);
        return;
    }
    if (final == 'Z')
    {
        emit_key(TD_KEY_TAB, TD_MOD_SHIFT, now);
        return;
    }
    if (final == 'R' && n == 2 && prm[0] > 1)
    {
        /* Cursor position report (rows > 1 tells it apart from Shift+F3). */
        emit_resize(prm[0], prm[1], now);
        return;
    }
    uint32_t key = letter_key(final);
    if (key)
    {
        /* ESC [ 1 ; 5 A  -> Ctrl+Up */
        uint8_t mods = mods_from_param(n > 1 ? prm[1] : 0);
        emit_key(key, mods, now);
    }
    /* Anything else is silently discarded. */
}

/* ----------------------------------------------------------- feeding */

void td_input_feed(td_input_t *p, uint8_t b, uint32_t now)
{
    p->last_byte_ms = now;

    switch (p->state)
    {
    case ST_PASTE:
        paste_byte(p, b, now);
        return;

    case ST_GROUND:
        if (b == 0x1B)
        {
            p->state = ST_ESC;
            p->seq_start = now;
        }
        else
        {
            ground_byte(p, b, 0, now);
        }
        return;

    case ST_ESC:
        if (b == '[')
        {
            p->state = ST_CSI;
            p->seq_len = 0;
            p->seq_overflow = false;
        }
        else if (b == 'O')
        {
            p->state = ST_SS3;
        }
        else if (b == 0x1B)
        {
            /* ESC ESC: report the first one and stay in ESC state. */
            emit_key(TD_KEY_ESC, 0, now);
            p->seq_start = now;
        }
        else
        {
            /* ESC followed by a character is Alt + that character. */
            p->state = ST_GROUND;
            ground_byte(p, b, TD_MOD_ALT, now);
        }
        return;

    case ST_SS3:
    {
        p->state = ST_GROUND;
        uint32_t key = letter_key(b);
        if (key)
            emit_key(key, 0, now);
        return;
    }

    case ST_CSI:
        if (p->seq_len == 0 && b == 'M')
        {
            /* Legacy X10 mouse: three raw bytes follow. */
            p->state = ST_X10;
            p->x10_left = 3;
            return;
        }
        if (b == 0x1B)
        {
            /* A new sequence started before this one ended: drop it. */
            p->state = ST_ESC;
            p->seq_start = now;
            return;
        }
        if (b < 0x20)
            return;              /* stray control byte: ignore */
        if (p->seq_len < sizeof(p->seq))
            p->seq[p->seq_len++] = b;
        else
            p->seq_overflow = true;
        if (b >= 0x40 && b <= 0x7E)
        {      /* final byte */
            p->state = ST_GROUND;          /* before: a paste start changes it */
            if (!p->seq_overflow)
                finish_csi(p, now);
        }
        return;

    case ST_X10:
        p->seq[3 - p->x10_left] = b;
        if (--p->x10_left == 0)
        {
            int cb = p->seq[0] - 32;
            emit_mouse(cb, p->seq[1] - 32, p->seq[2] - 32, (cb & 3) == 3 && !(cb & 64), now);
            p->state = ST_GROUND;
        }
        return;

    default:
        p->state = ST_GROUND;
        return;
    }
}

void td_input_poll_timeouts(td_input_t *p, uint32_t now)
{
    if (p->state == ST_PASTE)
    {
        if (now - p->last_byte_ms >= TD_PASTE_TIMEOUT_MS)
        {
            /* Bytes held back as a possible sequence were text. */
            paste_add(p, (const char *)p->seq, p->paste_match);
            p->paste_match = 0;
            paste_finish(p, now);
        }
        return;
    }
    uint32_t age = now - p->seq_start;
    if (p->state == ST_ESC && age >= TD_ESC_TIMEOUT_MS)
    {
        p->state = ST_GROUND;
        emit_key(TD_KEY_ESC, 0, now);
    }
    else if ((p->state == ST_CSI || p->state == ST_SS3 || p->state == ST_X10) &&
             age >= TD_SEQ_TIMEOUT_MS)
    {
        p->state = ST_GROUND;   /* stalled sequence: discard */
    }
}

/* ------------------------------------------------------------ naming */

const char *td_key_name(const td_event_t *ev, char *buf, int cap)
{
    static const char *names[] = {
        "Enter",
        "Tab",
        "Backspace",
        "Esc",
        "Up",
        "Down",
        "Right",
        "Left",
        "Home",
        "End",
        "Insert",
        "Delete",
        "PgUp",
        "PgDn",
        "F1",
        "F2",
        "F3",
        "F4",
        "F5",
        "F6",
        "F7",
        "F8",
        "F9",
        "F10",
        "F11",
        "F12",
    };
    char key[16];
    if (ev->key > TD_KEY_BASE && ev->key <= TD_KEY_F12)
    {
        snprintf(key, sizeof(key), "%s", names[ev->key - TD_KEY_BASE - 1]);
    }
    else if (ev->key == ' ')
    {
        snprintf(key, sizeof(key), "Space");
    }
    else
    {
        uint8_t u[4];
        int n = td_utf8_encode(ev->key, u);
        memcpy(key, u, (size_t)n);
        key[n] = '\0';
    }
    snprintf(buf, (size_t)cap, "%s%s%s%s",
             (ev->mods & TD_MOD_CTRL) ? "Ctrl+" : "",
             (ev->mods & TD_MOD_ALT) ? "Alt+" : "",
             (ev->mods & TD_MOD_SHIFT) ? "Shift+" : "", key);
    return buf;
}
