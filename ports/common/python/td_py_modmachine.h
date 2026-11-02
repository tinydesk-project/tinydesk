/*
 * td_py_modmachine.h - TinyDesk's part of the machine module, included by
 * extmod/modmachine.c (MICROPY_PY_MACHINE_INCLUDEFILE). The classes are
 * listed in MICROPY_PY_MACHINE_EXTRA_GLOBALS (mpconfigport.h). There is
 * no reset, sleep or clock setting: those belong to TinyDesk.
 */
#include "td_py_machine.h"

static void mp_machine_idle(void)
{
    mp_hal_delay_ms(1);
}

static mp_obj_t td_machine_unique_id(void)
{
    uint8_t id[6];
    if (!td_hw_unique_id(id))
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("machine: no hardware in this program"));
    return mp_obj_new_bytes(id, sizeof(id));
}
static MP_DEFINE_CONST_FUN_OBJ_0(td_machine_unique_id_obj, td_machine_unique_id);

/* freq(): the CPU clock in Hz; setting it is refused. */
static mp_obj_t td_machine_freq(size_t n_args, const mp_obj_t *args)
{
    (void)args;
    if (n_args)
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("machine: the CPU clock is TinyDesk's"));
    uint32_t hz = td_hw_cpu_freq();
    if (!hz)
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("machine: no hardware in this program"));
    return mp_obj_new_int_from_uint(hz);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(td_machine_freq_obj, 0, 1, td_machine_freq);
