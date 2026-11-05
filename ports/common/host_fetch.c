/*
 * host_fetch.c - downloads for the PC programs (td_fetch_ops_t): the Store
 * app's catalog and app packages.
 *
 * A file path (or file://) is copied; an http(s):// URL is handed to curl
 * (part of Windows 10 and later, and of most Linux systems), which checks
 * HTTPS certificates and follows redirects. Either runs in a thread, one
 * download at a time; a failed or cancelled one removes what it wrote.
 */
#include "host_fetch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#define NULL_DEVICE "NUL"
#else
#include <pthread.h>
#define NULL_DEVICE "/dev/null"
#endif

#define CHUNK 4096

static td_fetch_status_t s_st;
static char s_source[512], s_dest[512];
static uint32_t s_max;
static volatile int s_cancel, s_busy;

#ifdef _WIN32
static CRITICAL_SECTION s_lock;
static void lock(void)
{
    EnterCriticalSection(&s_lock);
}
static void unlock(void)
{
    LeaveCriticalSection(&s_lock);
}
#else
static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static void lock(void)
{
    pthread_mutex_lock(&s_lock);
}
static void unlock(void)
{
    pthread_mutex_unlock(&s_lock);
}
#endif

static void set(int state, uint32_t done, uint32_t total, const char *msg)
{
    lock();
    s_st.state = state;
    s_st.done = done;
    s_st.total = total;
    if (msg)
        snprintf(s_st.message, sizeof(s_st.message), "%s", msg);
    unlock();
}

static const char *copy_file(const char *path)
{
    FILE *in = fopen(path, "rb");
    if (!in)
        return "The source file does not exist.";
    FILE *out = fopen(s_dest, "wb");
    if (!out)
    {
        fclose(in);
        return "Cannot write the file.";
    }
    const char *err = NULL;
    char buf[CHUNK];
    uint32_t done = 0;
    size_t n;
    while (!err && (n = fread(buf, 1, sizeof(buf), in)) > 0)
    {
        done += (uint32_t)n;
        if (s_cancel)
            err = "Cancelled.";
        else if (s_max && done > s_max)
            err = "The download is larger than expected.";
        else if (fwrite(buf, 1, n, out) != n)
            err = "The disk is full.";
        else
            set(TD_FETCH_RUNNING, done, 0, NULL);
    }
    if (!err && ferror(in))
        err = "Cannot read the source file.";
    fclose(in);
    if (fclose(out) != 0 && !err)
        err = "The disk is full.";
    return err;
}

/* Characters a shell would treat specially even inside double quotes. */
static bool shell_safe(const char *s)
{
    for (; *s; s++)
        if (*s == '"' || *s == '`' || *s == '$' || *s == '%' || *s == '!' || *s == '\\' || (unsigned char)*s < 0x20)
            return false;
    return true;
}

static const char *run_curl(void)
{
    if (!shell_safe(s_source) || strchr(s_dest, '"'))
        return "The address has characters that are not allowed.";
    char cmd[1200], max[32] = "";
    if (s_max)
        snprintf(max, sizeof(max), "--max-filesize %lu ", (unsigned long)s_max);
    snprintf(cmd, sizeof(cmd), "curl -fsSL --proto =http,https --max-time 300 %s-o \"%s\" \"%s\" >" NULL_DEVICE " 2>&1", max,
             s_dest, s_source);
    int rc = system(cmd);
    if (s_cancel)
        return "Cancelled.";
    if (rc == 0)
        return NULL;
    return "The download failed (no network, or curl is missing).";
}

#ifdef _WIN32
static DWORD WINAPI worker(LPVOID arg)
#else
static void *worker(void *arg)
#endif
{
    (void)arg;
    const char *err;
    if (!strncmp(s_source, "http://", 7) || !strncmp(s_source, "https://", 8))
        err = run_curl();
    else
        err = copy_file(!strncmp(s_source, "file://", 7) ? s_source + 7 : s_source);
    uint32_t size = 0;
    if (err)
        remove(s_dest);
    else
    {
        FILE *f = fopen(s_dest, "rb");
        if (f)
        {
            fseek(f, 0, SEEK_END);
            size = (uint32_t)ftell(f);
            fclose(f);
        }
    }
    set(err ? TD_FETCH_FAILED : TD_FETCH_DONE, size, size, err ? err : "");
    s_busy = 0;
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

static bool fetch_start(const char *source, const char *dest, uint32_t max)
{
    if (s_busy || !source || !dest || strlen(source) >= sizeof(s_source) || strlen(dest) >= sizeof(s_dest))
        return false;
    snprintf(s_source, sizeof(s_source), "%s", source);
    snprintf(s_dest, sizeof(s_dest), "%s", dest);
    s_max = max;
    s_cancel = 0;
    s_busy = 1;
    set(TD_FETCH_RUNNING, 0, 0, "");
#ifdef _WIN32
    HANDLE t = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    bool ok = t != NULL;
    if (t)
        CloseHandle(t);
#else
    pthread_t t;
    bool ok = pthread_create(&t, NULL, worker, NULL) == 0;
    if (ok)
        pthread_detach(t);
#endif
    if (!ok)
    {
        s_busy = 0;
        set(TD_FETCH_FAILED, 0, 0, "Cannot start the download.");
    }
    return ok;
}

static void fetch_status(td_fetch_status_t *out)
{
    lock();
    *out = s_st;
    unlock();
}

static void fetch_cancel(void)
{
    s_cancel = 1;
}

static const td_fetch_ops_t s_ops = {fetch_start, fetch_status, fetch_cancel};

const td_fetch_ops_t *host_fetch_ops(void)
{
#ifdef _WIN32
    static bool ready;
    if (!ready)
    {
        InitializeCriticalSection(&s_lock);
        ready = true;
    }
#endif
    return &s_ops;
}
