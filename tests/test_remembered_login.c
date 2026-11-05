#include "td_test.h"
#include "td_apps.h"
static uint32_t now = 1000;
static bool timeout;
static int read_byte(void *p)
{
    (void)p;
    return -1;
}
static int write_bytes(void *p, const uint8_t *b, int n)
{
    (void)p;
    (void)b;
    return n;
}
static uint32_t millis(void *p)
{
    (void)p;
    return now;
}
static void sleep_ms(void *p, uint32_t ms)
{
    (void)p;
    now += ms;
}
static bool exists(const char *name)
{
    return !strcmp(name, "root");
}
static bool authenticate(const char *name, const char *password)
{
    (void)name;
    (void)password;
    return false;
}
static uint32_t generation(const char *name)
{
    return exists(name) ? 1 : 0;
}
static uint32_t remembered(char *name, int cap)
{
    snprintf(name, (size_t)cap, "root");
    return 1;
}
static const char *backend_user(void *p)
{
    (void)p;
    return timeout ? "" : "root";
}
static void set_user(void *p, const char *name)
{
    (void)p;
    (void)name;
}
int main(int argc, char **argv)
{
    timeout = argc > 1 && !strcmp(argv[1], "timeout");
    static const td_hal_t hal = {read_byte, write_bytes, millis, sleep_ms, NULL};
    static const td_user_ops_t users = {.remembered = remembered, .generation = generation};
    static const td_sysinfo_t info = {.user_exists = exists, .authenticate = authenticate, .users = &users};
    static const td_term_backend_t backend = {.user = backend_user, .set_user = set_user};
    td_set_sysinfo(&info);
    td_init(&hal);
    td_session_init();
    CHECK(td_session_locked());
    CHECK(td_session_auth_pending());
    CHECK(td_win_focused() == NULL);
    static td_buffer_t back;
    td_wm_compose(&back);
    CHECK_EQ(td_buffer_cell(&back, 0, 0)->bg, 0);
    td_terminal_set_backend(&backend);
    now += timeout ? 11000 : 200;
    td_step();
    if (timeout)
    {
        CHECK(td_session_locked());
        CHECK(!td_session_auth_pending());
        CHECK(td_win_focused() != NULL);
        CHECK(!strcmp(td_win_focused()->title, "Sign in"));
    }
    else
    {
        CHECK(!td_session_locked());
        CHECK(td_session_is_root());
        CHECK(td_win_focused() == NULL);
    }
    return TD_TEST_RESULT();
}
