/*
 * apps.c - register all built-in apps.
 */
#include "td_apps.h"
#include "td_modbus.h"
#include "td_mqtt.h"

/* MQTT and Modbus run from the main loop: no extra task, nothing to do
 * while they are unused. */
static void proto_poll(void *user)
{
    (void)user;
    td_mqtt_poll();
    td_mb_poll();
}

void td_proto_service_start(void)
{
    static bool started;
    if (started)
        return;
    started = true;
    td_timer_start(20, true, proto_poll, NULL, td_millis());
}

void td_apps_register_all(void)
{
    td_terminal_register();
    td_files_register();
    td_editor_register();
    td_network_register();
    td_mqtt_register();
    td_modbus_register();
    td_sysmon_register();
    td_taskmgr_register();
    td_logview_register();
    td_settings_register();
    td_update_register();
    td_counter_register();
    td_about_register();
    td_datetime_install_clock();
    td_proto_service_start();
    td_pyui_start();     /* windows for Python scripts (tdui) */
    td_session_init();   /* root until a shell reports its user; ~/Desktop icons,
                          * that user's settings */
}
