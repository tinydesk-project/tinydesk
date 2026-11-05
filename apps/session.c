/*
 * session.c - whose desktop this is.
 *
 * Mirrors TinyDesk Shell's user model: root sees the whole filesystem and lives in
 * /root; every other user lives in /home/<user> and is kept inside it. The
 * current user decides the Desktop folder, where the Files app may go and
 * which user the Terminal's shell runs as.
 *
 * The user changes when someone logs in to the desktop (Telnet, or
 * Start > Switch user...), and follows `login` / `logout` typed in the
 * Terminal. A change closes the previous user's windows and clears the
 * Terminal so nothing of theirs stays on screen.
 */
#include <stdio.h>
#include <string.h>

#include "td_apps.h"

#define NAME_LEN  32
#define PATH_LEN  TD_PATH_MAX
#define FOLLOW_MS 1000
#define EXPECT_MS 10000

static char s_user[NAME_LEN] = "root";
static char s_home[PATH_LEN];
static char s_jail[PATH_LEN];
static char s_expect[NAME_LEN];      /* user we asked the shell to become */
static uint32_t s_expect_until;
static char s_login_name[NAME_LEN];  /* Switch user dialog */
static bool s_started;
static bool s_authenticated = true;
static bool s_switch_sent;
static bool s_silent_login;
static uint32_t s_generation, s_expect_generation;
static void compute_paths(void);
static uint32_t user_generation(const char *name)
{
    const td_user_ops_t *ops = td_sysinfo()->users;
    return ops && ops->generation ? ops->generation(name) : 0;
}
static bool generation_valid(void)
{
    const td_user_ops_t *ops = td_sysinfo()->users;
    return !ops || !ops->generation || (s_generation && s_generation == ops->generation(s_user));
}


const char *td_session_user(void)
{
    return s_user;
}
bool td_session_is_root(void)
{
    return !td_session_locked() && strcmp(s_user, "root") == 0;
}
bool td_session_locked(void)
{
    return td_sysinfo()->authenticate && (!s_authenticated || !generation_valid());
}
bool td_session_auth_pending(void)
{
    return s_expect[0] != 0;
}
int td_session_role(void)
{
    const td_user_ops_t *ops = td_sysinfo()->users;
    return td_session_is_root() ? TD_USER_ROLE_ROOT : ops && ops->role ? ops->role(s_user)
                                                                       : TD_USER_ROLE_USER;
}
bool td_session_has_right(uint32_t right)
{
    if (td_session_locked())
        return false;
    const td_user_ops_t *ops = td_sysinfo()->users;
    uint32_t rights = td_session_is_root() ? TD_RIGHT_ALL : ops && ops->rights ? ops->rights(s_user)
                                                                               : TD_RIGHT_APPS;
    return (rights & right) == right;
}
const char *td_session_home(void)
{
    return s_home;
}
const char *td_session_jail(void)
{
    compute_paths();
    return s_jail;
}

static void compute_paths(void)
{
    const td_fs_ops_t *fs = td_sysinfo()->fs;
    const char *root = fs ? fs->root : "";
    if (td_session_is_root())
    {
        snprintf(s_home, sizeof(s_home), "%s/root", root);
        snprintf(s_jail, sizeof(s_jail), "%s", root);
    }
    else
    {
        snprintf(s_home, sizeof(s_home), "%s/home/%.*s", root, NAME_LEN - 1, s_user);
        snprintf(s_jail, sizeof(s_jail), "%s", td_session_has_right(TD_RIGHT_SHARED) ? root : s_home);
    }
}

bool td_session_real_path(const char *path, char *real, size_t cap)
{
    const td_fs_ops_t *fs = td_sysinfo()->fs;
    if (td_session_locked() || !fs || !path || !path[0] || strstr(path, ".."))
        return false;
    char home[PATH_LEN], logical[PATH_LEN + 96];
    snprintf(home, sizeof(home), "%s", td_shell_path(s_home));
    if (path[0] == '~')
        snprintf(logical, sizeof(logical), "%s%s", home, path + 1);
    else if (path[0] == '/')
        snprintf(logical, sizeof(logical), "%s", path);
    else
        snprintf(logical, sizeof(logical), "%s/%s", home, path);
    if (!td_session_is_root())
    {
        size_t n = strlen(home);
        bool shared = td_session_has_right(TD_RIGHT_SHARED) && (!strcmp(logical, "/shared") || !strncmp(logical, "/shared/", 8));
        if (!shared && (strncmp(logical, home, n) != 0 || (logical[n] != '/' && logical[n] != '\0')))
            return false;
    }
    return snprintf(real, cap, "%s%s", fs->root, logical) < (int)cap;
}
static bool child_path(const char *path, const char *root)
{
    size_t n = strlen(root);
    return !strncmp(path, root, n) && (!path[n] || path[n] == '/');
}
static bool same_folder(const char *path, const char *root)
{
    size_t n = strlen(path);
    while (n && path[n - 1] == '/')
        n--;
    return n == strlen(root) && !strncmp(path, root, n);
}
bool td_session_path_allowed(const char *path, int action)
{
    if (td_session_locked() || !path)
        return false;
    if (td_session_is_root())
        return true;
    const td_fs_ops_t *fs = td_sysinfo()->fs;
    if (!fs || !fs->root)
        return false;
    size_t n = strlen(fs->root);
    if (strncmp(path, fs->root, n) || (path[n] && path[n] != '/'))
        return false;
    path += n;
    if (strchr(path, '\\') || strchr(path, ':'))
        return false;
    for (const char *p = path; *p; p++)
        if ((p == path || p[-1] == '/') && p[0] == '.' && (!p[1] || p[1] == '/' || (p[1] == '.' && (!p[2] || p[2] == '/'))))
            return false;
    char home[48];
    snprintf(home, sizeof(home), "/home/%s", s_user);
    if (child_path(path, home))
        return action != TD_FS_REMOVE || !same_folder(path, home);
    if (td_session_has_right(TD_RIGHT_SHARED))
    {
        if (child_path(path, "/shared"))
            return action != TD_FS_REMOVE || !same_folder(path, "/shared");
        if ((action == TD_FS_LIST || action == TD_FS_EXISTS) && (!path[0] || !strcmp(path, "/") || !strcmp(path, "/home")))
            return true;
    }
    return false;
}

static const char *shell_user(void)
{
    const td_term_backend_t *b = td_terminal_backend();
    const char *u = (b && b->user) ? b->user(b->ctx) : NULL;
    return (u && u[0]) ? u : NULL;
}

/* Ask the shell to redraw its prompt (TinyDesk Shell's line editor: Ctrl+L). */
static void shell_redraw(void)
{
    const td_term_backend_t *b = td_terminal_backend();
    if (b && b->write)
        b->write(b->ctx, (const uint8_t *)"\x0c", 1);
}

static void session_commit(const char *user, bool switch_shell)
{
    if (!user || !user[0])
        return;
    /* Nothing of the previous user may stay on screen. */
    td_pyui_detach_begin();
    td_menu_close();
    td_wm_close_all();
    td_terminal_reset();

    snprintf(s_user, sizeof(s_user), "%s", user);
    s_generation = user_generation(s_user);
    compute_paths();
    td_files_reset();
    td_settings_apply_saved();       /* their theme, pattern, clock format */
    td_clipboard_clear();            /* nothing copied by the previous user */

    const td_term_backend_t *b = td_terminal_backend();
    if (switch_shell && b && b->set_user)
    {
        snprintf(s_expect, sizeof(s_expect), "%s", user);
        s_expect_until = td_millis() + EXPECT_MS;
        b->set_user(b->ctx, user);
    }
    else
    {
        shell_redraw();
    }

    td_desktop_folder_init();
    td_wm_set_user_label(s_user);
    td_logf('I', "desktop session: %s", s_user);
    td_wm_invalidate();
}

void td_session_lock(void)
{
    if (!td_sysinfo()->authenticate)
        return;
    s_authenticated = false;
    s_expect[0] = 0;
    s_switch_sent = false;
    s_silent_login = false;
    td_wm_set_input_gate(true, NULL);
    td_kiosk_stop_for_login();
    td_pyui_detach_begin();
    td_menu_close();
    td_wm_close_all();
    td_terminal_reset();
    td_clipboard_clear();
    const td_term_backend_t *b = td_terminal_backend();
    if (b && b->set_user)
        b->set_user(b->ctx, "");
    td_wm_set_user_label("Locked");
    td_login_open();
}
void td_session_accept_authenticated(const char *user)
{
    td_session_accept_login(user, user_generation(user));
}
void td_session_accept_login(const char *user, uint32_t generation)
{
    if (!user || !user[0] || (td_sysinfo()->user_exists && !td_sysinfo()->user_exists(user)))
        return;
    if (generation != user_generation(user))
    {
        td_login_status("Account changed. Please sign in again.");
        return;
    }
    snprintf(s_expect, sizeof(s_expect), "%s", user);
    s_expect_until = td_millis() + EXPECT_MS;
    s_switch_sent = false;
    s_expect_generation = generation;
    if (td_sysinfo()->authenticate)
    {
        s_authenticated = false;
        td_wm_set_input_gate(true, NULL);
        if (!s_silent_login)
        {
            td_login_open();
            td_login_status("Starting session...");
        }
    }
}
void td_session_switch(const char *user, bool switch_shell)
{
    if (switch_shell && td_sysinfo()->authenticate)
    {
        td_session_accept_authenticated(user);
        return;
    }
    s_authenticated = true;
    td_wm_set_input_gate(false, NULL);
    session_commit(user, switch_shell);
}
static void follow_shell(void *user)
{
    (void)user;
    const td_term_backend_t *b = td_terminal_backend();
    const char *su = shell_user();
    if (s_expect[0])
    {
        if (user_generation(s_expect) != s_expect_generation)
        {
            s_expect[0] = 0;
            s_switch_sent = false;
            s_silent_login = false;
            td_login_open();
            td_login_status("Account changed. Please sign in again.");
            return;
        }
        if (su && !strcmp(su, s_expect))
        {
            char name[NAME_LEN];
            snprintf(name, sizeof(name), "%s", s_expect);
            s_expect[0] = 0;
            s_silent_login = false;
            s_authenticated = true;
            td_wm_set_input_gate(false, NULL);
            session_commit(name, false);
            td_kiosk_start();
            return;
        }
        if (!s_switch_sent && b && b->set_user)
        {
            b->set_user(b->ctx, s_expect);
            s_switch_sent = true;
        }
        if ((int32_t)(td_millis() - s_expect_until) >= 0)
        {
            s_expect[0] = 0;
            s_switch_sent = false;
            s_silent_login = false;
            td_login_open();
            td_login_status("The shell is busy. Please try signing in again.");
        }
        return;
    }
    if (s_authenticated && !generation_valid())
    {
        td_session_lock();
        return;
    }
    if (td_session_locked())
    {
        td_login_open();
        return;
    }
    if (!su)
    {
        if (b && b->user && td_sysinfo()->authenticate)
            td_session_lock();
        return;
    }
    if (td_sysinfo()->user_exists && !td_sysinfo()->user_exists(su))
    {
        td_session_lock();
        return;
    }
    if (strcmp(su, s_user))
        td_session_switch(su, false);
}

/* ------------------------------------------------ Start > Switch user */

static void password_answer(const char *password, void *user)
{
    (void)user;
    const td_sysinfo_t *si = td_sysinfo();
    const td_user_ops_t *ops = si->users;
    uint32_t generation = ops && ops->login ? ops->login(s_login_name, password) : 0;
    bool ok = ops && ops->login ? generation != 0 : si->authenticate && si->authenticate(s_login_name, password);
    if (!ok)
    {
        td_msgbox("Switch user", "Wrong user name or password.", "OK", NULL, NULL);
        return;
    }
    td_session_accept_login(s_login_name, generation);
}

static void name_answer(const char *name, void *user)
{
    (void)user;
    const td_sysinfo_t *si = td_sysinfo();
    if (!si->user_exists || !si->user_exists(name))
    {
        td_msgbox("Switch user", "There is no user with that name.", "OK", NULL, NULL);
        return;
    }
    snprintf(s_login_name, sizeof(s_login_name), "%s", name);
    char prompt[64];
    snprintf(prompt, sizeof(prompt), "Password for %.32s:", name);
    td_passwordbox("Switch user", prompt, password_answer, NULL);
}

static void switch_user_menu(void)
{
    td_inputbox("Switch user", "User name:", "", name_answer, NULL);
}

/* ------------------------------------------------------------- init */

void td_session_init(void)
{
    if (s_started)
        return;
    s_started = true;
    td_timer_start(100, true, follow_shell, NULL, td_millis());
    if (td_sysinfo()->authenticate)
    {
        s_authenticated = false;
        td_wm_set_user_label("Locked");
        td_wm_set_input_gate(true, NULL);
        const td_user_ops_t *ops = td_sysinfo()->users;
        char name[NAME_LEN];
        uint32_t generation = ops && ops->remembered ? ops->remembered(name, sizeof(name)) : 0;
        if (generation)
        {
            s_silent_login = true;
            td_session_accept_login(name, generation);
        }
        if (!s_expect[0])
        {
            s_silent_login = false;
            td_login_open();
        }
        td_wm_add_start_item("Switch user...", switch_user_menu);
        td_wm_add_start_item("Sign out", td_session_lock);
        return;
    }
    const char *su = shell_user();
    snprintf(s_user, sizeof(s_user), "%s", su ? su : "root");
    compute_paths();
    td_files_reset();
    td_settings_apply_saved();
    td_desktop_folder_init();
    td_wm_set_user_label(s_user);
}
