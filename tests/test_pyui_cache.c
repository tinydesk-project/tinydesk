#include "td_test.h"
#include "pyui_cache.h"
#include <stdlib.h>
#include <string.h>

int main(void)
{
    td_pcache_ref_t a = {0}, b = {0};
    uint32_t stats[6];
    uint16_t pixels[128], decoded[128];
    for (int i = 0; i < 128; i++)
        pixels[i] = i < 64 ? 0x0969 : 0x17bf;
    td_pcache_config(256);
    CHECK(td_pcache_store(&a, pixels, 16, 8));
    td_pcache_info(stats);
    CHECK_EQ(stats[1], 8);
    CHECK(td_pcache_store(&b, pixels, 16, 8));
    CHECK_EQ(a.generation, b.generation);
    CHECK_EQ(a.index, b.index);
    td_pcache_info(stats);
    CHECK_EQ(stats[1], 8);
    CHECK(td_pcache_load(b, decoded, 16, 8));
    CHECK(!memcmp(pixels, decoded, sizeof(pixels)));
    const uint16_t *runs = NULL;
    size_t words = 0;
    CHECK(td_pcache_view(b, 16, 8, &runs, &words));
    CHECK(words == 4);
    CHECK(runs[0] == 64 && runs[1] == 0x0969 && runs[2] == 64 && runs[3] == 0x17bf);
    CHECK(!td_pcache_view(b, 8, 16, &runs, &words));
    CHECK(!td_pcache_contains(b, 8, 16));
    for (int update = 0; update < 100; update++)
    {
        for (int i = 0; i < 128; i++)
            pixels[i] = (uint16_t)(i < 64 ? update + 1 : update + 1000);
        td_pcache_ref_t next = {0};
        CHECK(td_pcache_store(&next, pixels, 16, 8));
        CHECK(td_pcache_load(next, decoded, 16, 8));
        CHECK(!memcmp(pixels, decoded, sizeof(pixels)));
        td_pcache_info(stats);
        CHECK(stats[1] <= 256);
    }
    CHECK(!td_pcache_load(a, decoded, 16, 8));
    td_pcache_info(stats);
    CHECK(stats[4] > 0);
    CHECK(!td_pcache_view(a, 16, 8, &runs, &words));
    td_pcache_config(256);
    CHECK(!td_pcache_contains(b, 16, 8));
    for (int i = 0; i < 128; i++)
        pixels[i] = (uint16_t)i;
    a = (td_pcache_ref_t){0};
    CHECK(!td_pcache_store(&a, pixels, 16, 8));
    td_pcache_info(stats);
    CHECK_EQ(stats[1], 0);

    /* Runs longer than uint16_t are split without changing their pixels. */
    uint16_t *large = malloc(70000 * sizeof(*large)), *copy = malloc(70000 * sizeof(*copy));
    CHECK(large && copy);
    for (int i = 0; i < 70000; i++)
        large[i] = 0x1234;
    CHECK(td_pcache_store(&a, large, 700, 100));
    CHECK(td_pcache_load(a, copy, 700, 100));
    CHECK(!memcmp(large, copy, 70000 * sizeof(*copy)));
    td_pcache_config(1000000);
    td_pcache_info(stats);
    CHECK_EQ(stats[0], 131072);
    td_pcache_config(0);
    CHECK(!td_pcache_ready());
    CHECK(!td_pcache_store(&a, large, 700, 100));
    free(large);
    free(copy);
    td_pcache_close();
    return TD_TEST_RESULT();
}
