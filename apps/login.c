/* Native authentication UI. The WM gate hides other windows and shortcuts. */
#include "td_apps.h"
#include <stdio.h>
#include <string.h>

static td_window_t *s_win;
static td_widget_t *s_accounts, *s_password, *s_remember, *s_note, *s_sign_in, *s_keyboard_button, *s_hint_button;
static char s_secret[129], s_name[32] = "root";
static char s_message[96];
static int s_fails;
static uint32_t s_retry;
static int s_selected = -1;
static bool s_keyboard;
static void login_layout(void);
static void sign_in(td_widget_t *button, void *user);

static void wipe_secret(void)
{
    volatile char *p = s_secret;
    for (unsigned i = 0; i < sizeof(s_secret); i++)
        p[i] = 0;
    if (s_password && s_password->used)
    {
        s_password->value = 0;
        s_password->scroll = 0;
    }
}
static bool prevent_close(td_window_t *win)
{
    (void)win;
    return false;
}
static void login_closed(td_window_t *win)
{
    (void)win;
    wipe_secret();
    s_message[0] = 0;
    s_win = NULL;
    s_accounts = s_password = s_remember = s_note = s_sign_in = s_keyboard_button = s_hint_button = NULL;
    if (s_keyboard)
        td_osk_show(false);   /* the On-screen keyboard Sign in opened */
    s_keyboard = false;
}
static const char *login_item(td_widget_t *list, int index, int *fg, void *user)
{
    (void)list;
    (void)user;
    (void)fg;
    static char out[64];
    const td_user_ops_t *ops = td_sysinfo()->users;
    td_user_info_t info;
    if (ops && ops->list && ops->list(index, &info) == 0)
    {
        snprintf(out, sizeof(out), "%-16.16s %s", info.name, info.role == TD_USER_ROLE_ROOT ? "Root" : info.role == TD_USER_ROLE_ADMIN ? "Admin"
                                                                                                                                       : "User");
        return out;
    }
    return index == 0 ? "root" : "";
}
static void chosen(td_widget_t *list, void *user)
{
    (void)user;
    const td_user_ops_t *ops = td_sysinfo()->users;
    td_user_info_t info;
    if (td_session_auth_pending())
        return;
    if (ops && ops->list && ops->list(list->value, &info) == 0)
        snprintf(s_name, sizeof(s_name), "%s", info.name);
    else
        snprintf(s_name, sizeof(s_name), "root");
    wipe_secret();
    s_selected = list->value;
    if (s_remember)
    {
        s_remember->visible = !strcmp(s_name, "root");
        td_checkbox_set(s_remember, ops && ops->remember_allowed && ops->remember_allowed(s_name));
    }
    if (s_message[0] && !strncmp(s_message, "Hint", 4))
        td_login_status("");   /* a hint belongs to the account it was asked for */
    td_widget_focus(s_password);
}
static void keyboard_toggle(td_widget_t *button, void *user)
{
    (void)button;
    (void)user;
    s_keyboard = !s_keyboard;
    login_layout();
    td_widget_focus(s_password);
    td_osk_show(s_keyboard);   /* the one On-screen keyboard, below the form */
    td_win_invalidate(s_win);
}
static void show_hint(td_widget_t *button, void *user)
{
    (void)button;
    (void)user;
    const td_user_ops_t *ops = td_sysinfo()->users;
    char hint[TD_HINT_MAX] = "", line[sizeof(s_message)];
    if (!ops || !ops->hint_get || ops->hint_get(s_name, hint, sizeof(hint)) != 0 || !hint[0])
        snprintf(line, sizeof(line), "%s has no password hint.", s_name);
    else
        snprintf(line, sizeof(line), "Hint: %s", hint);
    td_login_status(line);
    td_widget_focus(s_password);
}
/* Where the sign-in block (42 x 19 cells) starts in a w x h window: in the
 * middle, as far as it fits. */
static void block_origin(int w, int h, int *x, int *y)
{
    *x = (w - 42) / 2 < 1 ? 1 : (w - 42) / 2;
    *y = (h - 19) / 2 < 0 ? 0 : (h - 19) / 2;
}

/* The window's size the controls were placed for: a terminal that reports
 * its size late (at start) or changes it (a maximised PuTTY or browser)
 * gets them placed again, from the window's own size, so they stay with
 * the text draw_login() centres. */
static int s_laid_w, s_laid_h;

/* Without the keys: the account list in the middle. With them (the On-screen
 * keyboard window, apps/osk.c, at the bottom): the password, the buttons
 * and the note at the top. */
static void login_layout_for(int w, int h)
{
    if (!s_accounts || !s_password || !s_remember || !s_sign_in || !s_note || !s_keyboard_button || !s_hint_button)
        return;
    s_laid_w = w;
    s_laid_h = h;
    int x, y;
    block_origin(w, h, &x, &y);
    s_accounts->visible = !s_keyboard;
    s_accounts->rect = td_rect(x, y + 4, 40, 5);
    if (s_keyboard)
    {
        s_password->rect = td_rect(3, 2, w - 6, 1);
        s_remember->rect.x = 3;
        s_remember->rect.y = 3;
        s_sign_in->rect.x = 3;
        s_sign_in->rect.y = 4;
        s_keyboard_button->rect.x = 17;
        s_keyboard_button->rect.y = 4;
        s_hint_button->rect.x = 31;
        s_hint_button->rect.y = 4;
        s_note->rect = td_rect(3, 5, w - 6, 1);
    }
    else
    {
        s_password->rect = td_rect(x, y + 11, 40, 1);
        s_remember->rect.x = x;
        s_remember->rect.y = y + 13;
        s_sign_in->rect.x = x;
        s_sign_in->rect.y = y + 15;
        s_keyboard_button->rect.x = x + 14;
        s_keyboard_button->rect.y = y + 15;
        s_hint_button->rect.x = x + 28;
        s_hint_button->rect.y = y + 15;
        s_note->rect = td_rect(x, y + 17, w - x - 1 < 60 ? w - x - 1 : 60, 1);
    }
    td_widget_set_text(s_keyboard_button, s_keyboard ? "Accounts" : "Keyboard");
}

static void login_layout(void)
{
    if (!s_win)
        return;
    td_rect_t c = td_win_client(s_win);
    login_layout_for(c.w, c.h);
}
void td_login_status(const char *message)
{
    snprintf(s_message, sizeof(s_message), "%s", message ? message : "");
    if (td_win_is_open(s_win))
        td_win_invalidate(s_win);
}
static void sign_in(td_widget_t *button, void *user)
{
    (void)button;
    (void)user;
    uint32_t now = td_millis();
    if (td_session_auth_pending() || (int32_t)(s_retry - now) > 0)
        return;
    const td_sysinfo_t *si = td_sysinfo();
    const td_user_ops_t *ops = si->users;
    td_user_info_t selected_user;
    if (ops && ops->list && s_accounts && ops->list(s_accounts->value, &selected_user) == 0)
        snprintf(s_name, sizeof(s_name), "%s", selected_user.name);
    uint32_t generation = ops && ops->login ? ops->login(s_name, s_secret) : 0;
    bool authenticated = ops && ops->login ? generation != 0 : si->authenticate && si->authenticate(s_name, s_secret);
    if (!si->user_exists || !si->user_exists(s_name) || !authenticated)
    {
        wipe_secret();
        s_fails++;
        s_retry = now + (s_fails >= 5 ? 10000 : 1000);
        bool hint = ops && ops->hint_get;
        td_login_status(s_fails >= 5 ? "Too many attempts. Try again in 10 seconds." : hint ? "Incorrect user or password. Hint shows the password hint."
                                                                                            : "Incorrect user or password.");
        return;
    }
    wipe_secret();
    s_fails = 0;
    s_retry = 0;
    if (!strcmp(s_name, "root") && ops)
    {
        if (td_checkbox_get(s_remember) && ops->boot_user_set)
            (void)ops->boot_user_set("root", s_name);
        else if (ops->require_password)
            (void)ops->require_password("root");
    }
    char name[sizeof(s_name)];
    snprintf(name, sizeof(name), "%s", s_name);
    td_login_status("Starting session...");
    td_session_accept_login(name, generation);
}
static void draw_login(td_window_t *win, int w, int h)
{
    (void)win;
    if (w != s_laid_w || h != s_laid_h)
        login_layout_for(w, h);   /* the terminal's size changed: the controls follow */
    int x, y;
    block_origin(w, h, &x, &y);
    const td_theme_t *t = td_theme();
    td_fill(td_rect(0, 0, w, h), ' ', t->win_fg, t->win_bg);
    if (s_keyboard)
    {
        char label[96];
        snprintf(label, sizeof(label), "TINYDESK  /  SIGN IN  /  password for %s", s_name);
        td_textn(3, 0, label, w - 6, t->accent, t->win_bg, TD_BOLD);
    }
    /* The note is drawn here, not by its label: labels hold only
     * TD_TEXT_MAX bytes and a hint line can be longer. */
    if (s_note)
        td_textn(s_note->rect.x, s_note->rect.y, s_message, s_note->rect.w, t->accent, t->win_bg, 0);
    if (s_keyboard)
        return;
    td_text(x, y, "TINYDESK", t->accent, t->win_bg, TD_BOLD);
    td_text(x, y + 2, "Choose an account and enter its password.", t->win_fg, t->win_bg, 0);
    td_text(x, y + 10, "Password", t->win_fg, t->win_bg, 0);
    td_text(x, y + 18, "Root controls remembered login with bootuser.", 244, t->win_bg, 0);
}
static void login_tick(td_window_t *win)
{
    if (s_accounts && s_accounts->value != s_selected && !td_session_auth_pending())
        chosen(s_accounts, NULL);
    if (s_keyboard && !td_osk_shown())
    {
        /* Its Hide key closed the keyboard: the account list again. */
        s_keyboard = false;
        login_layout();
        td_win_invalidate(win);
    }
}
void td_login_open(void)
{
    if (td_win_is_open(s_win))
    {
        td_wm_set_input_gate(true, s_win);
        return;
    }
    td_wm_set_input_gate(true, NULL);
    const td_stats_t *st = td_stats();
    int x, y;
    block_origin(st->cols, st->rows, &x, &y);
    td_window_desc_t d = {.title = "Sign in", .rect = td_rect(0, 0, st->cols, st->rows), .flags = TD_WIN_FULLSCREEN | TD_WIN_MODAL, .on_draw = draw_login, .on_close = login_closed, .on_close_request = prevent_close, .on_tick = login_tick, .tick_ms = 200};
    s_win = td_win_create(&d);
    if (!s_win)
        return;
    s_accounts = td_list(s_win, td_rect(x, y + 4, 40, 5), login_item, chosen, NULL);
    int count = 0;
    td_user_info_t info;
    const td_user_ops_t *ops = td_sysinfo()->users;
    if (ops && ops->list)
        while (count < 12 && ops->list(count, &info) == 0)
            count++;
    if (s_accounts)
        td_list_set_count(s_accounts, count ? count : 1);
    s_password = td_textbox(s_win, x, y + 11, 40, 128, sign_in, NULL);
    if (s_password)
    {
        td_textbox_set_buffer(s_password, s_secret, sizeof(s_secret));
        s_password->secret = true;
    }
    s_remember = td_checkbox(s_win, x, y + 13, "Remember root on this device", false, NULL, NULL);
    s_note = td_label(s_win, x, y + 17, 42, s_message);
    s_sign_in = td_button(s_win, x, y + 15, "Sign in", sign_in, NULL);
    s_keyboard_button = td_button(s_win, x + 14, y + 15, "Keyboard", keyboard_toggle, NULL);
    s_hint_button = td_button(s_win, x + 28, y + 15, "Hint", show_hint, NULL);
    if (s_win->incomplete)
    {
        td_win_close(s_win);
        return;
    }
    if (s_hint_button)
        s_hint_button->visible = ops && ops->hint_get;
    if (s_note)
        s_note->visible = false;   /* drawn by draw_login */
    if (s_accounts)
        chosen(s_accounts, NULL);
    login_layout();
    td_wm_set_input_gate(true, s_win);
}
