/*
 * editor.c - a small text editor window, with extras for Python.
 *
 *   arrows, Home/End, PgUp/PgDn, Ctrl+Home/End,      move (hold Shift to
 *   Ctrl+Left/Right (by word)                          select)
 *   mouse: click places the cursor, drag or Shift+click selects,
 *          double-click selects a word, the wheel scrolls, right-click menu
 *   typing, Enter (keeps the indent), Tab / Shift+Tab   edit (replace or
 *   (indent the selected lines), Backspace, Delete      delete a selection)
 *   Ctrl+A select all, Ctrl+C copy, Ctrl+X cut, Ctrl+V paste
 *   (also Ctrl+Insert, Shift+Delete, Shift+Insert)
 *   Ctrl+Z undo, Ctrl+Y redo
 *   Ctrl+F find, F3 find again, Ctrl+R replace all, Ctrl+G go to a line,
 *   Ctrl+/ comment or uncomment the lines (#)
 *   Ctrl+S save, Ctrl+W or Esc close (asks about unsaved changes)
 *
 * Python files (.py) also get colours, line numbers, an indent after a
 * line ending in ':', and the matching bracket shown. F5 saves and runs
 * the file in an output pane at the bottom, which shows the Terminal's
 * shell session: F7 moves the keyboard there (input(), Ctrl+C) and back.
 * When the script stops with an error, its line is marked and F8 goes
 * there. Ctrl+Space suggests names (keywords, built-ins, module names, the
 * file's own words); they can also pop up while typing (right-click menu,
 * saved with the user's settings). Shell scripts (.tdsh) run with F5 too.
 *
 * The text lives in one flat buffer that is allocated while the window is
 * open, with the undo history next to it. It is sized to the file and grows
 * with the text, up to TD_EDITOR_MAX bytes (so a small script leaves RAM
 * for running it on a board without PSRAM). Files larger than that open
 * read-only. Line endings are stored as LF; CR characters are dropped when
 * loading.
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "editor_py.h"
#include "td_apps.h"

#ifndef TD_EDITOR_MAX
#define TD_EDITOR_MAX 16384
#endif
#ifndef TD_EDITOR_UNDO
#define TD_EDITOR_UNDO 6144      /* bytes of undo text kept */
#endif
#ifndef TD_EDITOR_UNDO_OPS
#define TD_EDITOR_UNDO_OPS 128   /* undo records kept, 20 bytes each */
#endif
#if TD_EDITOR_UNDO_OPS < 1
#error "TD_EDITOR_UNDO_OPS must be at least 1"
#endif
#define PATH_LEN        TD_PATH_MAX
#define DOUBLE_CLICK_MS 400
#define OUT_CAP         2048   /* output kept to find an error in */
#define COMP_MAX        24     /* suggestions */
#define COMP_ROWS       6      /* of them shown at once */
#define CLS_MAX         256    /* bytes of a line that get colours */
#define GROW            1024   /* the buffer grows by at least this much */

/* One recorded change. Undo walks back one group (a user action: a typed
 * word, a paste, a cut...) at a time. */
typedef struct
{
    int pos, len;        /* where, how many bytes */
    int text;            /* offset of the bytes in the pool */
    int cur;             /* cursor before the action */
    uint16_t group;
    bool ins;            /* insertion (else deletion) */
} undo_op_t;

typedef enum
{
    K_OTHER,
    K_TYPE,
    K_BACKSPACE,
    K_DELETE
} edit_kind_t;

typedef struct
{
    undo_op_t ops[TD_EDITOR_UNDO_OPS];
    int n;               /* ops recorded */
    int top;             /* ops applied; ops[top..n) can be redone */
    char pool[TD_EDITOR_UNDO];
    int used;
    uint16_t group;
    edit_kind_t kind;    /* kind of the last action, for merging */
    bool merge;          /* the next edit may join the last group */
    bool after_space;    /* last typed char was a space: a new word starts */
    int saved;           /* top when the file was last saved; -1: unknown */
    uint16_t broken;     /* group that did not fit: record none of it */
} undo_t;

typedef struct
{
    char *buf;          /* cap + 1 bytes of text, then CLS_MAX of colours */
    int len;
    int cap;
    int cur;            /* cursor as a byte offset */
    int anchor;         /* other end of the selection, -1 for none */
    int top;            /* first visible line */
    int left;           /* first visible column */
    int goal_col;       /* column kept while moving up and down */
    bool follow;        /* scroll to the cursor on the next draw */
    bool modified;
    bool readonly;
    bool close_after_save;
    bool mouse_down;
    uint32_t click_ms;
    int click_pos;
    char path[PATH_LEN];     /* "" for a new, unnamed file */
    char status[48];
    uint32_t status_ms;
    int view_rows;      /* text rows in the window at the last draw */
    undo_t *undo;

    bool python;        /* a .py file: colours, indent rules, completion */
    bool script;        /* a .tdsh file: F5 runs it */
    bool numbers;       /* line numbers */
    int gutter;         /* their width at the last draw */
    char find[TD_TEXT_MAX];

    /* Output pane: the Terminal's session, below the text. */
    bool pane;
    bool pane_focus;    /* keys go to the program */
    bool pane_drag;     /* resizing it with the mouse */
    int pane_h;         /* rows; 0: a third of the window */
    int pane_y;         /* row of its title bar at the last draw */
    int pane_scroll;
    int rows_all;       /* rows above the status bar at the last draw */
    bool run_after_save;
    char *out;          /* output since Run, escapes removed (OUT_CAP) */
    int out_len;
    uint8_t out_esc;    /* inside an escape sequence */
    bool err_done;      /* the error of this run was found */

    /* The error the last run stopped with. */
    int err_line;       /* 1-based; 0: none */
    bool err_here;      /* in this file */
    char err_msg[64];
    char err_file[PATH_LEN];   /* as the shell names it */

    /* Completion popup. */
    bool comp_open;
    int comp_n, comp_sel, comp_top, comp_start;
    pyed_word_t comp[COMP_MAX];
} editor_t;

static editor_t E;
static td_window_t *s_win;

static void update_mode(void);
static void run_file(void);

static const td_fs_ops_t *fs(void)
{
    return td_sysinfo()->fs;
}

static void set_status(const char *msg)
{
    snprintf(E.status, sizeof(E.status), "%s", msg);
    E.status_ms = td_millis();
    td_wm_invalidate();
}

static const char *file_name(void)
{
    if (!E.path[0])
        return "untitled";
    const char *slash = strrchr(E.path, '/');
    return slash ? slash + 1 : E.path;
}

static void update_title(void)
{
    char title[TD_TITLE_MAX], name[TD_TITLE_MAX - 10];
    /* "*" + name + " - Editor": the name is cut to fit the title, at a
     * character boundary. */
    td_utf8_copy(name, sizeof(name), file_name(), INT_MAX);
    snprintf(title, sizeof(title), "%s%s - Editor", E.modified ? "*" : "", name);
    td_win_set_title(s_win, title);
}

/* ------------------------------------------------------------ clipboard */

static char *s_clip;
static int s_clip_len;

bool td_clipboard_set(const char *text, int len)
{
    char *copy = malloc((size_t)len + 1);
    if (!copy)
        return false;
    memcpy(copy, text, (size_t)len);
    copy[len] = '\0';
    free(s_clip);
    s_clip = copy;
    s_clip_len = len;
    return true;
}

const char *td_clipboard_get(int *len)
{
    if (len)
        *len = s_clip ? s_clip_len : 0;
    return s_clip;
}

void td_clipboard_clear(void)
{
    free(s_clip);
    s_clip = NULL;
    s_clip_len = 0;
}

/* ------------------------------------------------ positions in the text */

static bool is_cont(char c)
{
    return ((unsigned char)c & 0xC0u) == 0x80u;
}

static int next_pos(int p)
{
    if (p >= E.len)
        return E.len;
    p++;
    while (p < E.len && is_cont(E.buf[p]))
        p++;
    return p;
}

static int prev_pos(int p)
{
    if (p <= 0)
        return 0;
    p--;
    while (p > 0 && is_cont(E.buf[p]))
        p--;
    return p;
}

static int line_start(int p)
{
    while (p > 0 && E.buf[p - 1] != '\n')
        p--;
    return p;
}

static int line_end(int p)
{
    while (p < E.len && E.buf[p] != '\n')
        p++;
    return p;
}

/* Column (in characters) of position p within its line. */
static int col_of(int p)
{
    int c = 0;
    for (int i = line_start(p); i < p; i++)
        if (!is_cont(E.buf[i]))
            c++;
    return c;
}

static int line_of(int p)
{
    int n = 0;
    for (int i = 0; i < p; i++)
        if (E.buf[i] == '\n')
            n++;
    return n;
}

static int line_count(void)
{
    return line_of(E.len) + 1;
}

/* Start of line n (clamped to the last line). */
static int nth_line(int n)
{
    int p = 0;
    for (int line = 0; line < n; line++)
    {
        int e = line_end(p);
        if (e >= E.len)
            break;
        p = e + 1;
    }
    return p;
}

/* Position at column col of the line starting at ls (or its end). */
static int at_col(int ls, int col)
{
    int p = ls, e = line_end(ls);
    for (int c = 0; c < col && p < e; c++)
        p = next_pos(p);
    return p;
}

/* Letters, digits, '_' and anything non-ASCII count as word characters. */
static bool word_char(char c)
{
    unsigned char u = (unsigned char)c;
    return u >= 0x80 || u == '_' || (u >= '0' && u <= '9') || ((u | 0x20) >= 'a' && (u | 0x20) <= 'z');
}

static int word_left(int p)
{
    while (p > 0 && !word_char(E.buf[p - 1]))
        p--;
    while (p > 0 && word_char(E.buf[p - 1]))
        p--;
    return p;
}

static int word_right(int p)
{
    while (p < E.len && word_char(E.buf[p]))
        p++;
    while (p < E.len && !word_char(E.buf[p]))
        p++;
    return p;
}

/* ---------------------------------------------------------- selection */

static bool has_sel(void)
{
    return E.anchor >= 0 && E.anchor != E.cur;
}

static void sel_range(int *a, int *b)
{
    *a = E.anchor < E.cur ? E.anchor : E.cur;
    *b = E.anchor < E.cur ? E.cur : E.anchor;
}

/* Number of characters selected. */
static int sel_chars(void)
{
    int a, b, n = 0;
    sel_range(&a, &b);
    for (int i = a; i < b; i++)
        if (!is_cont(E.buf[i]))
            n++;
    return n;
}

/* --------------------------------------------------- raw buffer edits */

static void raw_insert(int pos, const char *text, int n)
{
    memmove(E.buf + pos + n, E.buf + pos, (size_t)(E.len - pos));
    memcpy(E.buf + pos, text, (size_t)n);
    E.len += n;
    E.buf[E.len] = '\0';   /* keeps UTF-8 decoding in bounds */
}

static void raw_erase(int from, int to)
{
    memmove(E.buf + from, E.buf + to, (size_t)(E.len - to));
    E.len -= to - from;
    E.buf[E.len] = '\0';
}

static void after_change(void)
{
    E.modified = !E.undo || E.undo->top != E.undo->saved;
    update_title();
    E.follow = true;
    td_wm_invalidate();
}

/* ------------------------------------------------------------ undo */

/* Forget the oldest action to make room. */
static void undo_drop_oldest(undo_t *u)
{
    int k = 0;
    while (k < u->n && u->ops[k].group == u->ops[0].group)
        k++;
    int bytes = k < u->n ? u->ops[k].text : u->used;
    memmove(u->pool, u->pool + bytes, (size_t)(u->used - bytes));
    u->used -= bytes;
    memmove(u->ops, u->ops + k, (size_t)(u->n - k) * sizeof(undo_op_t));
    u->n -= k;
    u->top = u->top > k ? u->top - k : 0;
    for (int i = 0; i < u->n; i++)
        u->ops[i].text -= bytes;
    u->saved = u->saved >= k ? u->saved - k : -1;
}

/* Start a user action. Typing, Backspace and Delete runs join the previous
 * action (typing one word at a time); anything else starts a new one. */
static void undo_begin(edit_kind_t kind, bool space)
{
    undo_t *u = E.undo;
    if (!u)
        return;
    bool join = u->merge && kind != K_OTHER && kind == u->kind && u->top == u->n && u->n > 0;
    if (join && kind == K_TYPE && u->after_space && !space)
        join = false;   /* next word */
    if (!join)
        u->group++;
    u->kind = kind;
    u->after_space = space;
    u->merge = kind != K_OTHER;
}

static void undo_record(bool ins, int pos, const char *text, int len, int cur_before)
{
    undo_t *u = E.undo;
    if (!u || len <= 0 || u->broken == u->group)
        return;
    if (u->top < u->n)
    {                      /* a new edit ends the redo chain */
        u->used = u->ops[u->top].text;
        u->n = u->top;
        if (u->saved > u->top)
            u->saved = -1;
    }
    if (len > TD_EDITOR_UNDO)
    {               /* too big to keep: start over */
        u->n = u->top = u->used = 0;
        u->saved = -1;
        u->broken = u->group;
        return;
    }

    /* Extend the last op when this continues it. */
    undo_op_t *last = u->n > 0 ? &u->ops[u->n - 1] : NULL;
    if (last && last->group == u->group && last->ins == ins && last->text + last->len == u->used &&
        u->used + len <= TD_EDITOR_UNDO)
    {
        if (ins && last->pos + last->len == pos)
        {                       /* typing on */
            memcpy(u->pool + u->used, text, (size_t)len);
        }
        else if (!ins && pos + len == last->pos)
        {                     /* Backspace */
            memmove(u->pool + last->text + len, u->pool + last->text, (size_t)last->len);
            memcpy(u->pool + last->text, text, (size_t)len);
            last->pos = pos;
        }
        else if (!ins && pos == last->pos)
        {                           /* Delete */
            memcpy(u->pool + u->used, text, (size_t)len);
        }
        else
        {
            last = NULL;
        }
        if (last)
        {
            last->len += len;
            u->used += len;
            return;
        }
    }

    while (u->n > 0 && (u->n >= TD_EDITOR_UNDO_OPS || u->used + len > TD_EDITOR_UNDO))
    {
        if (u->ops[0].group == u->group)
        {    /* only this action is left */
            u->n = u->top = u->used = 0;
            u->saved = -1;
            u->broken = u->group;
            return;
        }
        undo_drop_oldest(u);
    }
    undo_op_t *op = &u->ops[u->n++];
    op->pos = pos;
    op->len = len;
    op->text = u->used;
    op->cur = cur_before;
    op->group = u->group;
    op->ins = ins;
    memcpy(u->pool + u->used, text, (size_t)len);
    u->used += len;
    u->top = u->n;
}

static void undo(void)
{
    undo_t *u = E.undo;
    if (!u || u->top == 0)
    {
        set_status("Nothing to undo");
        return;
    }
    uint16_t g = u->ops[u->top - 1].group;
    while (u->top > 0 && u->ops[u->top - 1].group == g)
    {
        const undo_op_t *op = &u->ops[--u->top];
        if (op->ins)
            raw_erase(op->pos, op->pos + op->len);
        else
            raw_insert(op->pos, u->pool + op->text, op->len);
        E.cur = op->cur;
    }
    u->merge = false;
    E.anchor = -1;
    after_change();
}

static void redo(void)
{
    undo_t *u = E.undo;
    if (!u || u->top == u->n)
    {
        set_status("Nothing to redo");
        return;
    }
    uint16_t g = u->ops[u->top].group;
    while (u->top < u->n && u->ops[u->top].group == g)
    {
        const undo_op_t *op = &u->ops[u->top++];
        if (op->ins)
        {
            raw_insert(op->pos, u->pool + op->text, op->len);
            E.cur = op->pos + op->len;
        }
        else
        {
            raw_erase(op->pos, op->pos + op->len);
            E.cur = op->pos;
        }
    }
    u->merge = false;
    E.anchor = -1;
    after_change();
}

/* ---------------------------------------------------------- editing */

static bool writable(void)
{
    if (E.readonly)
        set_status("Read-only: the file is too large to edit");
    return !E.readonly;
}

/* Delete [from, to) as part of the current action. */
static void erase_recorded(int from, int to)
{
    if (from >= to)
        return;
    undo_record(false, from, E.buf + from, to - from, E.cur);
    raw_erase(from, to);
    E.cur = from;
}

/* Delete the selection (if any) as part of the current action. */
static void erase_selection(void)
{
    if (!has_sel())
        return;
    int a, b;
    sel_range(&a, &b);
    erase_recorded(a, b);
    E.anchor = -1;
}

/* Room for `extra` more bytes of text, growing the buffer (up to
 * TD_EDITOR_MAX); false, with a note, when there is none. */
static bool room_for(int extra)
{
    int need = E.len + extra;
    if (need > TD_EDITOR_MAX)
    {
        set_status("The file is full");
        return false;
    }
    if (need <= E.cap)
        return true;
    int cap = need + GROW < TD_EDITOR_MAX ? need + GROW : TD_EDITOR_MAX;
    char *b = realloc(E.buf, (size_t)cap + 1 + CLS_MAX);
    if (!b)
    {
        set_status("Not enough memory");
        return false;
    }
    E.buf = b;
    E.cap = cap;
    return true;
}

/* Type or paste text, replacing the selection. */
static void insert_text(const char *text, int n, edit_kind_t kind)
{
    if (!writable() || n <= 0)
        return;
    int a = E.cur, b = E.cur;
    if (has_sel())
        sel_range(&a, &b);
    if (!room_for(n - (b - a)))
        return;
    bool space = n == 1 && (text[0] == ' ' || text[0] == '\n');
    bool replace = has_sel();
    undo_begin(replace ? K_OTHER : kind, space);
    if (replace && kind == K_TYPE && E.undo)
    {    /* typing on joins this step */
        E.undo->kind = K_TYPE;
        E.undo->merge = true;
    }
    erase_selection();
    undo_record(true, E.cur, text, n, E.cur);
    raw_insert(E.cur, text, n);
    E.cur += n;
    E.anchor = -1;
    after_change();
}

/* Backspace / Delete: the selection, or one character. */
static void delete_key(bool backwards)
{
    if (!writable())
        return;
    if (has_sel())
    {
        undo_begin(K_OTHER, false);
        erase_selection();
    }
    else
    {
        int from = backwards ? prev_pos(E.cur) : E.cur;
        int to = backwards ? E.cur : next_pos(E.cur);
        if (from >= to)
            return;
        undo_begin(backwards ? K_BACKSPACE : K_DELETE, false);
        erase_recorded(from, to);
    }
    after_change();
}

static void copy_selection(void)
{
    if (!has_sel())
    {
        set_status("Nothing selected");
        return;
    }
    int a, b;
    sel_range(&a, &b);
    if (td_clipboard_set(E.buf + a, b - a))
    {
        td_host_clipboard_set(E.buf + a, b - a);    /* the PC's clipboard, where the terminal allows */
        char msg[32];
        snprintf(msg, sizeof(msg), "Copied %d characters", sel_chars());
        set_status(msg);
    }
    else
    {
        set_status("Not enough memory to copy");
    }
}

static void cut_selection(void)
{
    if (!has_sel() || !writable())
    {
        if (!has_sel())
            set_status("Nothing selected");
        return;
    }
    copy_selection();
    undo_begin(K_OTHER, false);
    erase_selection();
    after_change();
}

static void paste(void)
{
    int n = 0;
    const char *text = td_clipboard_get(&n);
    if (!text || n == 0)
    {
        set_status("The clipboard is empty");
        return;
    }
    insert_text(text, n, K_OTHER);
}

/* Text pasted in the terminal (from the PC's clipboard): insert it as
 * one step, with CR / CRLF line ends turned into LF. */
static void paste_from_terminal(const td_event_t *ev)
{
    int n = 0;
    const char *text = td_paste_text(&n);
    if (!text || n <= 0)
        return;
    char *buf = malloc((size_t)n);
    if (!buf)
    {
        set_status("Not enough memory to paste");
        return;
    }
    int m = 0;
    for (int i = 0; i < n; i++)
    {
        if (text[i] == '\r')
        {
            buf[m++] = '\n';
            if (i + 1 < n && text[i + 1] == '\n')
                i++;
        }
        else if (text[i] != '\0')
        {
            buf[m++] = text[i];
        }
    }
    insert_text(buf, m, K_OTHER);
    td_clipboard_set(buf, m);                  /* Ctrl+V pastes it again */
    free(buf);
    char msg[48];
    snprintf(msg, sizeof(msg), "Pasted %d characters%s", m, ev->x ? " (cut: too long)" : "");
    set_status(msg);
}

static void select_all(void)
{
    E.anchor = 0;
    E.cur = E.len;
    E.follow = true;
    td_wm_invalidate();
}

static void move_lines(int delta)
{
    int line = line_of(E.cur) + delta;
    if (line < 0)
        line = 0;
    if (line >= line_count())
        line = line_count() - 1;
    E.cur = at_col(nth_line(line), E.goal_col);
}


/* ------------------------------------------------- lines and indents */

static bool blank(char c)
{
    return c == ' ' || c == '\t';
}

/* Starts of the first and last line of the selection (or the cursor's
 * line); a selection that ends at the start of a line leaves it out. */
static void line_range(int *first, int *last)
{
    int a = E.cur, b = E.cur;
    if (has_sel())
    {
        sel_range(&a, &b);
        if (b > a && b == line_start(b))
            b--;
    }
    *first = line_start(a);
    *last = line_start(b);
}

/* Change each line of line_range() with fn, the last one first, as one
 * undo step. fn returns how much the line grew (negative: shrank). Selected
 * lines stay selected. */
static void edit_lines(int (*fn)(int ls, void *arg), void *arg)
{
    if (!writable())
        return;
    int first, last;
    line_range(&first, &last);
    int l0 = line_of(first), l1 = line_of(last);
    bool sel = has_sel();
    int cur_ls = line_start(E.cur), cur_off = E.cur - cur_ls, cur_d = 0;
    undo_begin(K_OTHER, false);
    for (int ls = last;; ls = line_start(ls - 1))
    {
        int d = fn(ls, arg);
        if (ls == cur_ls)
            cur_d = d;
        if (ls <= first)
            break;
    }
    if (sel)
    {
        E.anchor = nth_line(l0);
        E.cur = line_end(nth_line(l1));
    }
    else
    {
        int ls = nth_line(l0), off = cur_off + cur_d;
        E.cur = ls + (off < 0 ? 0 : off);
        if (E.cur > line_end(ls))
            E.cur = line_end(ls);
    }
    after_change();
}

/* arg: +1 indents by 4 spaces, -1 takes up to 4 (or a tab) away. */
static int indent_line(int ls, void *arg)
{
    if (*(const int *)arg > 0)
    {
        if (ls >= E.len || E.buf[ls] == '\n' || !room_for(4))
            return 0;
        undo_record(true, ls, "    ", 4, E.cur);
        raw_insert(ls, "    ", 4);
        return 4;
    }
    int k = 0;
    while (k < 4 && ls + k < E.len && E.buf[ls + k] == ' ')
        k++;
    if (k == 0 && ls < E.len && E.buf[ls] == '\t')
        k = 1;
    if (k)
    {
        undo_record(false, ls, E.buf + ls, k, E.cur);
        raw_erase(ls, ls + k);
    }
    return -k;
}

typedef struct
{
    bool remove;
    int indent;    /* "# " goes after this many bytes */
} comment_t;

static int comment_line(int ls, void *arg)
{
    const comment_t *c = arg;
    int e = line_end(ls), i = ls;
    while (i < e && blank(E.buf[i]))
        i++;
    if (i == e)
        return 0;   /* blank lines stay as they are */
    if (c->remove)
    {
        int k = E.buf[i] != '#' ? 0 : i + 1 < e && E.buf[i + 1] == ' ' ? 2
                                                                       : 1;
        if (k)
        {
            undo_record(false, i, E.buf + i, k, E.cur);
            raw_erase(i, i + k);
        }
        return -k;
    }
    if (!room_for(2))
        return 0;
    undo_record(true, ls + c->indent, "# ", 2, E.cur);
    raw_insert(ls + c->indent, "# ", 2);
    return 2;
}

/* Ctrl+/: uncomment the lines when all of them are comments, else comment
 * them out at the smallest indent. */
static void toggle_comment(void)
{
    comment_t c = {true, INT_MAX};
    int first, last;
    line_range(&first, &last);
    for (int ls = first;; ls = line_end(ls) + 1)
    {
        int e = line_end(ls), i = ls;
        while (i < e && blank(E.buf[i]))
            i++;
        if (i < e)
        {
            if (E.buf[i] != '#')
                c.remove = false;
            if (i - ls < c.indent)
                c.indent = i - ls;
        }
        if (ls >= last || e >= E.len)
            break;
    }
    if (c.indent == INT_MAX)
        return;   /* only blank lines */
    edit_lines(comment_line, &c);
}

static bool starts_word(int p, int end, const char *word)
{
    int n = (int)strlen(word);
    return end - p >= n && memcmp(E.buf + p, word, (size_t)n) == 0 && (p + n == end || !word_char(E.buf[p + n]));
}

/* Enter keeps the line's indent; in Python one more after a ':' and one
 * less after return, pass, break, continue or raise. */
static void newline(void)
{
    char text[80];
    int n = 0, ls = line_start(E.cur), i = ls;
    text[n++] = '\n';
    while (i < E.cur && blank(E.buf[i]) && n < 72)
        text[n++] = E.buf[i++];
    if (E.python)
    {
        int j = E.cur;
        while (j > i && blank(E.buf[j - 1]))
            j--;
        if (j > i && E.buf[j - 1] == ':')
        {
            memcpy(text + n, "    ", 4);
            n += 4;
        }
        else if (starts_word(i, j, "return") || starts_word(i, j, "pass") || starts_word(i, j, "break") ||
                 starts_word(i, j, "continue") || starts_word(i, j, "raise"))
        {
            for (int k = 0; k < 4 && n > 1 && text[n - 1] == ' '; k++)
                n--;
        }
    }
    insert_text(text, n, K_TYPE);
}

/* Tab: indent the selected lines, or spaces to the next multiple of 4. */
static void tab_key(bool shift)
{
    static const int in = 1, out = -1;
    int a = 0, b = 0;
    if (has_sel())
        sel_range(&a, &b);
    if (shift)
        edit_lines(indent_line, (void *)&out);
    else if (has_sel() && memchr(E.buf + a, '\n', (size_t)(b - a)))
        edit_lines(indent_line, (void *)&in);
    else
        insert_text("    ", 4 - col_of(has_sel() ? a : E.cur) % 4, K_TYPE);
}

/* Backspace in a Python line's indent takes it back to a multiple of 4. */
static void backspace(void)
{
    if (E.python && !has_sel() && !E.readonly)
    {
        int ls = line_start(E.cur), i = ls;
        while (i < E.cur && E.buf[i] == ' ')
            i++;
        if (i == E.cur && E.cur > ls)
        {
            int k = (E.cur - ls - 1) % 4 + 1;
            undo_begin(K_BACKSPACE, false);
            erase_recorded(E.cur - k, E.cur);
            after_change();
            return;
        }
    }
    delete_key(true);
}

/* ------------------------------------------------------ find & go to */

static int find_at(const char *needle, int from, int to)
{
    int n = (int)strlen(needle);
    for (int i = from; i + n <= E.len && i < to; i++)
        if (memcmp(E.buf + i, needle, (size_t)n) == 0)
            return i;
    return -1;
}

static void select_range(int a, int b)
{
    E.anchor = a;
    E.cur = b;
    E.goal_col = col_of(E.cur);
    if (E.undo)
        E.undo->merge = false;
    E.follow = true;
    td_wm_invalidate();
}

static void find_box(void);

/* F3: the next match after the cursor, starting again at the top. */
static void find_next(void)
{
    if (!E.find[0])
    {
        find_box();
        return;
    }
    int from = E.cur, p = find_at(E.find, from, E.len);
    if (p < 0)
    {
        p = find_at(E.find, 0, from);
        if (p < 0)
        {
            set_status("Not found");
            return;
        }
        set_status("From the top");
    }
    select_range(p, p + (int)strlen(E.find));
}

static void find_answer(const char *text, void *user)
{
    (void)user;
    if (!td_win_is_open(s_win) || !text[0])
        return;
    snprintf(E.find, sizeof(E.find), "%s", text);
    if (has_sel())
    {     /* search from the start of the selection: it may be the match */
        int a, b;
        sel_range(&a, &b);
        E.cur = a;
    }
    find_next();
}

/* The selection (one short line) is the suggested text. */
static void find_initial(char *out, size_t cap)
{
    snprintf(out, cap, "%s", E.find);
    if (has_sel())
    {
        int a, b;
        sel_range(&a, &b);
        if (b - a < (int)cap && !memchr(E.buf + a, '\n', (size_t)(b - a)))
        {
            memcpy(out, E.buf + a, (size_t)(b - a));
            out[b - a] = '\0';
        }
    }
}

static void find_box(void)
{
    char init[TD_TEXT_MAX];
    find_initial(init, sizeof(init));
    td_inputbox("Find", "Find:", init, find_answer, NULL);
}

static void replace_with(const char *with, void *user)
{
    (void)user;
    if (!td_win_is_open(s_win) || !writable() || !E.find[0])
        return;
    int fn = (int)strlen(E.find), wn = (int)strlen(with), count = 0, p = 0;
    bool full = false;
    undo_begin(K_OTHER, false);
    while ((p = find_at(E.find, p, E.len)) >= 0)
    {
        if (!room_for(wn - fn))
        {
            full = true;
            break;
        }
        undo_record(false, p, E.buf + p, fn, E.cur);
        raw_erase(p, p + fn);
        undo_record(true, p, with, wn, E.cur);
        raw_insert(p, with, wn);
        if (E.cur > p)
            E.cur = E.cur >= p + fn ? E.cur - fn + wn : p;
        p += wn;
        count++;
    }
    E.anchor = -1;
    if (count)
        after_change();
    char msg[48];
    if (count)
        snprintf(msg, sizeof(msg), "Replaced %d%s", count, full ? " (the file is full)" : "");
    else
        snprintf(msg, sizeof(msg), "%s", full ? "The file is full" : "Not found");
    set_status(msg);
}

static void replace_answer(const char *text, void *user)
{
    (void)user;
    if (!td_win_is_open(s_win) || !text[0])
        return;
    snprintf(E.find, sizeof(E.find), "%s", text);
    td_inputbox("Replace all", "Replace with:", "", replace_with, NULL);
}

static void replace_box(void)
{
    if (!writable())
        return;
    char init[TD_TEXT_MAX];
    find_initial(init, sizeof(init));
    td_inputbox("Replace all", "Find:", init, replace_answer, NULL);
}

/* The cursor to the first character (after the indent) of line n. */
static void go_to_line(int n)
{
    if (n < 1)
        n = 1;
    if (n > line_count())
        n = line_count();
    int p = nth_line(n - 1), e = line_end(p);
    while (p < e && blank(E.buf[p]))
        p++;
    E.anchor = -1;
    E.cur = p;
    E.goal_col = col_of(p);
    E.follow = true;
    E.pane_focus = false;
    if (E.undo)
        E.undo->merge = false;
    td_wm_invalidate();
}

static void line_answer(const char *text, void *user)
{
    (void)user;
    if (td_win_is_open(s_win) && text[0])
        go_to_line(atoi(text));
}

static void line_box(void)
{
    char prompt[40];
    snprintf(prompt, sizeof(prompt), "Line number (1-%d):", line_count());
    td_inputbox("Go to line", prompt, "", line_answer, NULL);
}
/* ------------------------------------------------------ load & save */

static bool load(const char *path)
{
    E.len = 0;
    E.buf[0] = '\0';
    E.readonly = false;
    if (!path || !path[0])
        return true;
    int n = fs()->read(path, E.buf, TD_EDITOR_MAX + 1);
    if (n < 0)
        return false;
    if (n > TD_EDITOR_MAX)
    {
        n = TD_EDITOR_MAX;
        E.readonly = true;
    }
    int out = 0;
    for (int i = 0; i < n; i++)
        if (E.buf[i] != '\r')
            E.buf[out++] = E.buf[i];
    E.len = out;
    E.buf[E.len] = '\0';
    return true;
}

/* After loading: keep what the text needs and some room to grow. */
static void fit_buffer(void)
{
    int cap = E.readonly ? E.len : E.len + GROW < TD_EDITOR_MAX ? E.len + GROW
                                                                : TD_EDITOR_MAX;
    if (cap < E.cap)
    {
        char *b = realloc(E.buf, (size_t)cap + 1 + CLS_MAX);
        if (b)
        {
            E.buf = b;
            E.cap = cap;
        }
    }
}

static void close_now(void)
{
    E.modified = false;
    td_win_close(s_win);
}

static bool save_to(const char *path)
{
    if (fs()->write(path, E.buf, E.len) != 0)
    {
        set_status("Save failed");
        E.run_after_save = false;
        return false;
    }
    /* save() passes E.path itself; copying a string onto itself is
     * undefined (glibc empties it, and the next save asks for a name). */
    if (path != E.path)
        snprintf(E.path, sizeof(E.path), "%s", path);
    if (E.undo)
    {
        E.undo->saved = E.undo->top;
        E.undo->merge = false;       /* typing after a save is a new step */
    }
    E.modified = false;
    update_mode();
    update_title();
    set_status("Saved");
    td_desktop_refresh();
    if (E.close_after_save)
    {
        close_now();
    }
    else if (E.run_after_save)
    {
        E.run_after_save = false;
        run_file();
    }
    return true;
}

static void save_as_answer(const char *name, void *user)
{
    (void)user;
    char path[PATH_LEN];
    if (!td_win_is_open(s_win))
        return;
    if (!td_valid_name(name))
    {
        set_status("Invalid file name");
        E.close_after_save = false;
        E.run_after_save = false;
        return;
    }
    snprintf(path, sizeof(path), "%s/%s", td_desktop_dir(), name);
    if (fs()->exists && fs()->exists(path))
    {
        set_status("A file with that name already exists");
        E.close_after_save = false;
        E.run_after_save = false;
        return;
    }
    save_to(path);
}

static void save(void)
{
    if (E.readonly)
    {
        set_status("Read-only: not saved");
        return;
    }
    if (E.path[0])
        save_to(E.path);
    else
        td_inputbox("Save as", "File name (saved on the Desktop):", E.run_after_save ? "untitled.py" : "untitled.txt",
                    save_as_answer, NULL);
}

static void unsaved_answer(int button, void *user)
{
    (void)user;
    if (button == 0)
    {          /* Save */
        E.close_after_save = true;
        save();
    }
    else if (button == 1)
    {   /* Discard */
        close_now();
    }
}

/* [x], window menu Close, Esc and Ctrl+W come here first. */
static bool on_close_request(td_window_t *win)
{
    (void)win;
    if (!E.modified)
        return true;
    char text[160], name[24 * 4 + 1];
    td_utf8_copy(name, sizeof(name), file_name(), 24); /* 24 characters, never half of one */
    snprintf(text, sizeof(text), "Save changes to %s?", name);
    td_msgbox("Editor", text, "Save|Discard|Cancel", unsaved_answer, NULL);
    return false;
}

/* ----------------------------------------------------- run & output */

/* The file type decides the extras (again after Save as). */
static void update_mode(void)
{
    bool was = E.python;
    E.python = td_is_python(file_name());
    E.script = td_is_script(file_name());
    if (E.python && !was)
        E.numbers = true;
}

/* The error at the end of a traceback in the output:
 *   Traceback (most recent call last):
 *     File "/home/root/x.py", line 3, in <module>
 *   NameError: name 'y' isn't defined
 * The deepest frame in this file is the one shown. */
static void find_error(void)
{
    static const char tb[] = "Traceback (most recent call last):\n";
    const char *o = E.out, *end = E.out + E.out_len, *t = NULL;
    for (const char *p = o; (p = memchr(p, 'T', (size_t)(end - p))) != NULL; p++)
        if (end - p >= (int)sizeof(tb) - 1 && memcmp(p, tb, sizeof(tb) - 1) == 0)
            t = p;
    if (!t)
        return;
    const char *me = td_shell_path(E.path), *my_name = file_name();
    char file[PATH_LEN] = "";
    int line = 0;
    bool here = false;
    const char *p = t + sizeof(tb) - 1;
    for (;;)
    {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        if (!nl)
            return;   /* the rest has not arrived yet */
        if (*p != ' ')
            break;    /* the message */
        static const char fl[] = "  File \"";
        const char *q = p + sizeof(fl) - 1;
        const char *close = q < nl ? memchr(q, '"', (size_t)(nl - q)) : NULL;
        if (memcmp(p, fl, sizeof(fl) - 1) == 0 && close && nl - close > 7 && memcmp(close, "\", line ", 8) == 0)
        {
            char name[PATH_LEN];
            int n = (int)(close - q) < PATH_LEN - 1 ? (int)(close - q) : PATH_LEN - 1;
            memcpy(name, q, (size_t)n);
            name[n] = '\0';
            const char *base = strrchr(name, '/');
            bool mine = !strcmp(name, me) || !strcmp(base ? base + 1 : name, my_name);
            if (mine || !here)
            {    /* the deepest frame of this file, else the deepest of all */
                snprintf(file, sizeof(file), "%s", name);
                line = atoi(close + 8);
                here = mine;
            }
        }
        p = nl + 1;
    }
    if (!line)
        return;
    const char *nl = memchr(p, '\n', (size_t)(end - p));
    int n = (int)(nl - p);
    if (n >= (int)sizeof(E.err_msg))
        n = (int)sizeof(E.err_msg) - 1;
    memcpy(E.err_msg, p, (size_t)n);
    E.err_msg[n] = '\0';
    snprintf(E.err_file, sizeof(E.err_file), "%s", file);
    E.err_line = line;
    E.err_here = here;
    E.err_done = true;
    td_wm_invalidate();
}

/* Every byte the program prints: kept (without escape sequences) to find
 * the error in. */
static void output_tap(const uint8_t *data, int len)
{
    if (!E.out)
        return;
    bool line = false;
    for (int i = 0; i < len; i++)
    {
        uint8_t c = data[i];
        switch (E.out_esc)
        {
        case 1:   /* after ESC */
            E.out_esc = c == '[' ? 2 : c == ']' ? 3
                                                : 0;
            continue;
        case 2:   /* CSI: up to the final byte */
            if (c >= 0x40 && c <= 0x7E)
                E.out_esc = 0;
            continue;
        case 3:   /* OSC: up to BEL or ESC */
            if (c == 7)
                E.out_esc = 0;
            else if (c == 0x1B)
                E.out_esc = 1;
            continue;
        default:
            break;
        }
        if (c == 0x1B)
        {
            E.out_esc = 1;
            continue;
        }
        if (c == '\r' || (c < 0x20 && c != '\n' && c != '\t'))
            continue;
        if (E.out_len == OUT_CAP)
        {   /* keep the newer half */
            memmove(E.out, E.out + OUT_CAP / 2, OUT_CAP / 2);
            E.out_len = OUT_CAP / 2;
        }
        E.out[E.out_len++] = (char)c;
        line |= c == '\n';
    }
    if (line && !E.err_done)
        find_error();
}

static void show_pane(bool focus)
{
    E.pane = true;
    E.pane_focus = focus;
    td_terminal_set_viewer(s_win, output_tap);
    td_wm_invalidate();
}

static void save(void);

/* F5: save, then run the file in the shell, its output in the pane. */
static void run_file(void)
{
    if (!E.python && !E.script && E.path[0])
    {
        set_status("F5 runs .py and " TD_SCRIPT_EXT " files");
        return;
    }
    if (!td_terminal_backend())
    {
        set_status("No shell in this build");
        return;
    }
    if (E.modified || !E.path[0])
    {
        E.run_after_save = true;
        save();
        return;
    }
    const char *p = td_shell_path(E.path);
    char cmd[PATH_LEN + 16];
    if (strchr(p, '"') || snprintf(cmd, sizeof(cmd), E.python ? "py \"%s\"" : "tdsh run \"%s\"", p) >= (int)sizeof(cmd))
    {
        set_status("This file name cannot be passed to the shell");
        return;
    }
    if (!E.out)
        E.out = malloc(OUT_CAP);
    E.out_len = 0;
    E.out_esc = 0;
    E.err_done = !E.python;
    E.err_line = 0;
    E.pane_scroll = 0;
    show_pane(false);
    td_terminal_command(cmd);
}

/* Shift+F5 or the menu: Ctrl+C for the program. */
static void stop_program(void)
{
    td_terminal_send("\x03", 1);
    set_status("Ctrl+C sent");
}

/* F8: to the line the last run stopped at (opening its file if it is
 * another one). */
static void go_to_error(void)
{
    if (!E.err_line)
    {
        set_status("No error from the last run");
        return;
    }
    if (E.err_here)
    {
        go_to_line(E.err_line);
        return;
    }
    const char *root = fs()->root ? fs()->root : "";
    char path[PATH_LEN], msg[sizeof(E.err_msg)];
    int line = E.err_line;
    snprintf(msg, sizeof(msg), "%s", E.err_msg);
    if (E.err_file[0] != '/' || snprintf(path, sizeof(path), "%s%s", root, E.err_file) >= (int)sizeof(path))
        return;
    td_editor_open(path);
    if (td_win_is_open(s_win) && strcmp(E.path, path) == 0)
    {
        E.err_line = line;
        E.err_here = true;
        snprintf(E.err_msg, sizeof(E.err_msg), "%s", msg);
        snprintf(E.err_file, sizeof(E.err_file), "%s", td_shell_path(path));
        go_to_line(line);
    }
}

/* ------------------------------------------------------- completion */

static void comp_close(void)
{
    if (E.comp_open)
        td_wm_invalidate();
    E.comp_open = false;
}

/* Look for suggestions at the cursor; `asked` (Ctrl+Space) also shows
 * them for one letter and says when there are none. */
static void comp_update(bool asked)
{
    if (!asked && (!E.comp_open && (!E.python || !td_editor_prefs()->completion)))
        return;
    E.comp_n = has_sel() ? 0
                         : pyed_complete(E.buf, E.len, E.cur, asked || E.comp_open ? 1 : 2, E.comp, COMP_MAX,
                                         &E.comp_start);
    E.comp_open = E.comp_n > 0;
    E.comp_sel = 0;
    E.comp_top = 0;
    if (asked && !E.comp_open)
        set_status("No suggestions");
    td_wm_invalidate();
}

static void comp_accept(void)
{
    const pyed_word_t *w = &E.comp[E.comp_sel];
    int typed = E.cur - E.comp_start;
    char text[48];
    int n = w->n - typed < (int)sizeof(text) ? w->n - typed : (int)sizeof(text);
    memcpy(text, w->s + typed, (size_t)n);   /* w may point into the text */
    E.comp_open = false;
    if (n > 0)
        insert_text(text, n, K_OTHER);
    td_wm_invalidate();
}

/* Keys for the open popup; false: not one of them (the popup closes). */
static bool comp_key(uint32_t k)
{
    switch (k)
    {
    case TD_KEY_UP:
        E.comp_sel = E.comp_sel > 0 ? E.comp_sel - 1 : E.comp_n - 1;
        break;
    case TD_KEY_DOWN:
        E.comp_sel = E.comp_sel < E.comp_n - 1 ? E.comp_sel + 1 : 0;
        break;
    case TD_KEY_ENTER:
    case TD_KEY_TAB:
        comp_accept();
        return true;
    case TD_KEY_ESC:
        comp_close();
        return true;
    default:
        return false;
    }
    if (E.comp_sel < E.comp_top)
        E.comp_top = E.comp_sel;
    if (E.comp_sel >= E.comp_top + COMP_ROWS)
        E.comp_top = E.comp_sel - COMP_ROWS + 1;
    td_wm_invalidate();
    return true;
}

/* ---------------------------------------------------------- drawing */

/* Colours for Python text and the line numbers, per theme (0 is Classic,
 * white on red; the others are dark). */
static uint8_t syntax_colour(int kind)
{
    static const uint8_t classic[PYED_KINDS] = {15, 11, 14, 10, 7, 13, 15, 13};
    static const uint8_t dark[PYED_KINDS] = {252, 75, 80, 114, 244, 180, 221, 176};
    if (kind <= PYED_TEXT || kind >= PYED_KINDS)
        return td_theme()->input_fg;
    return td_theme_index() == 0 ? classic[kind] : dark[kind];
}

/* The bracket matching the one at or just before the cursor, or -1. */
static int matching_bracket(int *at)
{
    static const char open[] = "([{", close[] = ")]}";
    for (int k = 0; k < 2; k++)
    {
        int p = E.cur - k;
        if (p < 0 || p >= E.len)
            continue;
        const char *o = strchr(open, E.buf[p]), *c = strchr(close, E.buf[p]);
        if (!E.buf[p] || (!o && !c))
            continue;
        char want = o ? close[o - open] : open[c - close];
        int step = o ? 1 : -1, depth = 0;
        for (int i = p, n = 0; i >= 0 && i < E.len && n < 8000; i += step, n++)
        {
            if (E.buf[i] == E.buf[p])
                depth++;
            else if (E.buf[i] == want && --depth == 0)
            {
                *at = p;
                return i;
            }
        }
        return -1;
    }
    return -1;
}

static int digits(int n)
{
    int d = 1;
    while (n >= 10)
    {
        n /= 10;
        d++;
    }
    return d;
}

/* The suggestions, under the cursor (or above it near the bottom). */
static void draw_completion(int cx, int cy, int w)
{
    const td_theme_t *t = td_theme();
    int rows = E.comp_n < COMP_ROWS ? E.comp_n : COMP_ROWS, width = 8;
    for (int i = 0; i < E.comp_n; i++)
        if (E.comp[i].n + 2 > width)
            width = E.comp[i].n + 2;
    if (width > 30)
        width = 30;
    int x = cx - (E.cur - E.comp_start);
    if (x + width > w)
        x = w - width;
    if (x < 0)
        x = 0;
    int y = cy + 1 + rows <= E.view_rows ? cy + 1 : cy - rows;
    if (y < 0)
        y = 0;
    for (int r = 0; r < rows; r++)
    {
        int i = E.comp_top + r;
        bool sel = i == E.comp_sel;
        uint8_t fg = sel ? t->menu_select_fg : t->menu_fg, bg = sel ? t->menu_select_bg : t->menu_bg;
        char word[40];
        int n = E.comp[i].n < (int)sizeof(word) - 1 ? E.comp[i].n : (int)sizeof(word) - 1;
        memcpy(word, E.comp[i].s, (size_t)n);
        word[n] = '\0';
        td_fill(td_rect(x, y + r, width, 1), ' ', fg, bg);
        td_textn(x + 1, y + r, word, width - 2, fg, bg, 0);
    }
    if (E.comp_n > rows)
    {
        char more[24];
        snprintf(more, sizeof(more), "%d/%d", E.comp_sel + 1, E.comp_n);
        td_textn(x + width - (int)strlen(more), y + rows - 1, more, width, t->dim, t->menu_bg, 0);
    }
}

static void on_draw(td_window_t *win, int w, int h)
{
    const td_theme_t *t = td_theme();
    bool focused = win == td_win_focused();
    E.rows_all = h - 1;           /* the last row is the status bar */
    int rows = E.rows_all, pane_rows = 0;
    if (E.pane && E.rows_all >= 6)
    {
        pane_rows = E.pane_h ? E.pane_h : E.rows_all / 3;
        if (pane_rows > E.rows_all - 4)
            pane_rows = E.rows_all - 4;
        if (pane_rows < 2)
            pane_rows = 2;
        rows = E.rows_all - pane_rows - 1;
    }
    E.view_rows = rows > 1 ? rows : 1;
    E.pane_y = pane_rows ? rows : -1;
    E.gutter = E.numbers ? (digits(line_count()) > 2 ? digits(line_count()) : 2) + 2 : 0;
    int tw = w - E.gutter > 1 ? w - E.gutter : 1;   /* text columns */
    int cl = line_of(E.cur), cc = col_of(E.cur);

    if (E.follow)
    {
        if (cl < E.top)
            E.top = cl;
        if (cl >= E.top + E.view_rows)
            E.top = cl - E.view_rows + 1;
        if (cc < E.left)
            E.left = cc;
        if (cc >= E.left + tw)
            E.left = cc - tw + 1;
        E.follow = false;
    }

    int sa = 0, sb = 0, ba = -1, bb = -1;
    if (has_sel())
        sel_range(&sa, &sb);
    else
        bb = matching_bracket(&ba);

    /* Lines above the view only matter for a string that runs over. */
    int state = 0, p = 0;
    for (int line = 0; line < E.top; line++)
    {
        int e = line_end(p);
        if (E.python)
            state = pyed_lex_line(E.buf + p, e - p, state, NULL, 0);
        if (e >= E.len)
            break;
        p = e + 1;
    }

    td_fill(td_rect(0, 0, w, rows), ' ', t->input_fg, t->input_bg);
    uint8_t *cls = (uint8_t *)E.buf + E.cap + 1;   /* CLS_MAX bytes after the text */
    for (int row = 0; row < rows; row++)
    {
        int line = E.top + row;
        if (line >= line_count())
            break;
        int e = line_end(p);
        if (E.python)
            state = pyed_lex_line(E.buf + p, e - p, state, cls, CLS_MAX);
        if (E.gutter)
        {
            bool err = E.err_here && line + 1 == E.err_line;
            uint8_t fg = err ? 231 : line == cl ? syntax_colour(PYED_TEXT)
                                                : syntax_colour(PYED_COMMENT);
            uint8_t bg = err ? (td_theme_index() == 0 ? 9 : 124) : t->input_bg;
            char num[28];
            snprintf(num, sizeof(num), "%*d ", E.gutter - 1 < 12 ? E.gutter - 1 : 12, line + 1);
            td_text(0, row, num, fg, bg, 0);
        }
        const char *s = E.buf + p;
        int col = 0;
        while (s < E.buf + e)
        {
            int at = (int)(s - E.buf);
            uint32_t cp = td_utf8_next(&s);
            if (cp == 0)
            {              /* a NUL byte: show it, step over it */
                cp = '?';
                s++;
            }
            if (s > E.buf + e)
                break;
            if (cp == '\t')
                cp = ' ';
            else if (cp < 0x20u)
                cp = '?';
            bool sel = at >= sa && at < sb;
            if (col >= E.left && col - E.left < tw)
            {
                uint8_t fg = E.python && at - p < CLS_MAX ? syntax_colour(cls[at - p]) : t->input_fg;
                uint8_t attr = at == ba || at == bb ? TD_BOLD | TD_UNDERLINE : 0;
                td_putc(E.gutter + col - E.left, row, cp, sel ? t->select_fg : fg, sel ? t->select_bg : t->input_bg, attr);
            }
            col++;
        }
        /* A selected line break shows as one highlighted cell. */
        if (e < E.len && e >= sa && e < sb && col >= E.left && col - E.left < tw)
            td_putc(E.gutter + col - E.left, row, ' ', t->select_fg, t->select_bg, 0);
        p = e + 1;
    }

    bool text_focus = focused && !E.pane_focus;
    int cx = E.gutter + cc - E.left, cy = cl - E.top;
    if (text_focus && cy >= 0 && cy < rows && cc >= E.left && cc - E.left < tw)
    {
        uint32_t ch = ' ';
        if (E.cur < E.len && E.buf[E.cur] != '\n')
        {
            const char *s = E.buf + E.cur;
            ch = td_utf8_next(&s);
            if (ch < 0x20u)
                ch = ' ';
        }
        td_putc(cx, cy, ch, t->input_fg, t->input_bg, TD_REVERSE);
    }

    /* Output pane: a title row, then the shell session. */
    if (pane_rows)
    {
        bool hot = focused && E.pane_focus;
        uint8_t fg = hot ? t->title_fg : t->title_inactive_fg, bg = hot ? t->title_bg : t->title_inactive_bg;
        td_fill(td_rect(0, rows, w, 1), ' ', fg, bg);
        td_textn(1, rows, hot ? "Output - keys go to the program; F7 or Esc: back to the text" : "Output - F7: type here (input, Ctrl+C)  F5: run again",
                 w - 2, fg, bg, 0);
        E.pane_scroll = td_terminal_draw_view(win, 0, rows + 1, w, pane_rows, E.pane_scroll, hot);
    }

    /* Status bar. */
    char bar[200];
    bool recent = E.status[0] && td_millis() - E.status_ms < 4000;
    int n = snprintf(bar, sizeof(bar), " Ln %d, Col %d", cl + 1, cc + 1);
    if (has_sel())
        n += snprintf(bar + n, sizeof(bar) - (size_t)n, "  %d selected", sel_chars());
    if (E.readonly)
        n += snprintf(bar + n, sizeof(bar) - (size_t)n, "  [read-only]");
    char *info = bar + n;
    size_t room = sizeof(bar) - (size_t)n;
    if (recent)
        snprintf(info, room, "  %s", E.status);
    else if (E.err_line && E.err_here)
        snprintf(info, room, "  Line %d: %s  (F8)", E.err_line, E.err_msg);
    else if (E.err_line)
        snprintf(info, room, "  %.40s line %d: %s  (F8)", E.err_file, E.err_line, E.err_msg);
    else if (E.python || E.script)
        snprintf(info, room, "  F5 Run  F7 Output  ^F Find  ^G Line  ^/ Comment%s", E.python ? "  ^Space Complete" : "");
    else
        snprintf(info, room, "  ^S Save  ^Z Undo  ^Y Redo  ^F Find  ^G Line  ^C Copy  ^V Paste");
    bool err_bar = !recent && E.err_line;
    uint8_t bfg = err_bar ? 231 : t->title_inactive_fg, bbg = err_bar ? (td_theme_index() == 0 ? 1 : 52) : t->title_inactive_bg;
    td_fill(td_rect(0, E.rows_all, w, 1), ' ', bfg, bbg);
    td_textn(0, E.rows_all, bar, w, bfg, bbg, 0);

    if (E.comp_open && text_focus && cy >= 0 && cy < rows)
        draw_completion(cx, cy, w);
}

/* ------------------------------------------------------------ input */

static void settings_changed(void)
{
    if (!td_settings_save())
        set_status("Not saved: no settings file here");
}

static void toggle_completion(void)
{
    td_editor_prefs()->completion = !td_editor_prefs()->completion;
    settings_changed();
    set_status(td_editor_prefs()->completion ? "Suggestions pop up while typing" : "Suggestions only with Ctrl+Space");
}

/* Keys while the output pane has the keyboard: the program gets them. */
static bool pane_key(const td_event_t *ev)
{
    uint32_t k = ev->key;
    if (k == TD_KEY_F7 || k == TD_KEY_ESC)
    {
        E.pane_focus = false;
        td_wm_invalidate();
    }
    else if (k == TD_KEY_F5 && !(ev->mods & TD_MOD_SHIFT))
    {
        run_file();
    }
    else if (k == TD_KEY_F8)
    {
        go_to_error();
    }
    else if ((ev->mods & TD_MOD_SHIFT) && (k == TD_KEY_PGUP || k == TD_KEY_PGDN))
    {
        int page = E.rows_all - E.pane_y - 1;
        E.pane_scroll += k == TD_KEY_PGUP ? page : -page;
        if (E.pane_scroll < 0)
            E.pane_scroll = 0;
        td_wm_invalidate();
    }
    else
    {
        E.pane_scroll = 0;
        td_terminal_key(ev);
    }
    return true;
}

static bool on_key(const td_event_t *ev)
{
    uint32_t k = ev->key;
    bool ctrl = (ev->mods & TD_MOD_CTRL) != 0;
    bool shift = (ev->mods & TD_MOD_SHIFT) != 0;

    if (E.pane_focus && E.pane)
        return pane_key(ev);
    if (E.comp_open && !ctrl && comp_key(k))
        return true;

    switch (k)
    {
    case TD_KEY_F3:
        find_next();
        return true;
    case TD_KEY_F5:
        comp_close();
        if (shift)
            stop_program();
        else
            run_file();
        return true;
    case TD_KEY_F7:
        comp_close();
        show_pane(true);
        return true;
    case TD_KEY_F8:
        go_to_error();
        return true;
    default:
        break;
    }

    if (ctrl && (k == ' ' || k == '_' || k == '/'))
    {
        if (k == ' ')
            comp_update(true);
        else
            toggle_comment();
        return true;
    }
    if (ctrl && k >= 'a' && k <= 'z')
    {
        comp_close();
        switch (k)
        {
        case 's':
            E.run_after_save = false;
            save();
            return true;
        case 'w':
        case 'q':
            td_win_request_close(s_win);
            return true;
        case 'a':
            select_all();
            return true;
        case 'c':
            copy_selection();
            return true;
        case 'x':
            cut_selection();
            return true;
        case 'v':
            paste();
            return true;
        case 'z':
            if (shift)
                redo();
            else
                undo();
            return true;
        case 'y':
            redo();
            return true;
        case 'f':
            find_box();
            return true;
        case 'r':
            replace_box();
            return true;
        case 'g':
            line_box();
            return true;
        default:
            break;
        }
    }
    if (k == TD_KEY_ESC)
    {
        td_win_request_close(s_win);
        return true;
    }
    if (k == TD_KEY_INSERT)
    {
        if (ctrl)
            copy_selection();
        else if (shift)
            paste();
        return true;
    }
    if (k == TD_KEY_DELETE && shift)
    {
        cut_selection();
        return true;
    }

    /* Movement; Shift extends the selection. */
    bool keep_goal = false, moved = true;
    int before = E.cur;
    bool sel = has_sel();
    int sa = 0, sb = 0;
    if (sel)
        sel_range(&sa, &sb);
    switch (k)
    {
    case TD_KEY_LEFT:
        if (sel && !shift)
            E.cur = sa;
        else
            E.cur = ctrl ? word_left(E.cur) : prev_pos(E.cur);
        break;
    case TD_KEY_RIGHT:
        if (sel && !shift)
            E.cur = sb;
        else
            E.cur = ctrl ? word_right(E.cur) : next_pos(E.cur);
        break;
    case TD_KEY_UP:
        move_lines(-1);
        keep_goal = true;
        break;
    case TD_KEY_DOWN:
        move_lines(1);
        keep_goal = true;
        break;
    case TD_KEY_PGUP:
        move_lines(-E.view_rows);
        keep_goal = true;
        break;
    case TD_KEY_PGDN:
        move_lines(E.view_rows);
        keep_goal = true;
        break;
    case TD_KEY_HOME:
        E.cur = ctrl ? 0 : line_start(E.cur);
        break;
    case TD_KEY_END:
        E.cur = ctrl ? E.len : line_end(E.cur);
        break;
    default:
        moved = false;
        break;
    }
    if (moved)
    {
        comp_close();
        if (shift)
        {
            if (E.anchor < 0 || !sel)
                E.anchor = before;
        }
        else
        {
            E.anchor = -1;
        }
        if (E.undo)
            E.undo->merge = false;   /* typing elsewhere is a new step */
        if (!keep_goal)
            E.goal_col = col_of(E.cur);
        E.follow = true;
        td_wm_invalidate();
        return true;
    }

    bool suggest = false;
    switch (k)
    {
    case TD_KEY_ENTER:
        newline();
        break;
    case TD_KEY_TAB:
        tab_key(shift);
        break;
    case TD_KEY_BACKSPACE:
        backspace();
        suggest = E.comp_open;
        break;
    case TD_KEY_DELETE:
        delete_key(false);
        break;
    default:
        if (ctrl || (ev->mods & TD_MOD_ALT) || k < 0x20 || k >= TD_KEY_BASE)
            return false;
        {
            uint8_t u[4];
            insert_text((const char *)u, td_utf8_encode(k, u), K_TYPE);
        }
        suggest = k == '.' || k == '_' || (k < 0x80 && word_char((char)k));
        break;
    }
    if (suggest)
        comp_update(false);
    else
        comp_close();
    E.goal_col = col_of(E.cur);
    E.follow = true;
    td_wm_invalidate();
    return true;
}

/* ------------------------------------------------------ context menu */

enum
{
    M_UNDO,
    M_REDO,
    M_CUT,
    M_COPY,
    M_PASTE,
    M_ALL,
    M_FIND,
    M_REPLACE,
    M_LINE,
    M_COMMENT,
    M_RUN,
    M_STOP,
    M_ERROR,
    M_PANE,
    M_NUMBERS,
    M_COMPLETE,
    M_SEP
};

static int s_menu_ids[20];

static void menu_chosen(int item, void *user)
{
    (void)user;
    if (!td_win_is_open(s_win) || item < 0 || item >= (int)(sizeof(s_menu_ids) / sizeof(s_menu_ids[0])))
        return;
    switch (s_menu_ids[item])
    {
    case M_UNDO:
        undo();
        break;
    case M_REDO:
        redo();
        break;
    case M_CUT:
        cut_selection();
        break;
    case M_COPY:
        copy_selection();
        break;
    case M_PASTE:
        paste();
        break;
    case M_ALL:
        select_all();
        break;
    case M_FIND:
        find_box();
        break;
    case M_REPLACE:
        replace_box();
        break;
    case M_LINE:
        line_box();
        break;
    case M_COMMENT:
        toggle_comment();
        break;
    case M_RUN:
        run_file();
        break;
    case M_STOP:
        stop_program();
        break;
    case M_ERROR:
        go_to_error();
        break;
    case M_PANE:
        if (E.pane)
        {
            E.pane = E.pane_focus = false;
            td_wm_invalidate();
        }
        else
        {
            show_pane(false);
        }
        break;
    case M_NUMBERS:
        E.numbers = !E.numbers;
        td_wm_invalidate();
        break;
    case M_COMPLETE:
        toggle_completion();
        break;
    default:
        break;
    }
}

static void context_menu(int x, int y)
{
    const char *items[20];
    int n = 0;
#define ITEM(id, label) (s_menu_ids[n] = (id), items[n++] = (label))
    bool code = E.python || E.script;
    ITEM(M_UNDO, "Undo         Ctrl+Z");
    ITEM(M_REDO, "Redo         Ctrl+Y");
    ITEM(M_SEP, "-");
    ITEM(M_CUT, "Cut          Ctrl+X");
    ITEM(M_COPY, "Copy         Ctrl+C");
    ITEM(M_PASTE, "Paste        Ctrl+V");
    ITEM(M_ALL, "Select all   Ctrl+A");
    ITEM(M_SEP, "-");
    ITEM(M_FIND, "Find...      Ctrl+F");
    ITEM(M_REPLACE, "Replace all  Ctrl+R");
    ITEM(M_LINE, "Go to line   Ctrl+G");
    ITEM(M_COMMENT, "Comment      Ctrl+/");
    if (code)
    {
        ITEM(M_SEP, "-");
        ITEM(M_RUN, "Run          F5");
        ITEM(M_STOP, "Stop (Ctrl+C)");
        ITEM(M_ERROR, "Go to error  F8");
        ITEM(M_PANE, E.pane ? "Hide output" : "Show output  F7");
    }
    ITEM(M_SEP, "-");
    ITEM(M_NUMBERS, E.numbers ? "Line numbers   [x]" : "Line numbers   [ ]");
    if (E.python)
        ITEM(M_COMPLETE, td_editor_prefs()->completion ? "Auto-complete  [x]" : "Auto-complete  [ ]");
#undef ITEM
    td_rect_t c = td_win_client(s_win);
    td_menu_popup(c.x + x, c.y + y, items, n, menu_chosen, NULL);
}

/* ------------------------------------------------------------- mouse */

/* Text position under client cell (x, y), scrolling when y is outside. */
static int pos_at(int x, int y)
{
    if (y < 0)
    {
        E.top += y;
        if (E.top < 0)
            E.top = 0;
        y = 0;
    }
    else if (y >= E.view_rows)
    {
        E.top += y - E.view_rows + 1;
        if (E.top > line_count() - 1)
            E.top = line_count() - 1;
        y = E.view_rows - 1;
    }
    int line = E.top + y;
    if (line >= line_count())
        line = line_count() - 1;
    x -= E.gutter;
    return at_col(nth_line(line), E.left + (x < 0 ? 0 : x));
}

static bool on_mouse(const td_event_t *ev)
{
    bool in_pane = E.pane_y >= 0 && ev->y > E.pane_y && ev->y < E.rows_all;
    if (ev->button == TD_BUTTON_WHEEL_UP || ev->button == TD_BUTTON_WHEEL_DOWN)
    {
        int d = ev->button == TD_BUTTON_WHEEL_UP ? 3 : -3;
        if (in_pane)
        {
            E.pane_scroll = E.pane_scroll + d > 0 ? E.pane_scroll + d : 0;
            td_wm_invalidate();
            return true;
        }
        E.top -= d;
        if (E.top > line_count() - 1)
            E.top = line_count() - 1;
        if (E.top < 0)
            E.top = 0;
        td_wm_invalidate();
        return true;
    }
    if (E.pane_drag)
    {
        if (ev->action == TD_MOUSE_RELEASE)
            E.pane_drag = false;
        else
            E.pane_h = E.rows_all - ev->y - 1 > 2 ? E.rows_all - ev->y - 1 : 2;
        td_wm_invalidate();
        return true;
    }
    if (ev->action == TD_MOUSE_PRESS && E.pane_y >= 0 && ev->y >= E.pane_y && ev->y < E.rows_all)
    {
        comp_close();
        if (ev->y == E.pane_y && ev->button == TD_BUTTON_LEFT)
            E.pane_drag = true;    /* its title row: drag to resize */
        else if (ev->button == TD_BUTTON_RIGHT)
            context_menu(ev->x, ev->y);
        else
            E.pane_focus = true;
        td_wm_invalidate();
        return true;
    }
    if (ev->action == TD_MOUSE_PRESS)
    {
        E.pane_focus = false;
        comp_close();
    }
    if (ev->action == TD_MOUSE_PRESS && ev->button == TD_BUTTON_RIGHT)
    {
        if (ev->y < E.view_rows && !has_sel())
            E.cur = pos_at(ev->x, ev->y);
        context_menu(ev->x, ev->y);
        td_wm_invalidate();
        return true;
    }
    if (ev->action == TD_MOUSE_PRESS && ev->button == TD_BUTTON_LEFT && ev->y < E.view_rows)
    {
        int p = pos_at(ev->x, ev->y);
        uint32_t now = td_millis();
        if (p == E.click_pos && now - E.click_ms < DOUBLE_CLICK_MS)
        {
            /* Double-click: select the word (or run of other characters). */
            int a = p, b = p;
            bool wc = p < E.len && word_char(E.buf[p]);
            while (a > 0 && word_char(E.buf[a - 1]) == wc && E.buf[a - 1] != '\n')
                a--;
            while (b < E.len && word_char(E.buf[b]) == wc && E.buf[b] != '\n')
                b++;
            E.anchor = a;
            E.cur = b;
            E.click_ms = 0;
            E.mouse_down = false;
        }
        else
        {
            if (ev->mods & TD_MOD_SHIFT)
            {
                if (E.anchor < 0)
                    E.anchor = E.cur;
            }
            else
            {
                E.anchor = p;
            }
            E.cur = p;
            E.click_ms = now;
            E.click_pos = p;
            E.mouse_down = true;
        }
        if (E.undo)
            E.undo->merge = false;
        E.goal_col = col_of(E.cur);
        td_wm_invalidate();
        return true;
    }
    if (ev->action == TD_MOUSE_DRAG && E.mouse_down)
    {
        E.cur = pos_at(ev->x, ev->y);
        E.goal_col = col_of(E.cur);
        td_wm_invalidate();
        return true;
    }
    if (ev->action == TD_MOUSE_RELEASE && E.mouse_down)
    {
        E.mouse_down = false;
        if (E.anchor == E.cur)
            E.anchor = -1;
        td_wm_invalidate();
    }
    return true;
}

static bool on_event(td_window_t *win, const td_event_t *ev)
{
    (void)win;
    if (ev->type == TD_EV_KEY)
        return on_key(ev);
    if (ev->type == TD_EV_PASTE)
    {
        if (E.pane && E.pane_focus)
        {
            int n = 0;
            const char *text = td_paste_text(&n);
            if (text && n > 0)
                td_terminal_send(text, n);
        }
        else
        {
            comp_close();
            paste_from_terminal(ev);
        }
        return true;
    }
    if (ev->type == TD_EV_MOUSE)
        return on_mouse(ev);
    return false;
}

/* A file dropped on the Editor is opened (unless there are unsaved changes). */
static bool on_drop(td_window_t *win, int x, int y, const td_drag_item_t *item)
{
    (void)win;
    (void)x;
    (void)y;
    if (!item->is_dir)
        td_editor_open(item->path);
    return true;
}

/* Output shows up quickly while the pane is open. */
static void on_tick(td_window_t *win)
{
    (void)win;
    if (E.pane)
        td_terminal_pump();
}

static void on_close(td_window_t *win)
{
    (void)win;
    td_terminal_set_viewer(NULL, NULL);
    free(E.buf);
    free(E.undo);
    free(E.out);
    E.buf = NULL;
    E.undo = NULL;
    E.out = NULL;
    s_win = NULL;
}

/* -------------------------------------------------------------- api */

void td_editor_open(const char *path)
{
    if (!fs() || !fs()->read || !fs()->write)
    {
        td_msgbox("Editor", "No filesystem on this platform.", "OK", NULL, NULL);
        return;
    }
    if (td_win_is_open(s_win))
    {
        if (path && strcmp(path, E.path) == 0)
        {
            td_win_focus(s_win);
            return;
        }
        if (E.modified)
        {
            td_win_focus(s_win);
            set_status("Save or close this file first");
            return;
        }
        td_win_close(s_win);
    }

    memset(&E, 0, sizeof(E));
    E.anchor = -1;
    E.click_pos = -1;
    E.pane_y = -1;
    E.buf = malloc(TD_EDITOR_MAX + 1 + CLS_MAX);   /* to load; fit_buffer() shrinks it */
    E.cap = TD_EDITOR_MAX;
    E.undo = calloc(1, sizeof(undo_t));   /* without it: no undo, still edits */
    if (!E.buf)
    {
        free(E.undo);
        E.undo = NULL;
        td_msgbox("Editor", "Not enough memory.", "OK", NULL, NULL);
        return;
    }
    if (!load(path))
    {
        free(E.buf);
        free(E.undo);
        E.buf = NULL;
        E.undo = NULL;
        td_msgbox("Editor", "The file could not be read.", "OK", NULL, NULL);
        return;
    }
    fit_buffer();
    if (path)
        snprintf(E.path, sizeof(E.path), "%s", path);
    update_mode();

    td_window_desc_t d = {
        .title = "Editor",
        .rect = td_rect(-1, -1, 72, 20),
        .flags = TD_WIN_DEFAULT | TD_WIN_RAW_KEYS,
        .min_w = 30,
        .min_h = 6,
        .on_draw = on_draw,
        .on_event = on_event,
        .on_close = on_close,
        .on_close_request = on_close_request,
        .on_drop = on_drop,
        .on_tick = on_tick,
        .tick_ms = 30,
    };
    s_win = td_win_create(&d);
    if (!s_win)
    {
        free(E.buf);
        free(E.undo);
        E.buf = NULL;
        E.undo = NULL;
        return;
    }
    E.follow = true;
    update_title();
    if (E.readonly)
        set_status("File too large: read-only");
    else if (!E.undo)
        set_status("Low memory: undo is off");
}

static void launch(void)
{
    if (td_win_is_open(s_win))
        td_win_focus(s_win);
    else
        td_editor_open(NULL);
}

static const td_app_t s_app = {"Editor", launch, "\xC2\xB6_"};

void td_editor_register(void)
{
    td_app_register(&s_app);
}
