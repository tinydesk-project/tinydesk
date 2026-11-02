/*
 * td_py_machine_adc.h - machine.ADC, included by extmod/machine_adc.c
 * (MICROPY_PY_MACHINE_ADC_INCLUDEFILE). 12-bit readings at the widest
 * input range (about 0 to 3.1 V); read_uv() is calibrated where the chip
 * has calibration data.
 */
#include "td_py_machine.h"

#define MICROPY_PY_MACHINE_ADC_CLASS_CONSTANTS

struct _machine_adc_obj_t
{
    mp_obj_base_t base;
    uint8_t pin;
    int8_t handle;
};

static void mp_machine_adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)kind;
    machine_adc_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "ADC(Pin(%u))", self->pin);
}

/* ADC(pin) */
static mp_obj_t mp_machine_adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    int pin = td_machine_pin_arg((void *)args[0], 0);
    int handle = -1;
    td_machine_check(td_hw_adc_open(pin, &handle));
    machine_adc_obj_t *self = mp_obj_malloc(machine_adc_obj_t, type);
    self->pin = (uint8_t)pin;
    self->handle = (int8_t)handle;
    return MP_OBJ_FROM_PTR(self);
}

static mp_int_t mp_machine_adc_read_u16(machine_adc_obj_t *self)
{
    int raw = 0, uv = 0;
    td_machine_check(td_hw_adc_read(self->handle, &raw, &uv));
    return (raw << 4) | (raw >> 8);   /* 12 bits to 16 */
}

static mp_int_t mp_machine_adc_read_uv(machine_adc_obj_t *self)
{
    int raw = 0, uv = 0;
    td_machine_check(td_hw_adc_read(self->handle, &raw, &uv));
    return uv;
}
