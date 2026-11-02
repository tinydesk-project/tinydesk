/*
 * test_desktop.c - a new Desktop folder: Welcome.txt and the port's own
 * files (td_sysinfo_t.desktop_files), written once per user. The
 * filesystem is a list of paths in memory.
 */
#include "td_test.h"
#include "td_apps.h"

static uint32_t s_now = 10000;
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

  /* ------------------------------------------- directories and files */

#define MAX_PATHS 32

static struct
{
    char path[TD_PATH_MAX];
    char data[1024];
    bool is_dir;
} s_paths[MAX_PATHS];
static int s_count;
static int s_writes;

static int find(const char *path)
{
    for (int i = 0; i < s_count; i++)
        if (strcmp(s_paths[i].path, path) == 0)
            return i;
    return -1;
}

static int add(const char *path, bool is_dir)
{
    int i = find(path);
    if (i < 0 && s_count < MAX_PATHS)
    {
        i = s_count++;
        snprintf(s_paths[i].path, sizeof(s_paths[i].path), "%s", path);
    }
    if (i >= 0)
        s_paths[i].is_dir = is_dir;
    return i;
}

static int fs_mkdir(const char *path)
{
    if (find(path) >= 0)
        return -1;
    return add(path, true) >= 0 ? 0 : -1;
}

static int fs_write(const char *path, const char *data, int len)
{
    int i = add(path, false);
    if (i < 0 || len >= (int)sizeof(s_paths[i].data))
        return -1;
    memcpy(s_paths[i].data, data, (size_t)len);
    s_paths[i].data[len] = '\0';
    s_writes++;
    return 0;
}

static int fs_exists(const char *path)
{
    return find(path) >= 0;
}

static int fs_list(const char *dir, void (*fn)(const char *name, bool is_dir, uint32_t size, void *user),
                   void *user)
{
    size_t n = strlen(dir);
    int count = 0;
    for (int i = 0; i < s_count; i++)
    {
        const char *p = s_paths[i].path;
        if (strncmp(p, dir, n) == 0 && p[n] == '/' && !strchr(p + n + 1, '/'))
        {
            fn(p + n + 1, s_paths[i].is_dir, (uint32_t)strlen(s_paths[i].data), user);
            count++;
        }
    }
    return count;
}

static int fs_read(const char *path, char *buf, int cap)
{
    int i = find(path);
    if (i < 0)
        return -1;
    int n = (int)strlen(s_paths[i].data);
    n = n < cap ? n : cap;
    memcpy(buf, s_paths[i].data, (size_t)n);
    return n;
}

static const td_fs_ops_t s_fs = {.root = "/fs",
                                 .list = fs_list,
                                 .read = fs_read,
                                 .mkdir = fs_mkdir,
                                 .write = fs_write,
                                 .exists = fs_exists};

static const td_desktop_file_t s_files[] = {
    {"Credits.txt", "Made by the TinyTest team.\n"},
    {"Readme.md", "# Board notes\n"},
    {"../escape.txt", "outside the Desktop\n"},
    {"sub/dir.txt", "not a plain name\n"},
    {"", "no name\n"},
    {"NoText.txt", NULL},
    {NULL, NULL},
};
static td_sysinfo_t s_info = {.fs = &s_fs, .desktop_files = s_files};

static const char *text_of(const char *path)
{
    int i = find(path);
    return i >= 0 ? s_paths[i].data : NULL;
}

  /* ------------------------------------------------------------- tests */

static void test_new_desktop_gets_the_port_files(void)
{
    td_session_init();    /* root: /fs/root/Desktop */
    CHECK(text_of("/fs/root/Desktop/Welcome.txt") != NULL);
    CHECK(text_of("/fs/root/Desktop/Credits.txt") != NULL &&
          strcmp(text_of("/fs/root/Desktop/Credits.txt"), "Made by the TinyTest team.\n") == 0);
    CHECK(text_of("/fs/root/Desktop/Readme.md") != NULL &&
          strcmp(text_of("/fs/root/Desktop/Readme.md"), "# Board notes\n") == 0);
    CHECK(s_writes == 3);   /* the bad entries were skipped */
    CHECK(find("/fs/root/escape.txt") < 0 && find("/fs/root/Desktop/../escape.txt") < 0);
    CHECK(find("/fs/root/Desktop/sub/dir.txt") < 0);
    CHECK(find("/fs/root/Desktop/NoText.txt") < 0);
}

static void test_written_once_per_user(void)
{
    s_writes = 0;
    td_session_switch("bob", false);
    CHECK(text_of("/fs/home/bob/Desktop/Welcome.txt") != NULL);
    CHECK(text_of("/fs/home/bob/Desktop/Credits.txt") != NULL);
    CHECK(s_writes == 3);

      /* Back to root, whose Desktop exists: nothing is written again, and a
     * file the user changed stays as it is. */
    fs_write("/fs/root/Desktop/Credits.txt", "mine\n", 5);
    s_writes = 0;
    td_session_switch("root", false);
    CHECK(s_writes == 0);
    CHECK(strcmp(text_of("/fs/root/Desktop/Credits.txt"), "mine\n") == 0);
}

static void test_no_port_files(void)
{
    s_info.desktop_files = NULL;
    s_writes = 0;
    td_session_switch("eve", false);
    CHECK(text_of("/fs/home/eve/Desktop/Welcome.txt") != NULL);
    CHECK(find("/fs/home/eve/Desktop/Credits.txt") < 0);
    CHECK(s_writes == 1);
}

int main(void)
{
    td_set_sysinfo(&s_info);
    td_init(&s_hal);   /* no size answer: 80x25 */
    test_new_desktop_gets_the_port_files();
    test_written_once_per_user();
    test_no_port_files();
    return TD_TEST_RESULT();
}
