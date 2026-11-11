/*
 * store.c - the Store: lists the apps of a catalog (the tinydesk-apps
 * repository's catalog.txt) and installs, updates and removes them.
 *
 * Installing downloads NAME.tdapp into ~/Apps (as NAME.tdapp.part), checks
 * its size and SHA-256 against the catalog, then keeps it and adds it to
 * the user's Start menu. What is installed, and which unpack folder
 * (.tdapp-<hash>) each app uses, is kept in ~/Apps/.store, so Update and
 * Uninstall can clean up after a package. Apps the board cannot run (a newer
 * tdui API, more canvas cells than it has, a missing capability, native
 * code for another CPU) are listed but not installed. Downloads use the
 * port's td_fetch_ops_t; installing needs the Apps permission.
 *
 * With a drive such as the SD card (td_sysinfo()->drives) that the user may
 * use, "Install on the SD card" puts the package, and so its unpack folder,
 * in <drive>/TinyDesk/Apps/<user> instead, for room: the record in
 * ~/Apps/.store keeps where each package is ("sd" as a fourth field), and
 * the Start menu entry points at it. Without the card such an app does not
 * start; removing it then only forgets it.
 */
#include "td_apps.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STORE_URL   "https://tinydesk-project.github.io/tinydesk-apps/catalog.txt"
#define MAX_APPS    32
#define CATALOG_MAX (32 * 1024)
#define PACKAGE_MAX (16u * 1024u * 1024u)
#define VERIFY_STEP (16 * 1024)   /* bytes checked per tick */
#define RECORD_MAX  2048
#define TDUI_MAJOR  1             /* tdui.API, checked against tdui_contract.json */
#define TDUI_MINOR  0
#define WIN_W       78
#define WIN_H       21
#define LIST_W      30

typedef struct
{
    char id[32], title[41], version[16], summary[161], category[24];
    char file[40], sha[65], cache[24], needs[64], cpu[16];
    char author[41], repo[85], license[17];   /* shown only; 0.5.0 on */
    uint32_t size, cells;
    int api_major, api_minor;
} entry_t;

enum
{
    P_IDLE,
    P_CATALOG,
    P_PACKAGE,
    P_VERIFY
};

static td_window_t *s_win;
static td_widget_t *s_list, *s_title, *s_meta, *s_sum[4], *s_state, *s_install, *s_remove, *s_refresh;
static td_widget_t *s_progress, *s_note, *s_on_card, *s_by, *s_repo[2];
static entry_t *s_apps;
static int s_count, s_shown = -2, s_phase, s_busy_app = -1;
static char s_source[TD_PATH_MAX];
static uint32_t s_verified;
static char s_where[8];   /* where the package being installed goes: "sd" or "" */

/* ------------------------------------------------------------- SHA-256 */

typedef struct
{
    uint32_t h[8], len_lo, len_hi;
    uint8_t buf[64];
    uint32_t used;
} sha_t;

static const uint32_t K[64] = {
    0x428a2f98,
    0x71374491,
    0xb5c0fbcf,
    0xe9b5dba5,
    0x3956c25b,
    0x59f111f1,
    0x923f82a4,
    0xab1c5ed5,
    0xd807aa98,
    0x12835b01,
    0x243185be,
    0x550c7dc3,
    0x72be5d74,
    0x80deb1fe,
    0x9bdc06a7,
    0xc19bf174,
    0xe49b69c1,
    0xefbe4786,
    0x0fc19dc6,
    0x240ca1cc,
    0x2de92c6f,
    0x4a7484aa,
    0x5cb0a9dc,
    0x76f988da,
    0x983e5152,
    0xa831c66d,
    0xb00327c8,
    0xbf597fc7,
    0xc6e00bf3,
    0xd5a79147,
    0x06ca6351,
    0x14292967,
    0x27b70a85,
    0x2e1b2138,
    0x4d2c6dfc,
    0x53380d13,
    0x650a7354,
    0x766a0abb,
    0x81c2c92e,
    0x92722c85,
    0xa2bfe8a1,
    0xa81a664b,
    0xc24b8b70,
    0xc76c51a3,
    0xd192e819,
    0xd6990624,
    0xf40e3585,
    0x106aa070,
    0x19a4c116,
    0x1e376c08,
    0x2748774c,
    0x34b0bcb5,
    0x391c0cb3,
    0x4ed8aa4a,
    0x5b9cca4f,
    0x682e6ff3,
    0x748f82ee,
    0x78a5636f,
    0x84c87814,
    0x8cc70208,
    0x90befffa,
    0xa4506ceb,
    0xbef9a3f7,
    0xc67178f2,
};

#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static void sha_block(sha_t *s, const uint8_t *p)
{
    uint32_t w[64], a[8];
    for (int i = 0; i < 16; i++)
        w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 | (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
    for (int i = 16; i < 64; i++)
    {
        uint32_t s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    memcpy(a, s->h, sizeof(a));
    for (int i = 0; i < 64; i++)
    {
        uint32_t t1 = a[7] + (ROR(a[4], 6) ^ ROR(a[4], 11) ^ ROR(a[4], 25)) + ((a[4] & a[5]) ^ (~a[4] & a[6])) + K[i] + w[i];
        uint32_t t2 = (ROR(a[0], 2) ^ ROR(a[0], 13) ^ ROR(a[0], 22)) + ((a[0] & a[1]) ^ (a[0] & a[2]) ^ (a[1] & a[2]));
        memmove(a + 1, a, 7 * sizeof(uint32_t));
        a[4] += t1;
        a[0] = t1 + t2;
    }
    for (int i = 0; i < 8; i++)
        s->h[i] += a[i];
}

static void sha_init(sha_t *s)
{
    static const uint32_t h0[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                   0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    memset(s, 0, sizeof(*s));
    memcpy(s->h, h0, sizeof(h0));
}

static void sha_update(sha_t *s, const uint8_t *p, uint32_t n)
{
    uint32_t lo = s->len_lo;
    s->len_lo += n;
    if (s->len_lo < lo)
        s->len_hi++;
    while (n)
    {
        uint32_t take = 64 - s->used < n ? 64 - s->used : n;
        memcpy(s->buf + s->used, p, take);
        s->used += take;
        p += take;
        n -= take;
        if (s->used == 64)
        {
            sha_block(s, s->buf);
            s->used = 0;
        }
    }
}

static void sha_hex(sha_t *s, char out[65])
{
    uint32_t hi = s->len_hi << 3 | s->len_lo >> 29, lo = s->len_lo << 3;
    uint8_t pad = 0x80, zero = 0, len[8];
    sha_update(s, &pad, 1);
    while (s->used != 56)
        sha_update(s, &zero, 1);
    for (int i = 0; i < 4; i++)
    {
        len[i] = (uint8_t)(hi >> (24 - 8 * i));
        len[4 + i] = (uint8_t)(lo >> (24 - 8 * i));
    }
    sha_update(s, len, 8);
    for (int i = 0; i < 8; i++)
        snprintf(out + 8 * i, 9, "%08lx", (unsigned long)s->h[i]);
}

/* For the unit test: SHA-256 of a buffer as 64 hex digits. */
void td_store_sha256(const void *data, uint32_t len, char out[65])
{
    sha_t s;
    sha_init(&s);
    sha_update(&s, (const uint8_t *)data, len);
    sha_hex(&s, out);
}

/* --------------------------------------------------------- the catalog */

static const td_fs_ops_t *fs(void)
{
    return td_sysinfo()->fs;
}

static const td_fetch_ops_t *fetch(void)
{
    return td_sysinfo()->fetch;
}

static bool id_ok(const char *s)
{
    if (!s[0] || strlen(s) > 31)
        return false;
    for (; *s; s++)
        if (!((*s >= 'a' && *s <= 'z') || (*s >= '0' && *s <= '9') || *s == '_' || *s == '-'))
            return false;
    return true;
}

static bool hex_ok(const char *s, size_t n)
{
    if (strlen(s) != n)
        return false;
    for (; *s; s++)
        if (!((*s >= '0' && *s <= '9') || (*s >= 'a' && *s <= 'f')))
            return false;
    return true;
}

static bool entry_ok(const entry_t *e)
{
    char file[sizeof(e->file)];
    snprintf(file, sizeof(file), "%s.tdapp", e->id);
    return id_ok(e->id) && e->title[0] && e->version[0] && !strcmp(e->file, file) && hex_ok(e->sha, 64) &&
           !strncmp(e->cache, ".tdapp-", 7) && hex_ok(e->cache + 7, 16) && e->size > 0 && e->size <= PACKAGE_MAX;
}

#define COPY(field) snprintf(e->field, sizeof(e->field), "%s", value)

/* Parses catalog.txt (format 1, see tinydesk-apps' tools/make_index.py)
 * into list; returns the number of entries, or -1 if it is not one. */
int td_store_parse(char *text, void *list, int max)
{
    entry_t *apps = list, *e = NULL;
    int count = 0;
    bool first = true;
    for (char *line = text, *next; line && *line; line = next)
    {
        next = strchr(line, '\n');
        if (next)
            *next++ = 0;
        line[strcspn(line, "\r")] = 0;
        if (first)
        {
            if (strcmp(line, "TDSTORE 1") != 0)
                return -1;
            first = false;
            continue;
        }
        char *value = strchr(line, ' ');
        if (value)
            *value++ = 0;
        else
            value = line + strlen(line);
        if (!strcmp(line, "app"))
        {
            e = count < max ? &apps[count] : NULL;
            if (e)
            {
                memset(e, 0, sizeof(*e));
                COPY(id);
                e->api_major = 1;
                e->cells = 2000;
            }
        }
        else if (!strcmp(line, "end"))
        {
            if (e && entry_ok(e))
                count++;
            e = NULL;
        }
        else if (!e)
            continue;
        else if (!strcmp(line, "title"))
            COPY(title);
        else if (!strcmp(line, "version"))
            COPY(version);
        else if (!strcmp(line, "summary"))
            COPY(summary);
        else if (!strcmp(line, "category"))
            COPY(category);
        else if (!strcmp(line, "file"))
            COPY(file);
        else if (!strcmp(line, "sha256"))
            COPY(sha);
        else if (!strcmp(line, "cache"))
            COPY(cache);
        else if (!strcmp(line, "needs"))
            COPY(needs);
        else if (!strcmp(line, "cpu"))
            COPY(cpu);
        else if (!strcmp(line, "author"))
            COPY(author);
        else if (!strcmp(line, "repo"))
            COPY(repo);
        else if (!strcmp(line, "license"))
            COPY(license);
        else if (!strcmp(line, "size"))
            e->size = (uint32_t)strtoul(value, NULL, 10);
        else if (!strcmp(line, "cells"))
            e->cells = (uint32_t)strtoul(value, NULL, 10);
        else if (!strcmp(line, "api"))
        {
            char *dot;
            e->api_major = (int)strtol(value, &dot, 10);
            e->api_minor = *dot == '.' ? (int)strtol(dot + 1, NULL, 10) : 0;
        }
    }
    return first ? -1 : count;
}

int td_store_entry_size(void)
{
    return (int)sizeof(entry_t);
}

/* This board's native-code CPU, or "" (the PC). */
static const char *native_cpu(void)
{
    const char *chip = td_sysinfo()->chip ? td_sysinfo()->chip : "";
    if (strstr(chip, "ESP32-C"))
        return "rv32imc";
    return strstr(chip, "ESP32") ? "xtensawin" : "";
}

/* Why this board cannot run the app, or NULL. */
const char *td_store_cannot_run(const void *app)
{
    const entry_t *e = app;
    static char why[80];
    if (e->api_major != TDUI_MAJOR || e->api_minor > TDUI_MINOR)
    {
        snprintf(why, sizeof(why), "Needs a newer TinyDesk (tdui %d.%d).", e->api_major, e->api_minor);
        return why;
    }
    if (e->cells > TD_PYUI_MAX_CELLS)
        return "Needs a board with PSRAM (a larger canvas).";
    char needs[sizeof(e->needs)];
    snprintf(needs, sizeof(needs), "%s", e->needs);
    for (char *w = strtok(needs, " "); w; w = strtok(NULL, " "))
    {
        if (!strcmp(w, "gradient_fills"))
            continue;
        if (!strcmp(w, "rgb_pictures") && TD_PYUI_MAX_CELLS > 2000)
            continue;
        snprintf(why, sizeof(why), !strcmp(w, "rgb_pictures") ? "Needs a board with PSRAM (RGB pictures)." : "Needs a newer TinyDesk (%s).",
                 w);
        return why;
    }
    if (e->cpu[0] && strcmp(e->cpu, native_cpu()) != 0)
        return "Made for another CPU (native code).";
    return NULL;
}

/* ------------------------------------------- what is installed: ~/Apps/.store */

static void apps_path(char *out, size_t cap, const char *name)
{
    snprintf(out, cap, "%s/%s", td_apps_dir(), name);
}

/* The removable drive the user may keep apps on, or NULL. */
static const char *card(void)
{
    static td_drive_t d[4];
    int n = td_sysinfo()->drives ? td_sysinfo()->drives(d, 4) : 0;
    for (int i = 0; i < n; i++)
        if (d[i].removable && td_session_path_allowed(d[i].path, TD_FS_WRITE))
            return d[i].path;
    return NULL;
}

/* Where packages of a location go: "sd" (the card) or "" (~/Apps); false
 * when that is the card and it is not there. */
static bool package_dir(const char *where, char *out, size_t cap)
{
    if (strcmp(where, "sd") != 0)
    {
        snprintf(out, cap, "%s", td_apps_dir());
        return true;
    }
    const char *c = card();
    if (!c)
        return false;
    snprintf(out, cap, "%s/TinyDesk/Apps/%s", c, td_session_user());
    return true;
}

static void make_package_dir(const char *where)
{
    char dir[TD_PATH_MAX];
    if (strcmp(where, "sd") != 0 || !package_dir(where, dir, sizeof(dir)))
    {
        fs()->mkdir(td_apps_dir());
        return;
    }
    char step[TD_PATH_MAX];
    snprintf(step, sizeof(step), "%s/TinyDesk/Apps", card());
    fs()->mkdir(step);
    fs()->mkdir(dir);
}

/* The installed version of id ("" none), its unpack folder and where its
 * package is ("sd" or ""). */
static bool installed_at(const char *id, char *version, size_t vcap, char *cache, size_t ccap, char *where,
                         size_t wcap)
{
    char path[TD_PATH_MAX + 16], buf[RECORD_MAX];
    version[0] = 0;
    if (cache)
        cache[0] = 0;
    if (where)
        where[0] = 0;
    apps_path(path, sizeof(path), ".store");
    int n = fs() && fs()->read ? fs()->read(path, buf, (int)sizeof(buf) - 1) : -1;
    if (n <= 0)
        return false;
    buf[n] = 0;
    for (char *line = strtok(buf, "\n"); line; line = strtok(NULL, "\n"))
    {
        char lid[32], lver[16], lcache[24], lwhere[8] = "";
        if (sscanf(line, "%31s %15s %23s %7s", lid, lver, lcache, lwhere) >= 3 && !strcmp(lid, id))
        {
            snprintf(version, vcap, "%s", lver);
            if (cache)
                snprintf(cache, ccap, "%s", lcache);
            if (where)
                snprintf(where, wcap, "%s", lwhere);
            return true;
        }
    }
    return false;
}

static bool installed(const char *id, char *version, size_t vcap, char *cache, size_t ccap)
{
    return installed_at(id, version, vcap, cache, ccap, NULL, 0);
}

/* Writes id's line (version NULL: removes it). */
static bool record(const char *id, const char *version, const char *cache, const char *where)
{
    char path[TD_PATH_MAX + 16], buf[RECORD_MAX], out[RECORD_MAX];
    apps_path(path, sizeof(path), ".store");
    int n = fs()->read ? fs()->read(path, buf, (int)sizeof(buf) - 1) : -1;
    buf[n > 0 ? n : 0] = 0;
    int len = 0;
    for (char *line = strtok(buf, "\n"); line; line = strtok(NULL, "\n"))
    {
        char lid[32];
        if (sscanf(line, "%31s", lid) == 1 && strcmp(lid, id) != 0)
            len += snprintf(out + len, sizeof(out) - (size_t)len, "%s\n", line);
        if (len >= (int)sizeof(out) - 80)
            return false;
    }
    if (version)
        len += snprintf(out + len, sizeof(out) - (size_t)len, "%s %s %s%s%s\n", id, version, cache,
                        where[0] ? " " : "", where);
    return fs()->write(path, out, len) == 0;
}

/* a < b for x.y.z versions. */
static bool older(const char *a, const char *b)
{
    unsigned x[3] = {0}, y[3] = {0};
    sscanf(a, "%u.%u.%u", &x[0], &x[1], &x[2]);
    sscanf(b, "%u.%u.%u", &y[0], &y[1], &y[2]);
    for (int i = 0; i < 3; i++)
        if (x[i] != y[i])
            return x[i] < y[i];
    return false;
}

/* ------------------------------------------------------------- the window */

static void note(const char *fmt, const char *arg)
{
    if (s_note)
        td_widget_printf(s_note, fmt, arg ? arg : "");
}

static const char *item(td_widget_t *w, int index, int *fg, void *user)
{
    (void)w;
    (void)user;
    static char text[64];
    if (index < 0 || index >= s_count)
        return "";
    const entry_t *e = &s_apps[index];
    char version[16];
    installed(e->id, version, sizeof(version), NULL, 0);
    const char *mark = !version[0] ? "" : older(version, e->version) ? " (update)"
                                                                     : " (installed)";
    if (td_store_cannot_run(e))
        *fg = td_theme()->dim;
    int room = LIST_W - 3 - (int)strlen(mark);   /* the title's columns */
    char title[48];
    if (td_utf8_len(e->title) <= room)
        snprintf(title, sizeof(title), "%s", e->title);
    else
    {   /* cut, and say so: the details show all of it */
        td_utf8_copy(title, sizeof(title) - 4, e->title, room - 1);
        strcat(title, "\xE2\x80\xA6");
    }
    snprintf(text, sizeof(text), "%s%s", title, mark);
    return text;
}

/* The summary over up to four lines of the details column. */
static void wrap_summary(const char *text)
{
    const int width = WIN_W - LIST_W - 7;
    for (int i = 0; i < 4; i++)
    {
        while (*text == ' ')
            text++;
        int n = (int)strlen(text);
        if (n > width)
        {
            n = width;
            while (n > 0 && text[n] != ' ')
                n--;
            if (n == 0)
                n = width;
        }
        char line[80];
        snprintf(line, sizeof(line), "%.*s", n, text);
        td_widget_set_text(s_sum[i], line);
        text += n;
    }
}

/* Who made the app, its licence and where its source is: the repo address
 * over two lines. */
static void show_source(const entry_t *e)
{
    const int width = WIN_W - LIST_W - 7;
    char line[96];
    line[0] = 0;
    if (e && e->author[0])
        snprintf(line, sizeof(line), "By %s%s%s%s", e->author, e->license[0] ? " (" : "", e->license,
                 e->license[0] ? ")" : "");
    else if (e && e->license[0])
        snprintf(line, sizeof(line), "Licence: %s", e->license);
    td_widget_set_text(s_by, line);
    const char *repo = e ? e->repo : "";
    int n = (int)strlen(repo);
    snprintf(line, sizeof(line), "%.*s", n < width ? n : width, repo);
    td_widget_set_text(s_repo[0], line);
    snprintf(line, sizeof(line), "%.*s", width, n > width ? repo + width : "");
    td_widget_set_text(s_repo[1], line);
}

static void show_details(void)
{
    int i = td_list_selected(s_list);
    bool busy = s_phase != P_IDLE;
    if (i < 0 || i >= s_count)
    {
        td_widget_set_text(s_title, s_count ? "Choose an app on the left." : "");
        td_widget_set_text(s_meta, "");
        wrap_summary("");
        td_widget_set_text(s_state, "");
        show_source(NULL);
        td_widget_set_visible(s_install, false);
        td_widget_set_visible(s_remove, false);
        return;
    }
    const entry_t *e = &s_apps[i];
    char version[16], line[96];
    installed(e->id, version, sizeof(version), NULL, 0);
    snprintf(line, sizeof(line), "%s %s", e->title, e->version);
    td_widget_set_text(s_title, line);
    if (e->size)
        snprintf(line, sizeof(line), "%s, %lu KB", e->category[0] ? e->category : "App",
                 (unsigned long)((e->size + 1023) / 1024));
    else   /* installed, no catalog: the size is not known */
        snprintf(line, sizeof(line), "%s", e->category[0] ? e->category : "App");
    td_widget_set_text(s_meta, line);
    wrap_summary(e->summary);
    show_source(e);
    const char *why = td_store_cannot_run(e);
    if (why)
        td_widget_set_text(s_state, why);
    else if (!version[0])
        td_widget_set_text(s_state, "Not installed.");
    else
    {
        snprintf(line, sizeof(line), older(version, e->version) ? "Version %s is installed: an update is ready." : "Installed (%s).",
                 version);
        td_widget_set_text(s_state, line);
    }
    bool update = version[0] && older(version, e->version);
    td_widget_set_text(s_install, update ? "Update" : "Install");
    bool can_install = !busy && !why && (!version[0] || update);
    td_widget_set_visible(s_install, can_install);
    td_widget_set_visible(s_remove, !busy && version[0]);
    s_remove->rect.x = can_install ? s_install->rect.x + 14 : s_install->rect.x;   /* no gap */
    td_win_invalidate(s_win);
}

/* Without a catalog (offline), the installed apps from ~/Apps/.store, so
 * they can still be uninstalled. A package is always <id>.tdapp. */
static int list_installed(void)
{
    char path[TD_PATH_MAX + 16], buf[RECORD_MAX];
    apps_path(path, sizeof(path), ".store");
    int n = fs() && fs()->read ? fs()->read(path, buf, (int)sizeof(buf) - 1) : -1;
    if (n <= 0)
        return 0;
    buf[n] = 0;
    int count = 0;
    for (char *line = strtok(buf, "\n"); line && count < MAX_APPS; line = strtok(NULL, "\n"))
    {
        entry_t *e = &s_apps[count];
        memset(e, 0, sizeof(*e));
        if (sscanf(line, "%31s %15s %23s", e->id, e->version, e->cache) < 3)
            continue;
        snprintf(e->title, sizeof(e->title), "%s", e->id);
        snprintf(e->file, sizeof(e->file), "%s.tdapp", e->id);
        snprintf(e->summary, sizeof(e->summary), "Installed. The catalog could not be loaded.");
        e->api_major = TDUI_MAJOR;   /* unknown without the catalog: it ran here */
        count++;
    }
    return count;
}

static void start_catalog(void)
{
    char dest[TD_PATH_MAX + 16];
    if (!fetch())
    {
        note("Downloads are not supported here.%s", NULL);
        return;
    }
    if (!fs() || !fs()->mkdir || !fs()->read)
        return;
    fs()->mkdir(td_apps_dir());
    apps_path(dest, sizeof(dest), ".catalog.txt");
    const char *src = td_sysinfo()->store_url && td_sysinfo()->store_url[0] ? td_sysinfo()->store_url : STORE_URL;
    snprintf(s_source, sizeof(s_source), "%s", src);
    if (!fetch()->start(s_source, dest, CATALOG_MAX))
    {
        note("Another download is running; try again.%s", NULL);
        return;
    }
    s_phase = P_CATALOG;
    note("Loading the catalog...%s", NULL);
}

static int list_installed(void);

static void catalog_size(const char *name, bool is_dir, uint32_t size, void *user)
{
    if (!is_dir && !strcmp(name, ".catalog.txt"))
        *(uint32_t *)user = size;
}

static void catalog_loaded(void)
{
    char path[TD_PATH_MAX + 16];
    apps_path(path, sizeof(path), ".catalog.txt");
    /* Only as much memory as the file needs: a 32 KB block is often not to
     * be had on boards without PSRAM (the ESP32-C6). */
    uint32_t size = CATALOG_MAX;
    if (fs()->list && fs()->list(td_apps_dir(), catalog_size, &size) >= 0 && size > CATALOG_MAX)
        size = CATALOG_MAX;
    char *text = malloc(size + 1);
    int n = text ? fs()->read(path, text, (int)size) : -1;
    if (n > 0)
    {
        text[n] = 0;
        s_count = td_store_parse(text, s_apps, MAX_APPS);
    }
    free(text);
    if (n <= 0 || s_count < 0)
    {
        s_count = list_installed();
        note(s_count ? "No catalog (offline?): the installed apps can still be uninstalled.%s"
                     : "The catalog could not be read.%s",
             NULL);
    }
    else
        note(s_count ? "Choose an app.%s" : "The catalog has no apps.%s", NULL);
    td_list_set_count(s_list, s_count);
    td_list_select(s_list, s_count ? 0 : -1);
}

/* The package's address: the catalog's folder and its file name. */
static void package_url(char *out, size_t cap, const entry_t *e)
{
    const char *slash = strrchr(s_source, '/');
    int dir = slash ? (int)(slash - s_source + 1) : 0;
    snprintf(out, cap, "%.*s%s", dir, s_source, e->file);
}

static void on_install(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    int i = td_list_selected(s_list);
    if (s_phase != P_IDLE || i < 0 || i >= s_count)
        return;
    if (!td_session_has_right(TD_RIGHT_APPS))
    {
        note("Installing apps needs the Apps permission.%s", NULL);
        return;
    }
    if (!fs()->read_at || !fs()->rename || !fs()->remove)
    {
        note("This file system cannot hold apps.%s", NULL);
        return;
    }
    char url[TD_PATH_MAX + 64], dest[TD_PATH_MAX + 48], dir[TD_PATH_MAX];
    snprintf(s_where, sizeof(s_where), "%s", s_on_card && td_checkbox_get(s_on_card) && card() ? "sd" : "");
    package_dir(s_where, dir, sizeof(dir));
    make_package_dir(s_where);
    package_url(url, sizeof(url), &s_apps[i]);
    snprintf(dest, sizeof(dest), "%s/%s.part", dir, s_apps[i].file);
    if (!fetch()->start(url, dest, s_apps[i].size))
    {
        note("Another download is running; try again.%s", NULL);
        return;
    }
    s_busy_app = i;
    s_phase = P_PACKAGE;
    td_progress_set(s_progress, 0);
    note("Downloading %s...", s_apps[i].title);
    show_details();
}

static void remove_cache(const char *cache, const char *where)
{
    char path[TD_PATH_MAX + 32], dir[TD_PATH_MAX];
    if (strncmp(cache, ".tdapp-", 7) != 0 || !hex_ok(cache + 7, 16) || !package_dir(where, dir, sizeof(dir)))
        return;   /* only an unpack folder, nothing else */
    snprintf(path, sizeof(path), "%s/%s", dir, cache);
    if (fs()->exists(path))
        fs()->remove(path);
}

static void finish_install(void)
{
    const entry_t *e = &s_apps[s_busy_app];
    char part[TD_PATH_MAX + 48], final_[TD_PATH_MAX + 48], old_version[16], old_cache[24], old_where[8];
    char dir[TD_PATH_MAX];
    if (!package_dir(s_where, dir, sizeof(dir)))
    {
        note("The SD card was taken out; nothing was installed.%s", NULL);
        return;
    }
    snprintf(part, sizeof(part), "%s/%s.part", dir, e->file);
    snprintf(final_, sizeof(final_), "%s/%s", dir, e->file);
    installed_at(e->id, old_version, sizeof(old_version), old_cache, sizeof(old_cache), old_where, sizeof(old_where));
    if (fs()->exists(final_))
        fs()->remove(final_);
    if (fs()->rename(part, final_) != 0)
    {
        fs()->remove(part);
        note("The app could not be saved.%s", NULL);
        return;
    }
    if (old_version[0] && strcmp(old_where, s_where) != 0)
    {
        /* Moved between ~/Apps and the card: the old copy goes. */
        char old_dir[TD_PATH_MAX], old_pkg[TD_PATH_MAX + 48];
        if (package_dir(old_where, old_dir, sizeof(old_dir)))
        {
            snprintf(old_pkg, sizeof(old_pkg), "%s/%s", old_dir, e->file);
            if (td_in_start_menu(old_pkg))
                td_start_menu_remove(old_pkg);
            if (fs()->exists(old_pkg))
                fs()->remove(old_pkg);
        }
        remove_cache(old_cache, old_where);
    }
    else if (old_cache[0] && strcmp(old_cache, e->cache) != 0)
        remove_cache(old_cache, old_where);
    const char *err = record(e->id, e->version, e->cache, s_where) ? NULL : "Installed, but not recorded.";
    if (!err && !td_in_start_menu(final_))
        err = td_shortcut_make(td_start_dir(), final_);
    td_desktop_refresh();
    if (err)
        note("%s", err);
    else
        note(old_version[0] ? "%s is updated." : "%s is installed: find it in the Start menu.", e->title);
}

static sha_t s_sha;

static void verify_step(void)
{
    const entry_t *e = &s_apps[s_busy_app];
    char part[TD_PATH_MAX + 48], dir[TD_PATH_MAX];
    if (!package_dir(s_where, dir, sizeof(dir)))
    {
        s_phase = P_IDLE;
        s_busy_app = -1;
        note("The SD card was taken out; nothing was installed.%s", NULL);
        show_details();
        return;
    }
    snprintf(part, sizeof(part), "%s/%s.part", dir, e->file);
    char *buf = malloc(2048);
    bool failed = !buf;
    for (int done = 0; !failed && done < VERIFY_STEP && s_verified < e->size; done += 2048)
    {
        int n = fs()->read_at(part, s_verified, buf, 2048);
        if (n <= 0)
            failed = true;
        else
        {
            sha_update(&s_sha, (const uint8_t *)buf, (uint32_t)n);
            s_verified += (uint32_t)n;
        }
    }
    if (!failed && s_verified >= e->size && fs()->read_at(part, s_verified, buf, 1) != 0)
        failed = true;   /* longer than the catalog says */
    free(buf);
    td_progress_set(s_progress, (int)((uint64_t)s_verified * 100 / e->size));
    if (!failed && s_verified < e->size)
        return;
    s_phase = P_IDLE;
    char hex[65] = "";
    if (!failed)
        sha_hex(&s_sha, hex);
    if (failed || strcmp(hex, e->sha) != 0)
    {
        fs()->remove(part);
        note("The download of %s is damaged; nothing was installed.", e->title);
    }
    else
        finish_install();
    s_busy_app = -1;
    td_list_set_count(s_list, s_count);
    show_details();
}

static void removed(int button, void *user)
{
    int i = (int)(intptr_t)user;
    if (button != 0 || i < 0 || i >= s_count || !td_win_is_open(s_win))
        return;
    const entry_t *e = &s_apps[i];
    char path[TD_PATH_MAX + 48], version[16], cache[24], where[8], dir[TD_PATH_MAX];
    installed_at(e->id, version, sizeof(version), cache, sizeof(cache), where, sizeof(where));
    bool there = package_dir(where, dir, sizeof(dir));
    if (there)
    {
        snprintf(path, sizeof(path), "%s/%s", dir, e->file);
        if (td_in_start_menu(path))
            td_start_menu_remove(path);
        if (fs()->exists(path))
            fs()->remove(path);
        remove_cache(cache, where);
    }
    record(e->id, NULL, NULL, "");
    td_desktop_refresh();
    note(there ? "%s is uninstalled." : "%s is forgotten; its files stay on the SD card.", e->title);
    td_list_set_count(s_list, s_count);
    show_details();
}

static void on_remove(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    int i = td_list_selected(s_list);
    if (s_phase != P_IDLE || i < 0 || i >= s_count)
        return;
    if (!td_session_has_right(TD_RIGHT_APPS))
    {
        note("Uninstalling apps needs the Apps permission.%s", NULL);
        return;
    }
    static char text[96];
    snprintf(text, sizeof(text), "Uninstall %s? It goes from Apps and the Start menu.", s_apps[i].title);
    td_msgbox("Uninstall app", text, "Uninstall|Cancel", removed, (void *)(intptr_t)i);
}

static void on_refresh(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    if (s_phase == P_IDLE)
        start_catalog();
}

static void on_tick(td_window_t *win)
{
    (void)win;
    if (s_phase == P_CATALOG || s_phase == P_PACKAGE)
    {
        td_fetch_status_t st;
        fetch()->status(&st);
        if (st.state == TD_FETCH_RUNNING)
        {
            if (s_phase == P_PACKAGE && s_busy_app >= 0)
                td_progress_set(s_progress, (int)((uint64_t)st.done * 100 / s_apps[s_busy_app].size));
            return;
        }
        int phase = s_phase;
        s_phase = P_IDLE;
        if (st.state == TD_FETCH_FAILED)
        {
            note("%s", st.message[0] ? st.message : "The download failed.");
            s_busy_app = -1;
            if (phase == P_CATALOG && (s_count = list_installed()) > 0)
            {   /* offline: what is installed can still be uninstalled */
                note("No catalog (offline?): the installed apps can still be uninstalled.%s", NULL);
                td_list_set_count(s_list, s_count);
                td_list_select(s_list, 0);
            }
        }
        else if (phase == P_CATALOG)
            catalog_loaded();
        else
        {
            s_phase = P_VERIFY;
            s_verified = 0;
            sha_init(&s_sha);
            note("Checking %s...", s_apps[s_busy_app].title);
        }
        show_details();
        return;
    }
    if (s_phase == P_VERIFY)
    {
        verify_step();
        return;
    }
    int sel = td_list_selected(s_list);
    if (sel != s_shown)
    {
        s_shown = sel;
        show_details();
    }
}

static void on_draw(td_window_t *win, int w, int h)
{
    (void)win;
    const td_theme_t *t = td_theme();
    td_fill(td_rect(0, 0, w, h), ' ', t->win_fg, t->win_bg);
    for (int y = 1; y < h - 4; y++)
        td_text(LIST_W + 2, y, "\xE2\x94\x82", t->dim, t->win_bg, 0);   /* U+2502 */
}

static void on_close(td_window_t *win)
{
    (void)win;
    if (s_phase == P_CATALOG || s_phase == P_PACKAGE)
        fetch()->cancel();
    s_win = NULL;
    free(s_apps);
    s_apps = NULL;
    s_count = 0;
    s_phase = P_IDLE;
    s_busy_app = -1;
    s_shown = -2;
    s_list = s_title = s_meta = s_state = s_install = s_remove = s_refresh = s_progress = s_note = s_on_card = NULL;
    memset(s_sum, 0, sizeof(s_sum));
    s_by = s_repo[0] = s_repo[1] = NULL;
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    if (s_phase != P_IDLE)
        return;   /* closed while a download finishes */
    s_apps = calloc(MAX_APPS, sizeof(entry_t));
    if (!s_apps)
    {
        td_msgbox("Store", "Not enough memory to open the Store.", "OK", NULL, NULL);
        return;
    }
    td_window_desc_t d = {
        .title = "Store",
        .rect = td_rect(-1, -1, WIN_W, WIN_H),
        .flags = TD_WIN_MOVABLE | TD_WIN_CLOSABLE,
        .on_draw = on_draw,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = 100,
    };
    s_win = td_win_create(&d);
    if (!s_win)
    {
        free(s_apps);
        s_apps = NULL;
        return;
    }
    const int x = LIST_W + 4;
    s_list = td_list(s_win, td_rect(1, 1, LIST_W, WIN_H - 7), item, on_install, NULL);
    td_scrollbar(s_win, LIST_W + 1, 1, WIN_H - 7, s_list);
    s_title = td_label(s_win, x, 1, 0, "");
    s_meta = td_label(s_win, x, 2, 0, "");
    for (int i = 0; i < 4; i++)
        s_sum[i] = td_label(s_win, x, 4 + i, 0, "");
    s_state = td_label(s_win, x, 9, 0, "");
    s_install = td_button(s_win, x, 11, "Install", on_install, NULL);
    s_remove = td_button(s_win, x + 14, 11, "Uninstall", on_remove, NULL);
    s_by = td_label(s_win, x, 13, 0, "");
    s_repo[0] = td_label(s_win, x, 14, 0, "");
    s_repo[1] = td_label(s_win, x, 15, 0, "");
    s_progress = td_progress(s_win, 1, WIN_H - 5, WIN_W - 4);
    s_note = td_label(s_win, 1, WIN_H - 4, 0, "");
    s_refresh = td_button(s_win, 1, -1, "Refresh", on_refresh, NULL);
    if (card())   /* room for apps on the SD card */
        s_on_card = td_checkbox(s_win, 14, -1, "Install on the SD card", true, NULL, NULL);
    td_list_set_count(s_list, 0);
    show_details();
    start_catalog();
    td_widget_focus(s_list);
}

static const td_app_t s_app = {"Store", launch, "\xE2\x86\x93 "};   /* ↓ */

void td_store_register(void)
{
    td_app_register(&s_app);
}
