/* Native app regression at the CrowPanel cell size, with fake device services.
 * No network, GPIO, credentials or real filesystem are used. */
#include "td_test.h"
#include "td_apps.h"

static uint32_t now = 1000;
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
static const td_hal_t hal = {read_byte, write_bytes, millis, sleep_ms, NULL};
static uint32_t ram(void)
{
    return 128 * 1024;
}
static uint32_t total_ram(void)
{
    return 256 * 1024;
}
static uint32_t psram(void)
{
    return 8 * 1024 * 1024;
}
static int cpu(void)
{
    return 240;
}
static int tasks(td_task_info_t *out, int max)
{
    int n = max < 12 ? max : 12;
    for (int i = 0; i < n; i++)
    {
        out[i] = (td_task_info_t){.priority = 5, .core = i % 2, .state = 'B', .cpu_tenths = 10, .stack_free = UINT32_MAX};
        snprintf(out[i].name, sizeof(out[i].name), "worker-%d", i);
    }
    return n;
}
static bool compiled_files;
static int fs_list(const char *d, void (*fn)(const char *, bool, uint32_t, void *), void *u)
{
    (void)d;
    if (!compiled_files)
        return 0;
    fn("test.mpy", false, 40, u);
    fn("test.tdapp", false, 100, u);
    return 2;
}
static int fs_exists(const char *p)
{
    (void)p;
    return 0;
}
static int fs_read(const char *p, char *b, int n)
{
    (void)p;
    (void)b;
    (void)n;
    return -1;
}
static int users(int i, td_user_info_t *out)
{
    if (i)
        return -1;
    *out = (td_user_info_t){.role = TD_USER_ROLE_ROOT, .rights = TD_RIGHT_ALL};
    strcpy(out->name, "root");
    return 0;
}
static bool device_status(char *b, int n)
{
    snprintf(b, n, "Test device");
    return true;
}
static void mouse_get(td_mouse_settings_t *s)
{
    *s = (td_mouse_settings_t){100, 20, 20};
}
static void keyboard_get(td_keyboard_settings_t *s)
{
    *s = (td_keyboard_settings_t){0, 400, 20};
}
static const char *layout_name(int i)
{
    return i ? NULL : "US (English)";
}
static void net_status(td_net_status_t *s)
{
    memset(s, 0, sizeof(*s));
}
static bool scan(void)
{
    return true;
}
static int aps(td_wifi_ap_t *out, int max)
{
    (void)out;
    (void)max;
    return 0;
}
static bool server_status(int i, int *port, int *clients)
{
    (void)i;
    if (port)
        *port = 0;
    if (clients)
        *clients = 0;
    return false;
}
static bool telnet_get(void)
{
    return false;
}
static void telnet_set(bool enabled)
{
    (void)enabled;
}
static const char *telnet_peer(void)
{
    return NULL;
}
static void ota_info(td_ota_info_t *i)
{
    memset(i, 0, sizeof(*i));
    strcpy(i->version, "test");
}
static void ota_status(td_ota_status_t *s)
{
    memset(s, 0, sizeof(*s));
}
static const td_fs_ops_t fs = {.root = "/fs", .list = fs_list, .exists = fs_exists, .read = fs_read};
static const td_user_ops_t user_ops = {.list = users};
static const td_mouse_ops_t mouse_ops = {.get = mouse_get, .status = device_status};
static const td_keyboard_ops_t keyboard_ops = {.get = keyboard_get, .status = device_status, .layout_name = layout_name};
static const td_net_ops_t net = {.status = net_status, .scan = scan, .scan_results = aps, .server_status = server_status, .telnet_enabled = telnet_get, .telnet_enable = telnet_set, .telnet_peer = telnet_peer};
static const td_ota_ops_t ota = {.info = ota_info, .status = ota_status};
static const td_sysinfo_t info = {.platform = "Test CrowPanel", .chip = "ESP32-S3", .sdk_version = "test", .free_heap = ram, .min_free_heap = ram, .total_heap = total_ram, .psram_total = psram, .psram_free = psram, .cpu_mhz = cpu, .tasks = tasks, .fs = &fs, .users = &user_ops, .mouse = &mouse_ops, .keyboard = &keyboard_ops, .net = &net, .ota = &ota};
static td_buffer_t back;
static void tick(unsigned ms)
{
    now += ms;
    td_timers_run(now);
    td_wm_compose(&back);
}
static td_widget_t *widget(td_window_t *w, const char *text)
{
    for (td_widget_t *p = w->widgets; p; p = p->next)
        if (!strcmp(td_widget_text(p), text))
            return p;
    return NULL;
}
static td_widget_t *list(td_window_t *w, int index)
{
    for (td_widget_t *p = w->widgets; p; p = p->next)
        if (p->type == TD_WT_LIST && index-- == 0)
            return p;
    return NULL;
}
static void activate(td_widget_t *w)
{
    CHECK(w != NULL);
    if (w && w->on_activate)
        w->on_activate(w, w->user);
}
static void key(uint32_t k)
{
    td_event_t e = {.type = TD_EV_KEY, .key = k};
    td_wm_dispatch(&e);
}
static void click(int x, int y, int button)
{
    td_event_t e = {.type = TD_EV_MOUSE, .x = x, .y = y, .action = TD_MOUSE_PRESS, .button = button};
    td_wm_dispatch(&e);
}
static int find_text(const char *text, int *out_x)
{
    for (int y = 0; y < back.rows; y++)
        for (int x = 0; x < back.cols - (int)strlen(text); x++)
        {
            bool found = true;
            for (unsigned i = 0; i < strlen(text); i++)
                if (td_buffer_cell(&back, x + (int)i, y)->ch != (uint8_t)text[i])
                    found = false;
            if (found)
            {
                if (out_x)
                    *out_x = x;
                return y;
            }
        }
    return -1;
}
static bool registered(const char *name)
{
    for (int i = 0; i < td_app_count(); i++)
        if (!strcmp(td_app_get(i)->name, name))
            return true;
    return false;
}
static void check_controls(td_window_t *w, const char *app, bool audit)
{
    td_rect_t c = td_win_client(w);
    for (td_widget_t *p = w->widgets; p; p = p->next)
        if (p->focusable && p->visible)
        {
            td_rect_t r = td_widget_rect(p);
            bool fits = r.w > 0 && r.h > 0 && r.x >= c.x && r.y >= c.y && r.x + r.w <= c.x + c.w && r.y + r.h <= c.y + c.h;
            if (!fits)
                fprintf(stderr, "Clipped %s: %s (%d,%d %dx%d), client %dx%d\n", app, td_widget_text(p), r.x - c.x, r.y - c.y, r.w, r.h, c.w, c.h);
            if (!audit)
                CHECK(fits);
        }
}
static void layouts(bool audit)
{
    for (int large = 0; large <= 1; large++)
    {
        td_wm_set_window_size(large ? TD_UI_LARGE : TD_UI_SMALL);
        td_wm_set_taskbar_size(large ? TD_UI_LARGE : TD_UI_MEDIUM);
        for (int i = 0; i < td_app_count(); i++)
        {
            const char *name = td_app_get(i)->name;
            /* The keyboard never takes the focus and draws its own keys
             * (no widgets to clip): tests/test_wm.c covers it. */
            if (!strcmp(name, "On-screen keyboard"))
                continue;
            td_wm_close_all();
            CHECK(td_app_launch(name));
            tick(1000);
            td_window_t *w = td_win_focused();
            CHECK(w != NULL);
            if (!w)
                continue;
            check_controls(w, name, audit);
            if (w->flags & TD_WIN_RESIZABLE)
            {
                td_win_resize(w, w->min_w, w->min_h);
                tick(1000);
                check_controls(w, name, audit);
            }
        }
        td_wm_close_all();
        td_datetime_open();
        tick(1000);
        check_controls(td_win_focused(), "Date & time", audit);
    }
    td_wm_close_all();
    td_wm_set_window_size(TD_UI_SMALL);
    td_wm_set_taskbar_size(TD_UI_MEDIUM);
}
static void task_manager(void)
{
    td_window_desc_t d = {.title = "First", .rect = {0, 0, 20, 6}, .flags = TD_WIN_DEFAULT};
    td_window_t *first = td_win_create(&d);
    d.title = "Chosen";
    td_window_t *chosen = td_win_create(&d);
    CHECK(td_app_launch("Task Manager"));
    td_window_t *tm = td_win_focused();
    td_widget_t *apps = list(tm, 0), *sys = list(tm, 1);
    CHECK(apps && sys);
    td_rect_t stable = tm->rect;
    td_list_select(apps, 1);
    td_win_close(first);
    tick(1000);
    CHECK_EQ(td_list_selected(apps), 0); /* follows chosen identity as an earlier slot disappears */
    activate(widget(tm, "Switch to"));
    CHECK(td_win_focused() == chosen);
    td_win_focus(tm);
    d.title = "Blocker";
    td_window_t *blocker = td_win_create(&d);
    CHECK(blocker == first);
    tick(1000);
    td_win_focus(tm);
    uint32_t old = chosen->generation;
    td_win_close(chosen);
    d.title = "Replacement";
    td_window_t *replacement = td_win_create(&d);
    CHECK(replacement == chosen);
    CHECK(replacement->generation != old);
    td_win_focus(tm);
    activate(widget(tm, "End task"));
    CHECK(td_win_is_open(replacement)); /* no tick yet: stale handle rejected */
    tick(1000);
    CHECK_EQ(td_list_selected(apps), -1);
    activate(widget(tm, "End task"));
    CHECK(td_win_is_open(replacement));
    td_list_select(apps, 1);
    td_widget_focus(sys);
    key(TD_KEY_DELETE);
    CHECK(td_win_is_open(replacement));
    for (int i = 0; i < 120; i++)
        tick(1000);
    CHECK(!memcmp(&stable, &tm->rect, sizeof(stable)));
    int fg = -1;
    const char *row = sys->get_item(sys, 0, &fg, sys->user);
    CHECK(strstr(row, "4294967295") == NULL);
    td_wm_close_all();
}
static void log_follow(void)
{
    for (int i = 0; i < 80; i++)
        td_logf('I', "Entry %03d", i);
    CHECK(td_app_launch("Log Viewer"));
    td_window_t *w = td_win_focused();
    td_widget_t *rows = list(w, 0), *follow = widget(w, "Follow");
    CHECK(rows && follow);
    tick(300);
    CHECK_EQ(rows->count, 64);
    td_list_set_view(rows, 10, 5);
    td_logf('I', "Newest");
    tick(300);
    CHECK_EQ(rows->value, 63);
    CHECK(rows->scroll > 5);
    td_checkbox_set(follow, false);
    tick(300);
    td_list_set_view(rows, 20, 12);
    int fg = -1;
    char before[100];
    snprintf(before, sizeof(before), "%s", rows->get_item(rows, 12, &fg, rows->user));
    td_logf('I', "Rotated");
    tick(300);
    CHECK_EQ(rows->scroll, 11);
    CHECK_EQ(rows->value, 19);
    CHECK(!strcmp(before, rows->get_item(rows, rows->scroll, &fg, rows->user)));
    td_checkbox_set(follow, true);
    tick(300);
    CHECK_EQ(rows->value, 63);
    activate(widget(w, "Clear"));
    tick(300);
    CHECK_EQ(rows->count, 0);
    td_logf('I', "After clear");
    tick(300);
    CHECK_EQ(rows->count, 1);
    td_wm_close_all();
}
static int extra_chosen;
static void extra(void)
{
    extra_chosen++;
}
static void menu_scroll(void)
{
    td_wm_add_start_item("Extra one", extra);
    td_wm_add_start_item("Extra two", extra);
    td_wm_add_start_item("Extra three", extra);
    td_wm_add_start_item("Extra last", extra);
    td_wm_set_screen_size(80, 14);
    td_wm_toggle_start_menu();
    key(TD_KEY_END);
    td_wm_compose(&back);
    CHECK(find_text("Exit", NULL) >= 0);
    key(TD_KEY_UP);
    key(TD_KEY_UP);
    td_wm_compose(&back);
    CHECK(find_text("Extra last", NULL) >= 0);
    key(TD_KEY_ENTER);
    CHECK_EQ(extra_chosen, 1);
    td_wm_toggle_start_menu();
    td_wm_compose(&back);
    for (int i = 0; i < 40 && find_text("Extra last", NULL) < 0; i++)
    {
        int x = -1, y = -1;
        for (int r = 0; r < back.rows; r++)
            for (int c = 0; c < back.cols; c++)
                if (td_buffer_cell(&back, c, r)->ch == 0x25bc)
                {
                    x = c;
                    y = r;
                }
        CHECK(x >= 0);
        if (x < 0)
            break;
        click(x, y, TD_BUTTON_LEFT);
        td_wm_compose(&back);
    }
    int x, y = find_text("Extra last", &x);
    CHECK(y >= 0);
    if (y >= 0)
        click(x, y, TD_BUTTON_LEFT);
    CHECK_EQ(extra_chosen, 2);
    td_wm_toggle_start_menu();
    key(TD_KEY_END);
    td_wm_compose(&back);
    click(3, 5, TD_BUTTON_WHEEL_UP);
    td_wm_compose(&back);
    td_wm_set_screen_size(80, 24);
    CHECK(!td_menu_is_open());
}
static char run_command[256];
static int run_write(void *ctx, const uint8_t *buf, int n)
{
    (void)ctx;
    size_t len = strlen(run_command);
    if (len + (size_t)n < sizeof(run_command))
    {
        memcpy(run_command + len, buf, (size_t)n);
        run_command[len + n] = 0;
    }
    return n;
}
static int run_start(void *ctx, int cols, int rows)
{
    (void)ctx;
    (void)cols;
    (void)rows;
    return 0;
}
static int run_read(void *ctx, uint8_t *buf, int n)
{
    (void)ctx;
    (void)buf;
    (void)n;
    return 0;
}
static void compiled_launch(void)
{
    static const td_term_backend_t backend = {.name = "Test", .start = run_start, .read = run_read, .write = run_write};
    compiled_files = true;
    td_terminal_set_backend(&backend);
    td_files_open("/fs");
    td_window_t *files = td_win_focused();
    td_widget_t *rows = list(files, 0);
    CHECK(rows && rows->count >= 2);
    bool found_mpy = false;
    for (int i = 0; i < rows->count; i++)
    {
        int fg = -1;
        const char *row = rows->get_item(rows, i, &fg, rows->user);
        if (!strstr(row, ".mpy"))
            continue;
        found_mpy = true;
        td_list_select(rows, i);
        td_rect_t r = td_widget_rect(rows);
        click(r.x + 2, r.y + i, TD_BUTTON_RIGHT);
        tick(1);
        CHECK(find_text("Run (F9)", NULL) >= 0);
        key(TD_KEY_ESC);
        td_win_focus(files);
        run_command[0] = 0;
        key(TD_KEY_F9);
        if (!strstr(run_command, "test.mpy"))
            fprintf(stderr, "F9 command [%s], selected %d, focus %s\n", run_command, td_list_selected(rows), td_win_focused() ? td_win_focused()->title : "none");
        CHECK(strstr(run_command, "py \"") && strstr(run_command, "test.mpy"));
        break;
    }
    CHECK(found_mpy);
    td_win_focus(files);
    run_command[0] = 0;
    td_list_select(rows, rows->count - 1);
    activate(rows);
    CHECK(strstr(run_command, "py \"") && strstr(run_command, "test.tdapp"));
    td_wm_close_all();
    td_terminal_set_backend(NULL);
    compiled_files = false;
}
int main(int argc, char **argv)
{
    bool audit = argc > 1 && !strcmp(argv[1], "--audit");
    td_set_sysinfo(&info);
    td_init(&hal);
    td_wm_set_screen_size(80, 24);
    td_apps_register_all();
    const char *required[] = {"Task Manager", "Users", "Mouse", "Keyboard", "Pixels", "Image Viewer", "About"};
    for (unsigned i = 0; i < sizeof(required) / sizeof(required[0]); i++)
        CHECK(registered(required[i]));
    layouts(audit);
    td_wm_set_screen_size(100, 30);
    layouts(audit);
    td_wm_set_screen_size(80, 24);
    if (!audit)
    {
        task_manager();
        log_follow();
        menu_scroll();
        compiled_launch();
    }
    return TD_TEST_RESULT();
}
