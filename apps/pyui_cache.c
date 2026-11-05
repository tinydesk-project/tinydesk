/* Lossless run cache for already-rendered page artwork. One arena avoids
 * fragmented heaps and duplicate copies of identical panels/buttons. */
#include "pyui_cache.h"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

#define ENTRIES   256
#define MAX_ITEM  32768u
#define MAX_BYTES 131072u
typedef struct
{
    uint32_t generation, used, hash;
    size_t offset, bytes;
    int w, h;
} entry_t;
static entry_t *s_entries;
static uint8_t *s_arena;
static size_t s_capacity, s_used;
static uint32_t s_generation, s_clock;
static atomic_uint s_stats[6];

void td_pcache_close(void)
{
    free(s_entries);
    s_entries = NULL;
    s_arena = NULL;
    s_capacity = s_used = 0;
    /* Generation survives resets, so old picture handles cannot match. */
    for (int i = 0; i < 6; i++)
        atomic_store(&s_stats[i], 0);
}

void td_pcache_config(size_t bytes)
{
    td_pcache_close();
    if (bytes > MAX_BYTES)
        bytes = MAX_BYTES;
    if (bytes < 256)
        return;
    s_entries = calloc(1, ENTRIES * sizeof(*s_entries) + bytes);
    if (s_entries)
    {
        s_arena = (uint8_t *)(s_entries + ENTRIES);
        s_capacity = bytes;
        atomic_store(&s_stats[0], (unsigned)bytes);
    }
}

bool td_pcache_ready(void)
{
    return s_arena != NULL;
}
bool td_pcache_contains(td_pcache_ref_t ref, int w, int h)
{
    if (!s_entries || !ref.generation || ref.index >= ENTRIES)
        return false;
    const entry_t *e = &s_entries[ref.index];
    return e->generation == ref.generation && e->w == w && e->h == h;
}

bool td_pcache_load(td_pcache_ref_t ref, uint16_t *pixels, int w, int h)
{
    if (!td_pcache_contains(ref, w, h))
    {
        atomic_fetch_add(&s_stats[3], 1);
        return false;
    }
    entry_t *e = &s_entries[ref.index];
    e->used = ++s_clock;
    const uint16_t *runs = (const uint16_t *)(s_arena + e->offset);
    size_t at = 0;
    for (size_t i = 0; i < e->bytes / sizeof(uint16_t); i += 2)
    {
        unsigned count = runs[i];
        uint16_t value = runs[i + 1];
        for (unsigned j = 0; j < count; j++)
            pixels[at++] = value;
    }
    atomic_fetch_add(&s_stats[2], 1);
    return true;
}

bool td_pcache_view(td_pcache_ref_t ref, int w, int h, const uint16_t **runs, size_t *words)
{
    if (!td_pcache_contains(ref, w, h))
        return false;
    entry_t *e = &s_entries[ref.index];
    e->used = ++s_clock;
    *runs = (const uint16_t *)(s_arena + e->offset);
    *words = e->bytes / sizeof(uint16_t);
    atomic_fetch_add(&s_stats[2], 1);
    return true;
}

static bool same_pixels(const entry_t *e, const uint16_t *pixels)
{
    const uint16_t *runs = (const uint16_t *)(s_arena + e->offset);
    size_t at = 0;
    for (size_t i = 0; i < e->bytes / sizeof(uint16_t); i += 2)
        for (unsigned j = 0; j < runs[i]; j++)
            if (pixels[at++] != runs[i + 1])
                return false;
    return true;
}

static void evict(int slot)
{
    entry_t *e = &s_entries[slot];
    size_t end = e->offset + e->bytes;
    memmove(s_arena + e->offset, s_arena + end, s_used - end);
    for (int i = 0; i < ENTRIES; i++)
        if (s_entries[i].generation && s_entries[i].offset > e->offset)
            s_entries[i].offset -= e->bytes;
    s_used -= e->bytes;
    e->generation = 0;
    atomic_fetch_add(&s_stats[4], 1);
}

bool td_pcache_store(td_pcache_ref_t *ref, const uint16_t *pixels, int w, int h)
{
    if (!s_arena || w < 1 || h < 1)
        return false;
    size_t total = (size_t)w * h, bytes = 0;
    uint32_t hash = 2166136261u;
    for (size_t at = 0; at < total;)
    {
        uint16_t value = pixels[at];
        unsigned count = 1;
        while (at + count < total && count < 65535 && pixels[at + count] == value)
            count++;
        bytes += 4;
        if (bytes > MAX_ITEM || bytes >= total * 2 || bytes > s_capacity)
            return false;
        hash = (hash ^ value) * 16777619u;
        hash = (hash ^ count) * 16777619u;
        at += count;
    }
    /* Check the actual pixels after the hash: a collision must never show
     * another control. Multiple pages can share one identical run stream. */
    for (int i = 0; i < ENTRIES; i++)
    {
        entry_t *e = &s_entries[i];
        if (e->generation && e->hash == hash && e->w == w && e->h == h && e->bytes == bytes && same_pixels(e, pixels))
        {
            e->used = ++s_clock;
            *ref = (td_pcache_ref_t){e->generation, (uint16_t)i};
            return true;
        }
    }
    int slot = -1;
    for (;;)
    {
        for (int i = 0; i < ENTRIES; i++)
            if (!s_entries[i].generation)
            {
                slot = i;
                break;
            }
        if (slot >= 0 && bytes <= s_capacity - s_used)
            break;
        int oldest = -1;
        for (int i = 0; i < ENTRIES; i++)
            if (s_entries[i].generation && (oldest < 0 || (int32_t)(s_entries[i].used - s_entries[oldest].used) < 0))
                oldest = i;
        if (oldest < 0)
            return false;
        evict(oldest);
    }
    uint16_t *runs = (uint16_t *)(s_arena + s_used);
    size_t n = 0;
    for (size_t at = 0; at < total;)
    {
        uint16_t value = pixels[at];
        unsigned count = 1;
        while (at + count < total && count < 65535 && pixels[at + count] == value)
            count++;
        runs[n++] = (uint16_t)count;
        runs[n++] = value;
        at += count;
    }
    if (!++s_generation)
        ++s_generation;
    s_entries[slot] = (entry_t){s_generation, ++s_clock, hash, s_used, bytes, w, h};
    s_used += bytes;
    atomic_store(&s_stats[1], (unsigned)s_used);
    *ref = (td_pcache_ref_t){s_generation, (uint16_t)slot};
    return true;
}

void td_pcache_warmed(void)
{
    atomic_fetch_add(&s_stats[5], 1);
}
void td_pcache_info(uint32_t out[6])
{
    for (int i = 0; i < 6; i++)
        out[i] = atomic_load(&s_stats[i]);
}
