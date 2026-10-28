/*
 * td_py_mod.c - the `tinydesk` module: what a script may ask TinyDesk to
 * do. TinyDesk keeps the hardware and the connections; scripts use them
 * through the same functions as the shell's commands and the apps.
 *
 *   tinydesk.version()                          "0.1.6"
 *   tinydesk.shell(line)                        run a shell command line
 *   tinydesk.log(text, level='I')               a line in the Log Viewer
 *   tinydesk.mqtt_publish(topic, payload, qos=0, retain=False)
 *   tinydesk.modbus_read(target, unit, table, addr, count=1)  -> [values]
 *   tinydesk.modbus_write(target, unit, table, addr, values)
 */
#include <stdio.h>
#include <string.h>

#include "extmod/vfs.h"
#include "py/mperrno.h"
#include "py/runtime.h"
#include "td_apps.h"
#include "td_modbus.h"
#include "td_mqtt.h"
#include "td_py.h"
#include "tinydesk/td.h"

static mp_obj_t td_version(void)
{
    return MP_OBJ_NEW_QSTR(qstr_from_str(TD_VERSION));
}
static MP_DEFINE_CONST_FUN_OBJ_0(td_version_obj, td_version);

/* shell(line): runs in the script's session, starting from the script's
 * current directory; a `cd` in it does not move the shell. Output goes to
 * the same place as print(). Returns the exit status. */
static mp_obj_t td_shell(mp_obj_t line_in)
{
    const char *line = mp_obj_str_get_str(line_in);
    tdsh_session_t *session = td_py_session();
    if (!session)
        mp_raise_OSError(MP_EIO);
    const char *cwd = mp_obj_str_get_str(mp_vfs_getcwd());
    char saved[TDSH_MAX_PATH];
    memcpy(saved, session->cwd, sizeof(saved));
    snprintf(session->cwd, sizeof(session->cwd), "%s", cwd);
    int status = tdsh_execute_line(session, line);
    memcpy(session->cwd, saved, sizeof(saved));
    return MP_OBJ_NEW_SMALL_INT(status);
}
static MP_DEFINE_CONST_FUN_OBJ_1(td_shell_obj, td_shell);

static mp_obj_t td_log(size_t n_args, const mp_obj_t *args)
{
    const char *text = mp_obj_str_get_str(args[0]);
    char level = 'I';
    if (n_args > 1)
    {
        const char *l = mp_obj_str_get_str(args[1]);
        if (!strchr("EWID", l[0]) || !l[0] || l[1])
            mp_raise_ValueError(MP_ERROR_TEXT("level must be 'E', 'W', 'I' or 'D'"));
        level = l[0];
    }
    /* The Log Viewer ends a line at '\n': add one, in the same call, so no
     * other task's log line lands between the text and its end. */
    size_t n = strlen(text);
    if (n > 0 && text[n - 1] == '\n')
    {
        td_log_append(level, text);
        return mp_const_none;
    }
    char *line = m_new(char, n + 2);
    memcpy(line, text, n);
    line[n] = '\n';
    line[n + 1] = '\0';
    td_log_append(level, line);
    m_del(char, line, n + 2);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(td_log_obj, 1, 2, td_log);

static mp_obj_t td_mqtt_pub(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum
    {
        ARG_topic,
        ARG_payload,
        ARG_qos,
        ARG_retain
    };
    static const mp_arg_t allowed[] = {
        {MP_QSTR_topic, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        {MP_QSTR_payload, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
        {MP_QSTR_qos, MP_ARG_INT, {.u_int = 0}},
        {MP_QSTR_retain, MP_ARG_BOOL, {.u_bool = false}},
    };
    mp_arg_val_t a[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed), allowed, a);
    const char *topic = mp_obj_str_get_str(a[ARG_topic].u_obj);
    mp_buffer_info_t payload;
    mp_get_buffer_raise(a[ARG_payload].u_obj, &payload, MP_BUFFER_READ);   /* str or bytes */
    if (a[ARG_qos].u_int < 0 || a[ARG_qos].u_int > 1)
        mp_raise_ValueError(MP_ERROR_TEXT("qos must be 0 or 1"));
    td_mqtt_status_t st;
    td_mqtt_status(&st);
    if (st.state != TD_MQTT_CONNECTED)
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("MQTT is not connected (use the MQTT app or mqtt connect)"));
    if (td_mqtt_publish(topic, payload.buf, (int)payload.len, (int)a[ARG_qos].u_int, a[ARG_retain].u_bool) < 0)
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("MQTT publish failed"));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(td_mqtt_pub_obj, 2, td_mqtt_pub);

/* The request common to modbus_read and modbus_write. */
static void mb_request(td_mb_request_t *r, td_mb_table_t *t, mp_obj_t target, mp_obj_t unit, mp_obj_t table,
                       mp_obj_t addr)
{
    memset(r, 0, sizeof(*r));
    snprintf(r->target, sizeof(r->target), "%s", mp_obj_str_get_str(target));
    mp_int_t u = mp_obj_get_int(unit);
    mp_int_t a = mp_obj_get_int(addr);
    if (u < 0 || u > 255)
        mp_raise_ValueError(MP_ERROR_TEXT("unit must be 0..255"));
    if (a < 0 || a > 65535)
        mp_raise_ValueError(MP_ERROR_TEXT("address must be 0..65535"));
    if (!td_mb_parse_table(mp_obj_str_get_str(table), t))
        mp_raise_ValueError(MP_ERROR_TEXT("table must be 'co', 'di', 'hr' or 'ir'"));
    r->unit = (uint8_t)u;
    r->addr = (uint16_t)a;
}

/* Run a request; raise OSError on failure. The result is in the GC heap,
 * not on the stack (it holds up to TD_MB_MAX_READ values). */
static td_mb_result_t *mb_transact(const td_mb_request_t *r)
{
    td_mb_result_t *res = m_new(td_mb_result_t, 1);
    char err[80];
    if (!td_mb_transact(r, res, err, sizeof(err)))
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("modbus: %s"), err);
    if (res->status != 0)
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("modbus: %s"), res->text);
    return res;
}

static mp_obj_t td_modbus_read(size_t n_args, const mp_obj_t *args)
{
    td_mb_request_t r;
    td_mb_table_t t;
    mb_request(&r, &t, args[0], args[1], args[2], args[3]);
    mp_int_t count = n_args > 4 ? mp_obj_get_int(args[4]) : 1;
    if (count < 1 || count > TD_MB_MAX_READ)
        mp_raise_ValueError(MP_ERROR_TEXT("count out of range"));
    r.fc = (uint8_t)t;
    r.count = (uint16_t)count;
    td_mb_result_t *res = mb_transact(&r);
    mp_obj_t list = mp_obj_new_list(0, NULL);
    for (uint16_t i = 0; i < res->count; i++)
        mp_obj_list_append(list, MP_OBJ_NEW_SMALL_INT(res->values[i]));
    m_del(td_mb_result_t, res, 1);
    return list;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(td_modbus_read_obj, 4, 5, td_modbus_read);

/* values: one int, or a list/tuple of them. */
static mp_obj_t td_modbus_write(size_t n_args, const mp_obj_t *args)
{
    (void)n_args;
    td_mb_request_t r;
    td_mb_table_t t;
    mb_request(&r, &t, args[0], args[1], args[2], args[3]);
    if (t != TD_MB_COILS && t != TD_MB_HOLDING)
        mp_raise_ValueError(MP_ERROR_TEXT("only coils ('co') and holding registers ('hr') can be written"));
    size_t n;
    mp_obj_t *items;
    mp_obj_t one = args[4];
    if (mp_obj_is_int(one))
    {
        n = 1;
        items = &one;
    }
    else
    {
        mp_obj_get_array(args[4], &n, &items);
    }
    if (n < 1 || n > TD_MB_MAX_WRITE)
        mp_raise_ValueError(MP_ERROR_TEXT("too many values"));
    for (size_t i = 0; i < n; i++)
    {
        mp_int_t v = mp_obj_get_int(items[i]);
        if (v < -32768 || v > 65535)
            mp_raise_ValueError(MP_ERROR_TEXT("values must be 16-bit"));
        r.values[i] = (uint16_t)v;
    }
    r.nvalues = (uint16_t)n;
    if (t == TD_MB_COILS)
        r.fc = n == 1 ? 5 : 15;
    else
        r.fc = n == 1 ? 6 : 16;
    td_mb_result_t *res = mb_transact(&r);
    m_del(td_mb_result_t, res, 1);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(td_modbus_write_obj, 5, 5, td_modbus_write);

static const mp_rom_map_elem_t td_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_tinydesk)},
    {MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&td_version_obj)},
    {MP_ROM_QSTR(MP_QSTR_shell), MP_ROM_PTR(&td_shell_obj)},
    {MP_ROM_QSTR(MP_QSTR_log), MP_ROM_PTR(&td_log_obj)},
    {MP_ROM_QSTR(MP_QSTR_mqtt_publish), MP_ROM_PTR(&td_mqtt_pub_obj)},
    {MP_ROM_QSTR(MP_QSTR_modbus_read), MP_ROM_PTR(&td_modbus_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_modbus_write), MP_ROM_PTR(&td_modbus_write_obj)},
};
static MP_DEFINE_CONST_DICT(td_module_globals, td_module_globals_table);

const mp_obj_module_t td_py_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&td_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_tinydesk, td_py_module);
