/*
 * test_trash.c - the Recycle Bin (apps/trash.c) on a real folder through
 * td_fs_stdio: delete and restore files and folders, names already taken,
 * folders that are gone, the size limit (oldest first), items too big,
 * the bin turned off, items deleted after their time, a user other than
 * root restoring only into their home, and emptying the bin.
 */
#include <stdio.h>
#include <stdlib.h>

#include "td_apps.h"
#include "td_fs_stdio.h"
#include "td_test.h"

#define ROOT "trash_test_fs"

static uint32_t s_now = 10000;
static int64_t s_utc = 1800000000;   /* 2027 */
static int fake_read(void *ctx)
{
    (void)ctx;
    return -1;
}
static int fake_write(void *ctx, const uint8_t *b, int n)
{
    (void)ctx;
    (void)b;
    return n;
}
static uint32_t fake_millis(void *ctx)
{
    (void)ctx;
    return s_now;
}
static void fake_sleep(void *ctx, uint32_t ms)
{
    (void)ctx;
    s_now += ms;
}
static const td_hal_t s_hal = {fake_read, fake_write, fake_millis, fake_sleep, NULL};

static bool clock_now(int64_t *utc)
{
    *utc = s_utc;
    return true;
}

static td_sysinfo_t s_info = {.time_now = clock_now};

static const td_fs_ops_t *fs(void)
{
    return s_info.fs;
}

static void make_file(const char *path, int bytes, char fill)
{
    static char data[400 * 1024];
    memset(data, fill, (size_t)bytes);
    CHECK(fs()->write(path, data, bytes) == 0);
}

static char first(const char *path)
{
    char c = 0;
    return fs()->read(path, &c, 1) == 1 ? c : 0;
}

static void count_entry(const char *name, bool is_dir, uint32_t size, void *user)
{
    (void)name;
    (void)size;
    if (is_dir)
        ++*(int *)user;
}

static int bin_items(void)
{
    int n = 0;
    char dir[TD_PATH_MAX];
    snprintf(dir, sizeof(dir), "%s/.trash", td_home_dir());
    if (fs()->exists(dir))
        fs()->list(dir, count_entry, &n);
    return n;
}

static void id_entry(const char *name, bool is_dir, uint32_t size, void *user)
{
    (void)size;
    if (is_dir)
        snprintf(user, 24, "%s", name);
}

/* The id of the (last listed) item in the bin. */
static const char *an_id(void)
{
    static char id[24];
    char dir[TD_PATH_MAX];
    id[0] = '\0';
    snprintf(dir, sizeof(dir), "%s/.trash", td_home_dir());
    fs()->list(dir, id_entry, id);
    return id;
}

static void path_in_home(char *buf, size_t cap, const char *rel)
{
    snprintf(buf, cap, "%s/%s", td_home_dir(), rel);
}

static void test_file(void)
{
    char a[TD_PATH_MAX], where[TD_PATH_MAX];
    path_in_home(a, sizeof(a), "a.txt");
    make_file(a, 100, 'A');
    CHECK_EQ(td_trash_put(a, false), TD_TRASH_OK);
    CHECK(!fs()->exists(a));
    CHECK_EQ(bin_items(), 1);
    CHECK(td_trash_restore(an_id(), where, sizeof(where)) == NULL);
    CHECK(strcmp(where, a) == 0);
    CHECK_EQ(first(a), 'A');
    CHECK_EQ(bin_items(), 0);

    /* The name is taken by now: it comes back as "a (2).txt". */
    CHECK_EQ(td_trash_put(a, false), TD_TRASH_OK);
    make_file(a, 10, 'N');
    CHECK(td_trash_restore(an_id(), where, sizeof(where)) == NULL);
    path_in_home(a, sizeof(a), "a (2).txt");
    CHECK(strcmp(where, a) == 0);
    CHECK_EQ(first(a), 'A');
}

static void test_folder(void)
{
    char sub[TD_PATH_MAX], deep[TD_PATH_MAX], f[TD_PATH_MAX], where[TD_PATH_MAX];
    path_in_home(sub, sizeof(sub), "sub");
    path_in_home(deep, sizeof(deep), "sub/deep");
    path_in_home(f, sizeof(f), "sub/deep/f.txt");
    CHECK(fs()->mkdir(sub) == 0);
    CHECK(fs()->mkdir(deep) == 0);
    make_file(f, 50, 'F');
    CHECK_EQ(td_trash_put(deep, true), TD_TRASH_OK);
    CHECK(!fs()->exists(deep));

    /* Its folder is gone too: restoring makes it again. */
    CHECK(fs()->remove(sub) == 0);
    CHECK(td_trash_restore(an_id(), where, sizeof(where)) == NULL);
    CHECK(strcmp(where, deep) == 0);
    CHECK_EQ(first(f), 'F');
}

static void test_limits(void)
{
    char p[TD_PATH_MAX];
    td_trash_prefs()->size = 2;   /* 256 KB */
    CHECK_EQ(td_trash_limit(), 256u * 1024u);

    path_in_home(p, sizeof(p), "big.bin");
    make_file(p, 300 * 1024, 'B');
    CHECK_EQ(td_trash_put(p, false), TD_TRASH_TOO_BIG);
    CHECK(fs()->exists(p));

    /* Three of 100 KB: the oldest goes so that the bin holds 256 KB. */
    const char *names[3] = {"one.bin", "two.bin", "three.bin"};
    for (int i = 0; i < 3; i++)
    {
        path_in_home(p, sizeof(p), names[i]);
        make_file(p, 100 * 1024, (char)('1' + i));
        s_utc += 10;
        CHECK_EQ(td_trash_put(p, false), TD_TRASH_OK);
    }
    CHECK_EQ(bin_items(), 2);
    char where[TD_PATH_MAX];
    while (bin_items() > 0)
        CHECK(td_trash_restore(an_id(), where, sizeof(where)) == NULL);
    path_in_home(p, sizeof(p), "one.bin");
    CHECK(!fs()->exists(p));   /* the oldest was deleted for good */
    path_in_home(p, sizeof(p), "three.bin");
    CHECK(fs()->exists(p));

    /* Off: nothing goes into the bin. */
    td_trash_prefs()->size = 1;
    CHECK_EQ(td_trash_limit(), 0u);
    CHECK_EQ(td_trash_put(p, false), TD_TRASH_OFF);
    td_trash_prefs()->size = 0;

    /* Kept a day: two days later it is gone. */
    td_trash_prefs()->keep = 1;
    CHECK_EQ(td_trash_put(p, false), TD_TRASH_OK);
    CHECK_EQ(bin_items(), 1);
    s_utc += 3600;
    td_trash_purge();
    CHECK_EQ(bin_items(), 1);
    s_utc += 2 * 86400;
    td_trash_purge();
    CHECK_EQ(bin_items(), 0);
    td_trash_prefs()->keep = 0;
}

static void test_other_user(void)
{
    char p[TD_PATH_MAX], info[TD_PATH_MAX * 2], where[TD_PATH_MAX];
    td_session_switch("bob", false);
    path_in_home(p, sizeof(p), "b.txt");
    make_file(p, 20, 'b');
    CHECK_EQ(td_trash_put(p, false), TD_TRASH_OK);

    /* A record changed to point outside the home: it comes back home. */
    snprintf(info, sizeof(info), "%s/.trash/%s/.tdtrash", td_home_dir(), an_id());
    const char *evil = "path=" ROOT "/etc/b.txt\ntime=1\nsize=20\ndir=0\n";
    CHECK(fs()->write(info, evil, (int)strlen(evil)) == 0);
    CHECK(td_trash_restore(an_id(), where, sizeof(where)) == NULL);
    CHECK(strcmp(where, p) == 0);
    CHECK(!fs()->exists(ROOT "/etc/b.txt"));

    CHECK_EQ(td_trash_put(p, false), TD_TRASH_OK);
    td_trash_empty();
    CHECK_EQ(bin_items(), 0);
    td_session_switch("root", false);
}

int main(void)
{
    s_info.fs = td_fs_stdio(ROOT);
    if (fs()->exists(ROOT))
        fs()->remove(ROOT);
    CHECK(fs()->mkdir(ROOT) == 0);
    td_set_sysinfo(&s_info);
    td_init(&s_hal);
    td_session_init();   /* root: ROOT/root */
    test_file();
    test_folder();
    test_limits();
    test_other_user();
    fs()->remove(ROOT);
    return TD_TEST_RESULT();
}
