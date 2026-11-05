#ifndef TD_PYUI_CACHE_H
#define TD_PYUI_CACHE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint32_t generation;
    uint16_t index;
} td_pcache_ref_t;
/* UI-thread owned, fixed-size arena. Entries hold lossless RGB565 runs. */
void td_pcache_config(size_t bytes);
void td_pcache_close(void);
bool td_pcache_ready(void);
bool td_pcache_contains(td_pcache_ref_t ref, int w, int h);
bool td_pcache_load(td_pcache_ref_t ref, uint16_t *pixels, int w, int h);
/* Borrowed until the next store/config/close on the owning UI thread.
 * Pairs are uint16 run length, RGB565 color; no decoded heap block. */
bool td_pcache_view(td_pcache_ref_t ref, int w, int h, const uint16_t **runs, size_t *words);
bool td_pcache_store(td_pcache_ref_t *ref, const uint16_t *pixels, int w, int h);
void td_pcache_warmed(void);
/* capacity, occupied bytes, hits, misses, evictions, warmed entries. */
void td_pcache_info(uint32_t out[6]);
#endif
