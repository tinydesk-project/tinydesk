/*
 * mqtt.c - the MQTT app: connect to a broker, subscribe, publish and watch
 * messages. It shares its connection with the `mqtt` shell command.
 *
 * Labels are drawn in on_draw rather than as label widgets: the ESP32 build
 * has a small widget pool shared by all windows.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "td_apps.h"
#include "td_mqtt.h"
#include "td_sock.h"

#define LIST_Y 6

/* Window state, allocated while the window is open (RAM is tight on the
 * ESP32: nothing here should cost memory while the app is closed). */
typedef struct
{
    char user[32], pass[64];
    char note[64];                    /* last action's outcome */
    uint32_t first_seq;               /* first message shown */
    uint32_t shown_last;
    char item[TD_MQTT_TOPIC_MAX + TD_MQTT_PAYLOAD_KEEP + 40];
} ui_t;

static td_window_t *s_win;
static td_widget_t *s_broker, *s_connect, *s_topic, *s_payload, *s_retain, *s_list;
static ui_t *U;

static const char *text_of(const td_widget_t *w)
{
    return td_widget_text(w);
}

static void note(const char *msg)
{
    if (!U)
        return;
    snprintf(U->note, sizeof(U->note), "%s", msg);
    td_wm_invalidate();
}

/* ------------------------------------------------------------ actions */

#define DEFAULT_CONF "~/mqtt.conf"

/* A path typed by the user or written in a config file, as the desktop user
 * may see it ("~/x", "/home/bob/x", or relative to their home), to a real
 * path. Everyone but root stays inside their home. */
static bool app_resolve(const char *path, char *real, size_t cap, void *ctx)
{
    (void)ctx;
    return td_session_real_path(path, real, cap);
}

static bool is_config_name(const char *t)
{
    size_t n = strlen(t);
    return (n > 5 && strcmp(t + n - 5, ".conf") == 0) || t[0] == '~' || t[0] == '/';
}

static bool conf_exists(void)
{
    char real[240];
    const td_fs_ops_t *fs = td_sysinfo()->fs;
    return app_resolve(DEFAULT_CONF, real, sizeof(real), NULL) && fs->exists && fs->exists(real);
}

static void do_connect(void)
{
    td_mqtt_status_t st;
    td_mqtt_status(&st);
    if (st.state != TD_MQTT_OFF)
    {
        td_mqtt_disconnect();
        note("Disconnected");
        return;
    }
    td_mqtt_config_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
        return;
    char err[96];
    const char *text = text_of(s_broker);
    if (is_config_name(text))
    {
        if (!td_mqtt_load_config(text, app_resolve, NULL, cfg, err, sizeof(err)))
        {
            char msg[64];
            snprintf(msg, sizeof(msg), "%.62s", err);
            note(msg);
            free(cfg);
            return;
        }
    }
    else
    {
        cfg->auto_reconnect = true;
        if (!td_mqtt_parse_broker(text, cfg))
        {
            note("Broker: host[:port], mqtts://host[:port] or a .conf file");
            free(cfg);
            return;
        }
    }
    if (U->user[0])
    {                         /* Login... overrides the file */
        snprintf(cfg->user, sizeof(cfg->user), "%s", U->user);
        snprintf(cfg->pass, sizeof(cfg->pass), "%s", U->pass);
    }
    note("Connecting...");
    if (!td_mqtt_connect(cfg, err, sizeof(err)))
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "%.62s", err);
        note(msg);
    }
    else
    {
        U->note[0] = '\0';
    }
    memset(cfg, 0, sizeof(*cfg));             /* passwords */
    free(cfg);
    U->first_seq = 1;
    U->shown_last = 0;
}

/* Config...: open ~/mqtt.conf in the Editor (created from the template). */
static void on_config(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    char real[240];
    const td_fs_ops_t *fs = td_sysinfo()->fs;
    if (!app_resolve(DEFAULT_CONF, real, sizeof(real), NULL) || !fs->write)
    {
        note("No filesystem here");
        return;
    }
    if (!conf_exists() && fs->write(real, td_mqtt_config_template, (int)strlen(td_mqtt_config_template)) != 0)
    {
        note("Could not create ~/mqtt.conf");
        return;
    }
    td_widget_set_text(s_broker, DEFAULT_CONF);
    note("Edit and save ~/mqtt.conf, then Connect");
    td_editor_open(real);
}

static void on_connect(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    do_connect();
}

static void pass_answer(const char *text, void *user)
{
    (void)user;
    if (!U)
        return;
    snprintf(U->pass, sizeof(U->pass), "%s", text);
    note(U->user[0] ? "Login saved for this session; connect again to use it" : "No user name: login cleared");
}

static void user_answer(const char *text, void *user)
{
    (void)user;
    if (!U)
        return;
    snprintf(U->user, sizeof(U->user), "%s", text);
    U->pass[0] = '\0';
    if (!U->user[0])
    {
        note("Login cleared");
        return;
    }
    td_passwordbox("MQTT login", "Password (empty for none):", pass_answer, NULL);
}

static void on_login(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    td_inputbox("MQTT login", "User name (empty for none):", U->user, user_answer, NULL);
}

static const char *rc_text(int rc, const char *ok)
{
    return rc == 0 ? ok : rc == -2 ? "Too many subscriptions (8 at most)"
                                   : "Connect first, and give a topic";
}

static void on_subscribe(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    td_mqtt_status_t st;
    td_mqtt_status(&st);
    note(rc_text(td_mqtt_subscribe(text_of(s_topic), 1),
                 st.state == TD_MQTT_CONNECTED ? "Subscribed" : "Will subscribe once connected"));
}

static void on_unsubscribe(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    note(td_mqtt_unsubscribe(text_of(s_topic)) == 0 ? "Unsubscribed" : "Not subscribed to that topic");
}

static void do_publish(void)
{
    const char *topic = text_of(s_topic), *msg = text_of(s_payload);
    if (strchr(topic, '+') || strchr(topic, '#'))
    {
        note("Publish needs a topic without + or #");
        return;
    }
    int rc = td_mqtt_publish(topic, msg, (int)strlen(msg), 0, td_checkbox_get(s_retain));
    note(rc == 0 ? "Published" : rc == -2 ? "Busy, try again"
                                          : "Connect first, and give a topic");
}

static void on_publish(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    do_publish();
}

/* ------------------------------------------------------------ the list */

static const char *get_item(td_widget_t *w, int index, int *fg, void *user)
{
    (void)w;
    (void)user;
    td_mqtt_msg_t m;
    if (!td_mqtt_message(U->first_seq + (uint32_t)index, &m))
        return "";
    char when[16];
    td_time_of_day(m.utc, when, sizeof(when));
    for (char *p = m.payload; *p; p++)
        if ((unsigned char)*p < 0x20)
            *p = ' ';           /* keep it on one line */
    snprintf(U->item, sizeof(U->item), "%-8s %s %s  %s%s", when, m.outgoing ? "->" : "<-", m.topic, m.payload,
             m.len > TD_MQTT_PAYLOAD_KEEP ? "..." : "");
    if (m.outgoing)
        *fg = td_theme()->dim;
    return U->item;
}

static void update(void)
{
    td_mqtt_status_t st;
    td_mqtt_status(&st);
    td_widget_set_text(s_connect, st.state == TD_MQTT_OFF ? "Connect" : "Disconnect");

    uint32_t last = td_mqtt_last_seq();
    if (last < U->shown_last)
        U->first_seq = 1;            /* a new connection */
    if (last >= TD_MQTT_LOG && U->first_seq < last - TD_MQTT_LOG + 1)
        U->first_seq = last - TD_MQTT_LOG + 1;
    if (U->first_seq == 0)
        U->first_seq = 1;
    int count = last >= U->first_seq ? (int)(last - U->first_seq + 1) : 0;
    if (last != U->shown_last)
    {
        int sel = td_list_selected(s_list);
        bool follow = sel < 0 || sel >= count - 2;
        td_list_set_count(s_list, count);
        if (follow && count > 0)
            td_list_select(s_list, count - 1);
        U->shown_last = last;
    }
    td_wm_invalidate();
}

static void on_tick(td_window_t *win)
{
    (void)win;
    update();
}

static void on_draw(td_window_t *win, int w, int h)
{
    (void)win;
    (void)h;
    const td_theme_t *t = td_theme();
    td_text(0, 0, "Broker", t->win_fg, t->win_bg, 0);
    td_text(0, 2, "Topic", t->win_fg, t->win_bg, 0);
    td_text(0, 3, "Message", t->win_fg, t->win_bg, 0);

    td_mqtt_status_t st;
    td_mqtt_status(&st);
    uint8_t fg = st.state == TD_MQTT_CONNECTED ? t->accent : t->dim;
    char line[96];
    if (st.state == TD_MQTT_CONNECTED)
        snprintf(line, sizeof(line), "%s  (%u in, %u out)", st.text, (unsigned)st.rx, (unsigned)st.tx);
    else
        snprintf(line, sizeof(line), "%s", st.text);
    td_textn(0, 1, line, 58, fg, t->win_bg, st.state == TD_MQTT_CONNECTED ? TD_BOLD : 0);

    /* Subscriptions, then the last action's outcome. */
    char subs[160] = "Subscribed: ";
    int n = 0, qos;
    char topic[TD_MQTT_TOPIC_MAX];
    while (td_mqtt_subscription(n, topic, sizeof(topic), &qos))
    {
        size_t used = strlen(subs);
        snprintf(subs + used, sizeof(subs) - used, "%s%s", n ? ", " : "", topic);
        n++;
    }
    if (!n)
        snprintf(subs, sizeof(subs), "Subscribed: nothing yet");
    td_textn(0, 4, subs, w, t->win_fg, t->win_bg, 0);
    if (U->note[0])
        td_textn(0, 5, U->note, w, t->accent, t->win_bg, 0);
    else
        td_textn(0, 5, "Time     Dir Topic  Message", w, t->dim, t->win_bg, 0);
}

static void on_close(td_window_t *win)
{
    (void)win;
    s_win = NULL;
    memset(U, 0, sizeof(*U));             /* the password too */
    free(U);
    U = NULL;
}

static void on_topic_enter(td_widget_t *w, void *user)
{
    on_subscribe(w, user);
}
static void on_payload_enter(td_widget_t *w, void *user)
{
    (void)w;
    (void)user;
    do_publish();
}

static void launch(void)
{
    if (td_win_is_open(s_win))
    {
        td_win_focus(s_win);
        return;
    }
    td_window_desc_t d = {
        .title = "MQTT",
        .rect = td_rect(-1, -1, 76, 20),
        .flags = TD_WIN_DEFAULT,
        .min_w = 75,
        .min_h = 12,
        .on_draw = on_draw,
        .on_close = on_close,
        .on_tick = on_tick,
        .tick_ms = 250,
    };
    U = calloc(1, sizeof(*U));
    if (!U)
    {
        td_msgbox("MQTT", "Not enough memory. Close a window, then try again.", "OK", NULL, NULL);
        return;
    }
    s_win = td_win_create(&d);
    if (!s_win)
    {
        free(U);
        U = NULL;
        return;
    }

    s_broker = td_textbox(s_win, 8, 0, 34, TD_TEXT_MAX - 1, on_connect, NULL);
    s_connect = td_button(s_win, 44, 0, "Connect", on_connect, NULL);
    td_button(s_win, 59, 0, "Login...", on_login, NULL);
    td_button(s_win, 59, 1, "Config...", on_config, NULL);
    s_topic = td_textbox(s_win, 8, 2, 34, TD_TEXT_MAX - 1, on_topic_enter, NULL);
    td_button(s_win, 44, 2, "Subscribe", on_subscribe, NULL);
    td_button(s_win, 58, 2, "Unsubscribe", on_unsubscribe, NULL);
    s_payload = td_textbox(s_win, 8, 3, 34, TD_TEXT_MAX - 1, on_payload_enter, NULL);
    td_button(s_win, 44, 3, "Publish", on_publish, NULL);
    s_retain = td_checkbox(s_win, 56, 3, "Retain", false, NULL, NULL);
    s_list = td_list(s_win, td_rect(0, LIST_Y, -1, 0), get_item, NULL, NULL);
    td_scrollbar(s_win, -1, LIST_Y, 0, s_list);

    td_mqtt_status_t st;
    td_mqtt_status(&st);
    td_widget_set_text(s_broker, st.broker[0] ? st.broker : conf_exists() ? DEFAULT_CONF
                                                                          : "localhost:1883");
    td_widget_set_text(s_topic, "tinydesk/test");
    td_widget_set_text(s_payload, "hello from TinyDesk");
    U->first_seq = 1;
    U->shown_last = (uint32_t)-1;
    update();
    td_widget_focus(s_broker);
}

static const td_app_t s_app = {"MQTT", launch, "MQ"};

void td_mqtt_register(void)
{
    td_app_register(&s_app);
}
