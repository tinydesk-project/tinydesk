/*
 * td_py_tdui.c - the _tdui module under tdui (modules/tdui.py): messages
 * to and from the desktop, which makes the windows (apps/pyui.c).
 *
 *   _tdui.open()                          start; OSError without a desktop
 *   _tdui.send(op, id, win, flags, a, b, c, d, e, f, text)
 *   _tdui.recv(timeout_ms)                (op, id, a, b, c, d, text) or None
 *
 * Only a script run on the desktop's own console (the Terminal window, the
 * Editor, Run on the desktop) gets windows; one in an SSH session does not.
 * Its windows close when it ends (td_tdui_release).
 */
#include <string.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "td_apps.h"
#include "td_py.h"

#define DETACH_WAIT_MS 3000

static bool (*s_desktop_check)(void);
static bool s_open;

/* Where recv() puts an event's text: on the script's heap, not in RAM for
 * good. */
MP_REGISTER_ROOT_POINTER(char *td_tdui_text);

void td_py_set_desktop_check(bool (*fn)(void))
{
    s_desktop_check = fn;
}

static mp_obj_t tdui_open(void)
{
    if (s_open)
        return mp_const_none;
    if (!s_desktop_check || !s_desktop_check())
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("tdui: no desktop in this session"));
    if (!td_pyui_attach())
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("tdui: the desktop is not ready (or memory is short)"));
    MP_STATE_VM(td_tdui_text) = m_new(char, TD_PYUI_TEXT_MAX + 1);
    s_open = true;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(tdui_open_obj, tdui_open);

static int16_t arg16(mp_obj_t o)
{
    mp_int_t v = mp_obj_get_int(o);
    return (int16_t)(v < -32768 ? -32768 : v > 32767 ? 32767 : v);
}

/* A full queue (the desktop is busy) is waited out; Ctrl+C still works. */
static mp_obj_t tdui_send(size_t n_args, const mp_obj_t *args)
{
    if (!s_open)
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("tdui: not open"));
    td_pyui_msg_t m = {.op = (uint8_t)mp_obj_get_int(args[0]), .id = (uint16_t)mp_obj_get_int(args[1])};
    if (n_args > 2)
        m.win = (uint16_t)mp_obj_get_int(args[2]);
    if (n_args > 3)
        m.flags = (uint8_t)mp_obj_get_int(args[3]);
    int16_t *v[] = {&m.a, &m.b, &m.c, &m.d, &m.e, &m.f};
    for (size_t i = 4; i < n_args && i < 10; i++)
        *v[i - 4] = arg16(args[i]);
    const char *text = "";
    size_t len = 0;
    if (n_args > 10 && args[10] != mp_const_none)
    {
        mp_buffer_info_t b;
        mp_get_buffer_raise(args[10], &b, MP_BUFFER_READ);   /* str or bytes */
        text = b.buf;
        len = b.len;
    }
    m.len = (uint16_t)(len > TD_PYUI_TEXT_MAX ? TD_PYUI_TEXT_MAX : len);
    while (!td_pyui_send(&m, text))
    {
        mp_event_handle_nowait();
        tdsh_sleep_ms(2);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(tdui_send_obj, 2, 11, tdui_send);

/* Wait up to timeout_ms (0: just look) for the next event. */
static mp_obj_t tdui_recv(mp_obj_t timeout_in)
{
    if (!s_open)
        return mp_const_none;
    mp_int_t timeout = mp_obj_get_int(timeout_in);
    uint64_t end = tdsh_monotonic_ms() + (uint64_t)(timeout > 0 ? timeout : 0);
    td_pyui_msg_t m;
    char *text = MP_STATE_VM(td_tdui_text);
    for (;;)
    {
        if (td_pyui_recv(&m, text, TD_PYUI_TEXT_MAX + 1))
            break;
        mp_event_handle_nowait();
        uint64_t now = tdsh_monotonic_ms();
        if (now >= end)
            return mp_const_none;
        uint64_t left = end - now;
        tdsh_sleep_ms(left < 5 ? (uint32_t)left : 5);
    }
    mp_obj_t t[7] = {
        MP_OBJ_NEW_SMALL_INT(m.op),
        MP_OBJ_NEW_SMALL_INT(m.id),
        MP_OBJ_NEW_SMALL_INT(m.a),
        MP_OBJ_NEW_SMALL_INT(m.b),
        MP_OBJ_NEW_SMALL_INT(m.c),
        MP_OBJ_NEW_SMALL_INT(m.d),
        m.len ? mp_obj_new_str(text, m.len) : mp_const_none,
    };
    return mp_obj_new_tuple(7, t);
}
static MP_DEFINE_CONST_FUN_OBJ_1(tdui_recv_obj, tdui_recv);

void td_tdui_release(void)
{
    if (!s_open)
        return;
    s_open = false;
    td_pyui_detach_begin();
    /* The desktop closes the windows on its next tick. If it does not let go
     * in time, the queues stay allocated until the next script asks. */
    uint64_t end = tdsh_monotonic_ms() + DETACH_WAIT_MS;
    while (!td_pyui_detach_done() && tdsh_monotonic_ms() < end)
        tdsh_sleep_ms(5);
}

static const mp_rom_map_elem_t tdui_module_globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR__tdui)},
    {MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&tdui_open_obj)},
    {MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&tdui_send_obj)},
    {MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&tdui_recv_obj)},
};
static MP_DEFINE_CONST_DICT(tdui_module_globals, tdui_module_globals_table);

const mp_obj_module_t td_tdui_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&tdui_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR__tdui, td_tdui_module);
