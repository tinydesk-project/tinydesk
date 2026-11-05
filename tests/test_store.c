/*
 * test_store.c - the Store's catalog (apps/store.c): parsing catalog.txt
 * format 1, which entries are refused, what a board cannot run, and the
 * SHA-256 that checks downloads.
 */
#include "td_apps.h"
#include "td_test.h"

#include <stdlib.h>
#include <string.h>

#define SHA_A "9cad5b5974c82102e03fc83190bb2601658e80e6b5fcf33cf7fc10a465d17eaa"

static const char *s_catalog = "TDSTORE 1\n"
                               "app device_monitor\n"
                               "title Device monitor\n"
                               "version 1.0.0\n"
                               "summary A form.\n"
                               "category Examples\n"
                               "file device_monitor.tdapp\n"
                               "size 10022\n"
                               "sha256 " SHA_A "\n"
                               "cache .tdapp-2c11cd0ada8203de\n"
                               "api 1.0\n"
                               "cells 2000\n"
                               "future_key something new\r\n"   /* unknown keys and CRLF are fine */
                               "end\n"
                               "app big_panel\n"
                               "title Big panel\n"
                               "version 2.1.0\n"
                               "file big_panel.tdapp\n"
                               "size 9152\n"
                               "sha256 " SHA_A "\n"
                               "cache .tdapp-82bea64d62820a22\n"
                               "cells 32000\n"
                               "end\n"
                               "app newer_api\n"
                               "title Newer\n"
                               "version 1.0.0\n"
                               "file newer_api.tdapp\n"
                               "size 1\n"
                               "sha256 " SHA_A "\n"
                               "cache .tdapp-0000000000000000\n"
                               "api 1.7\n"
                               "end\n"
                               "app native_xtensa\n"
                               "title Native\n"
                               "version 1.0.0\n"
                               "file native_xtensa.tdapp\n"
                               "size 1\n"
                               "sha256 " SHA_A "\n"
                               "cache .tdapp-0000000000000000\n"
                               "needs gradient_fills\n"
                               "cpu xtensawin\n"
                               "end\n"
                               /* refused: a path in file, a bad id, a short hash, an odd cache */
                               "app evil\ntitle E\nversion 1.0.0\nfile ../evil.tdapp\nsize 1\nsha256 " SHA_A
                               "\ncache .tdapp-0000000000000000\nend\n"
                               "app Bad.Id\ntitle B\nversion 1.0.0\nfile Bad.Id.tdapp\nsize 1\nsha256 " SHA_A
                               "\ncache .tdapp-0000000000000000\nend\n"
                               "app short\ntitle S\nversion 1.0.0\nfile short.tdapp\nsize 1\nsha256 abc\n"
                               "cache .tdapp-0000000000000000\nend\n"
                               "app cache\ntitle C\nversion 1.0.0\nfile cache.tdapp\nsize 1\nsha256 " SHA_A
                               "\ncache ../../etc\nend\n"
                               "app unfinished\ntitle U\n";

static td_sysinfo_t s_info;

static const void *at(const char *list, int i)
{
    return list + (size_t)i * (size_t)td_store_entry_size();
}

static void test_parse(void)
{
    char *text = malloc(strlen(s_catalog) + 1);
    char *list = calloc(8, (size_t)td_store_entry_size());
    strcpy(text, s_catalog);
    CHECK_EQ(td_store_parse(text, list, 8), 4);

    /* What each board can run. The PC (no native CPU) builds of this test
     * have the default 2000 cells unless built with more. */
    s_info.chip = "host CPU";
    td_set_sysinfo(&s_info);
    CHECK(td_store_cannot_run(at(list, 0)) == NULL);
    CHECK((td_store_cannot_run(at(list, 1)) == NULL) == (TD_PYUI_MAX_CELLS >= 32000));
    CHECK(strstr(td_store_cannot_run(at(list, 2)), "newer TinyDesk") != NULL);
    CHECK(strstr(td_store_cannot_run(at(list, 3)), "another CPU") != NULL);
    s_info.chip = "ESP32-S3 rev 0.2, 2 cores";
    CHECK(td_store_cannot_run(at(list, 3)) == NULL);
    s_info.chip = "ESP32-C6 rev 0.1, 1 core";
    CHECK(td_store_cannot_run(at(list, 3)) != NULL);

    /* Room for fewer entries: the rest are skipped. */
    strcpy(text, s_catalog);
    CHECK_EQ(td_store_parse(text, list, 2), 2);
    /* Not a catalog. */
    char other[] = "<html>not found</html>\n";
    CHECK_EQ(td_store_parse(other, list, 8), -1);
    char empty[] = "";
    CHECK_EQ(td_store_parse(empty, list, 8), -1);
    char none[] = "TDSTORE 1\n";
    CHECK_EQ(td_store_parse(none, list, 8), 0);
    free(text);
    free(list);
}

static void test_sha256(void)
{
    char hex[65];
    td_store_sha256("", 0, hex);
    CHECK(!strcmp(hex, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    td_store_sha256("abc", 3, hex);
    CHECK(!strcmp(hex, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    const char *two = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";   /* 56 bytes: two blocks */
    td_store_sha256(two, (uint32_t)strlen(two), hex);
    CHECK(!strcmp(hex, "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
    static char million[1000000];
    memset(million, 'a', sizeof(million));
    td_store_sha256(million, sizeof(million), hex);
    CHECK(!strcmp(hex, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));
}

int main(void)
{
    test_parse();
    test_sha256();
    return TD_TEST_RESULT();
}
