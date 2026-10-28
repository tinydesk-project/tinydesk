/*
 * editor_py.c - Python support for the Editor: a line lexer for syntax
 * colours, and completion suggestions from fixed word lists (keywords,
 * built-ins, the modules TinyDesk's MicroPython has) and from the file.
 */
#include "editor_py.h"

#include <stdbool.h>
#include <string.h>

/* Word lists, space separated. */
static const char s_keywords[] = "False None True and as assert async await break class continue def del elif else "
                                 "except finally for from global if import in is lambda nonlocal not or pass raise "
                                 "return try while with yield";

static const char s_builtins[] =
    "abs all any bin bool bytearray bytes callable chr classmethod compile complex delattr dict dir divmod "
    "enumerate eval exec filter float frozenset getattr globals hasattr hash help hex id input int isinstance "
    "issubclass iter len list locals map max memoryview min next object oct open ord pow print property range "
    "repr reversed round self set setattr slice sorted staticmethod str sum super tuple type zip "
    "ArithmeticError AssertionError AttributeError BaseException EOFError Exception GeneratorExit ImportError "
    "IndentationError IndexError KeyError KeyboardInterrupt LookupError MemoryError NameError "
    "NotImplementedError OSError OverflowError RuntimeError StopAsyncIteration StopIteration SyntaxError "
    "SystemExit TypeError UnicodeError ValueError ZeroDivisionError";

/* Modules and their names: the module name, then what "module." offers. */
static const char *const s_modules[][2] = {
    {"tinydesk", "shell log mqtt_publish modbus_read modbus_write version"},
    {"asyncio", "run sleep sleep_ms create_task gather wait_for wait_for_ms current_task get_event_loop "
                "new_event_loop Event Lock Task CancelledError TimeoutError open_connection start_server"},
    {"os", "listdir ilistdir mkdir rmdir remove rename stat getcwd chdir urandom uname sep"},
    {"sys", "argv path exit platform version version_info implementation modules byteorder maxsize stdin "
            "stdout stderr print_exception"},
    {"time", "sleep sleep_ms sleep_us ticks_ms ticks_us ticks_diff ticks_add time time_ns localtime gmtime mktime"},
    {"json", "dumps loads dump load"},
    {"math", "pi e inf nan sqrt pow exp log log10 log2 sin cos tan asin acos atan atan2 floor ceil fabs fmod "
             "trunc radians degrees isnan isinf isfinite copysign"},
    {"random", "random randint randrange choice uniform getrandbits seed"},
    {"re", "compile match search sub split"},
    {"hashlib", "sha256 sha1 md5"},
    {"binascii", "hexlify unhexlify a2b_base64 b2a_base64 crc32"},
    {"gc", "collect mem_free mem_alloc enable disable isenabled threshold"},
    {"micropython", "const native viper mem_info qstr_info stack_use heap_lock heap_unlock opt_level"},
    {"struct", "pack unpack calcsize pack_into unpack_from"},
    {"collections", "namedtuple OrderedDict deque"},
    {"heapq", "heappush heappop heapify"},
    {"errno", "errorcode ENOENT EEXIST EINVAL EIO EACCES EBADF ENOMEM ETIMEDOUT"},
    {"select", "poll POLLIN POLLOUT POLLERR POLLHUP"},
};
#define MODULE_COUNT ((int)(sizeof(s_modules) / sizeof(s_modules[0])))

static bool ident_start(char c)
{
    unsigned char u = (unsigned char)c;
    return u >= 0x80 || u == '_' || ((u | 0x20) >= 'a' && (u | 0x20) <= 'z');
}

static bool ident_char(char c)
{
    return ident_start(c) || (c >= '0' && c <= '9');
}

/* Is s[0..n) one of the words in the space-separated list? */
static bool in_list(const char *list, const char *s, int n)
{
    const char *p = list;
    while (*p)
    {
        const char *e = strchr(p, ' ');
        int m = e ? (int)(e - p) : (int)strlen(p);
        if (m == n && memcmp(p, s, (size_t)n) == 0)
            return true;
        if (!e)
            break;
        p = e + 1;
    }
    return false;
}

/* ------------------------------------------------------------ lexer */

static void mark(uint8_t *cls, int cap, int a, int b, uint8_t kind)
{
    if (!cls)
        return;
    if (b > cap)
        b = cap;
    for (int i = a; i < b; i++)
        cls[i] = kind;
}

/* Index after the closing triple quote q found from `from`, or -1. */
static int triple_end(const char *s, int n, int from, char q)
{
    for (int j = from; j < n; j++)
    {
        if (s[j] == '\\')
            j++;
        else if (s[j] == q && j + 2 < n && s[j + 1] == q && s[j + 2] == q)
            return j + 3;
    }
    return -1;
}

/* A string whose quote is at s[q] (a prefix such as r or b starts at
 * `from`). Returns the index after it; *state is set for a triple-quoted
 * string that goes on past the line. */
static int string_at(const char *s, int n, int from, int q, int *state, uint8_t *cls, int cap)
{
    char c = s[q];
    if (q + 2 < n && s[q + 1] == c && s[q + 2] == c)
    {
        int end = triple_end(s, n, q + 3, c);
        if (end < 0)
        {
            mark(cls, cap, from, n, PYED_STRING);
            *state = c == '\'' ? 1 : 2;
            return n;
        }
        mark(cls, cap, from, end, PYED_STRING);
        return end;
    }
    int j = q + 1;
    while (j < n)
    {
        if (s[j] == '\\')
            j += 2;
        else if (s[j++] == c)
            break;
    }
    if (j > n)
        j = n;
    mark(cls, cap, from, j, PYED_STRING);
    return j;
}

static bool string_prefix(const char *s, int n)
{
    if (n < 1 || n > 2)
        return false;
    for (int i = 0; i < n; i++)
    {
        char c = (char)(s[i] | 0x20);
        if (c != 'r' && c != 'b' && c != 'f' && c != 'u')
            return false;
    }
    return true;
}

int pyed_lex_line(const char *s, int n, int state, uint8_t *cls, int cap)
{
    mark(cls, cap, 0, n, PYED_TEXT);
    int i = 0;
    if (state)
    {
        int end = triple_end(s, n, 0, state == 1 ? '\'' : '"');
        if (end < 0)
        {
            mark(cls, cap, 0, n, PYED_STRING);
            return state;
        }
        mark(cls, cap, 0, end, PYED_STRING);
        i = end;
        state = 0;
    }
    bool name_next = false;   /* after def / class */
    bool line_start = true;   /* only blanks so far */
    while (i < n)
    {
        char c = s[i];
        if (c == ' ' || c == '\t')
        {
            i++;
            continue;
        }
        if (c == '#')
        {
            mark(cls, cap, i, n, PYED_COMMENT);
            break;
        }
        if (c == '\'' || c == '"')
        {
            i = string_at(s, n, i, i, &state, cls, cap);
        }
        else if ((c >= '0' && c <= '9') || (c == '.' && i + 1 < n && s[i + 1] >= '0' && s[i + 1] <= '9'))
        {
            int j = i + 1;
            while (j < n && (ident_char(s[j]) || s[j] == '.'))
                j++;
            mark(cls, cap, i, j, PYED_NUMBER);
            i = j;
        }
        else if (c == '@' && line_start)
        {
            int j = i + 1;
            while (j < n && (ident_char(s[j]) || s[j] == '.'))
                j++;
            mark(cls, cap, i, j, PYED_DECORATOR);
            i = j;
        }
        else if (ident_start(c))
        {
            int j = i;
            while (j < n && ident_char(s[j]))
                j++;
            if (j < n && (s[j] == '\'' || s[j] == '"') && string_prefix(s + i, j - i))
            {
                i = string_at(s, n, i, j, &state, cls, cap);
            }
            else
            {
                bool kw = in_list(s_keywords, s + i, j - i);
                uint8_t kind = name_next                           ? PYED_NAME
                               : kw                                ? PYED_KEYWORD
                               : in_list(s_builtins, s + i, j - i) ? PYED_BUILTIN
                                                                   : PYED_TEXT;
                mark(cls, cap, i, j, kind);
                name_next = kw && ((j - i == 3 && !memcmp(s + i, "def", 3)) || (j - i == 5 && !memcmp(s + i, "class", 5)));
                line_start = false;
                i = j;
                continue;
            }
        }
        else
        {
            i++;
        }
        name_next = false;
        line_start = false;
        if (state)
            return state;
    }
    return state;
}

/* ------------------------------------------------------- completion */

typedef struct
{
    pyed_word_t *out;
    int max, count;
    const char *prefix;
    int plen;
} found_t;

static void add(found_t *f, const char *s, int n)
{
    if (n <= f->plen || memcmp(s, f->prefix, (size_t)f->plen) != 0)
        return;
    for (int i = 0; i < f->count; i++)
        if (f->out[i].n == n && memcmp(f->out[i].s, s, (size_t)n) == 0)
            return;
    if (f->count >= f->max)
        return;
    /* Keep them sorted (insertion). */
    int at = f->count;
    while (at > 0)
    {
        const pyed_word_t *w = &f->out[at - 1];
        int m = w->n < n ? w->n : n;
        int d = memcmp(w->s, s, (size_t)m);
        if (d < 0 || (d == 0 && w->n <= n))
            break;
        f->out[at] = f->out[at - 1];
        at--;
    }
    f->out[at].s = s;
    f->out[at].n = n;
    f->count++;
}

static void add_list(found_t *f, const char *list)
{
    const char *p = list;
    while (*p)
    {
        const char *e = strchr(p, ' ');
        int m = e ? (int)(e - p) : (int)strlen(p);
        add(f, p, m);
        if (!e)
            break;
        p = e + 1;
    }
}

int pyed_complete(const char *text, int len, int cur, int min_prefix, pyed_word_t *out, int max, int *start)
{
    int a = cur;
    while (a > 0 && ident_char(text[a - 1]))
        a--;
    if (a < cur && !ident_start(text[a]))
        return 0;   /* a number */
    *start = a;
    found_t f = {out, max, 0, text + a, cur - a};

    /* module.name */
    if (a > 0 && text[a - 1] == '.')
    {
        int m = a - 1;
        while (m > 0 && ident_char(text[m - 1]))
            m--;
        for (int i = 0; i < MODULE_COUNT; i++)
        {
            if ((int)strlen(s_modules[i][0]) == a - 1 - m && !memcmp(s_modules[i][0], text + m, (size_t)(a - 1 - m)))
            {
                add_list(&f, s_modules[i][1]);
                return f.count;
            }
        }
        if (f.plen < (min_prefix > 1 ? min_prefix : 1))
            return 0;
    }
    else if (f.plen < (min_prefix > 1 ? min_prefix : 1))
    {
        return 0;
    }
    else
    {
        add_list(&f, s_keywords);
        add_list(&f, s_builtins);
        for (int i = 0; i < MODULE_COUNT; i++)
            add(&f, s_modules[i][0], (int)strlen(s_modules[i][0]));
    }

    /* Words in the text, except the one being typed. */
    int i = 0;
    while (i < len && f.count < max)
    {
        if (!ident_start(text[i]) || (i > 0 && ident_char(text[i - 1])))
        {
            i++;
            continue;
        }
        int j = i;
        while (j < len && ident_char(text[j]))
            j++;
        if (i != a && j - i <= 40)
            add(&f, text + i, j - i);
        i = j;
    }
    return f.count;
}
