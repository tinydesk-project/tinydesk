/*
 * td_py_machine_pwm.h - machine.PWM, included by extmod/machine_pwm.c
 * (MICROPY_PY_MACHINE_PWM_INCLUDEFILE). The chip's LED PWM controller;
 * outputs with the same frequency share a timer.
 */
#include "td_py_machine.h"

struct _machine_pwm_obj_t
{
    mp_obj_base_t base;
    uint8_t pin;
    int8_t handle;   /* -1: deinit */
};

static void mp_machine_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)kind;
    machine_pwm_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->handle < 0)
        mp_printf(print, "PWM(Pin(%u))", self->pin);
    else
        mp_printf(print, "PWM(Pin(%u), freq=%u, duty_u16=%u)", self->pin, (unsigned)td_hw_pwm_freq(self->handle),
                  (unsigned)td_hw_pwm_duty(self->handle));
}

static void pwm_open(machine_pwm_obj_t *self, uint32_t freq)
{
    int handle = -1;
    td_machine_check(td_hw_pwm_open(self->pin, freq, &handle));
    self->handle = (int8_t)handle;
}

static void pwm_need_open(machine_pwm_obj_t *self)
{
    if (self->handle < 0)
        pwm_open(self, 1000);
}

static void mp_machine_pwm_duty_set_u16(machine_pwm_obj_t *self, mp_int_t duty_u16)
{
    if (duty_u16 < 0 || duty_u16 > 65535)
        mp_raise_ValueError(MP_ERROR_TEXT("duty_u16: 0 to 65535"));
    pwm_need_open(self);
    td_hw_pwm_set_duty(self->handle, (uint32_t)duty_u16);
}

static void mp_machine_pwm_duty_set_ns(machine_pwm_obj_t *self, mp_int_t duty_ns)
{
    pwm_need_open(self);
    uint64_t d = (uint64_t)(duty_ns < 0 ? 0 : duty_ns) * td_hw_pwm_freq(self->handle) * 65536u / 1000000000u;
    mp_machine_pwm_duty_set_u16(self, d > 65535 ? 65535 : (mp_int_t)d);
}

/* init(freq=..., duty_u16=..., duty_ns=...) */
static void mp_machine_pwm_init_helper(machine_pwm_obj_t *self, size_t n_args, const mp_obj_t *pos_args,
                                       mp_map_t *kw_args)
{
    enum
    {
        ARG_freq,
        ARG_duty_u16,
        ARG_duty_ns
    };
    static const mp_arg_t allowed[] = {
        {MP_QSTR_freq, MP_ARG_INT, {.u_int = -1}},
        {MP_QSTR_duty_u16, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
        {MP_QSTR_duty_ns, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = -1}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed), allowed, args);
    mp_int_t freq = args[ARG_freq].u_int;
    if (freq == 0 || freq > 40000000)
        mp_raise_ValueError(MP_ERROR_TEXT("freq: 1 Hz to 40 MHz"));
    if (self->handle < 0)
        pwm_open(self, freq > 0 ? (uint32_t)freq : 1000);
    else if (freq > 0)
        td_machine_check(td_hw_pwm_set_freq(self->handle, (uint32_t)freq));
    if (args[ARG_duty_u16].u_int >= 0)
        mp_machine_pwm_duty_set_u16(self, args[ARG_duty_u16].u_int);
    else if (args[ARG_duty_ns].u_int >= 0)
        mp_machine_pwm_duty_set_ns(self, args[ARG_duty_ns].u_int);
}

/* PWM(pin, freq=1000, *, duty_u16=0, duty_ns=...) */
static mp_obj_t mp_machine_pwm_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 1, 2, true);
    int pin = td_machine_pin_arg((void *)args[0], 1);
    machine_pwm_obj_t *self = mp_obj_malloc(machine_pwm_obj_t, type);
    self->pin = (uint8_t)pin;
    self->handle = -1;
    mp_map_t kw;
    mp_map_init_fixed_table(&kw, n_kw, args + n_args);
    mp_machine_pwm_init_helper(self, n_args - 1, args + 1, &kw);
    return MP_OBJ_FROM_PTR(self);
}

static void mp_machine_pwm_deinit(machine_pwm_obj_t *self)
{
    if (self->handle >= 0)
        td_hw_pwm_close(self->handle);
    self->handle = -1;
}

static mp_obj_t mp_machine_pwm_freq_get(machine_pwm_obj_t *self)
{
    pwm_need_open(self);
    return MP_OBJ_NEW_SMALL_INT(td_hw_pwm_freq(self->handle));
}

static void mp_machine_pwm_freq_set(machine_pwm_obj_t *self, mp_int_t freq)
{
    if (freq <= 0 || freq > 40000000)
        mp_raise_ValueError(MP_ERROR_TEXT("freq: 1 Hz to 40 MHz"));
    pwm_need_open(self);
    td_machine_check(td_hw_pwm_set_freq(self->handle, (uint32_t)freq));
}

static mp_obj_t mp_machine_pwm_duty_get_u16(machine_pwm_obj_t *self)
{
    pwm_need_open(self);
    return MP_OBJ_NEW_SMALL_INT(td_hw_pwm_duty(self->handle));
}

static mp_obj_t mp_machine_pwm_duty_get_ns(machine_pwm_obj_t *self)
{
    pwm_need_open(self);
    uint32_t f = td_hw_pwm_freq(self->handle);
    uint64_t ns = f ? (uint64_t)td_hw_pwm_duty(self->handle) * 1000000000u / f / 65536u : 0;
    return mp_obj_new_int_from_uint((mp_uint_t)ns);
}
