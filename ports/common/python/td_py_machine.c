/*
 * td_py_machine.c - the machine module's Pin, hardware I2C and SPI, and the
 * glue the extmod parts need (pins for SoftI2C/SoftSPI, time_pulse_us,
 * bitstream). ADC, PWM and UART are in td_py_machine_*.h, which extmod's
 * machine_adc.c, machine_pwm.c and machine_uart.c include.
 *
 * Only root may touch hardware, pins TinyDesk uses are refused, and what a
 * script set up is undone when it ends (td_hw_release_all). The chip side
 * is behind td_py_hw.h.
 */
#include "td_py_machine.h"

#include <string.h>

#include "extmod/modmachine.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "py/runtime.h"
#include "td_py.h"

/* ------------------------------------------------------------ checks */

void td_machine_need_root(void)
{
    tdsh_session_t *s = td_py_session();
    if (!s || strcmp(s->username, "root") != 0)
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("machine: hardware needs root"));
}

void td_machine_check(const char *err)
{
    if (err)
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("%s"), err);
}

typedef struct
{
    mp_obj_base_t base;
    uint8_t pin;
} machine_pin_obj_t;

int td_machine_pin_arg(void *obj, int output)
{
    mp_obj_t o = (mp_obj_t)obj;
    mp_int_t pin;
    if (mp_obj_is_type(o, &machine_pin_type))
        pin = ((machine_pin_obj_t *)MP_OBJ_TO_PTR(o))->pin;
    else
        pin = mp_obj_get_int(o);
    td_machine_need_root();
    if (pin < 0 || pin >= TD_HW_MAX_PINS)
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin"));
    td_machine_check(td_hw_pin_use((int)pin, output != 0));
    return (int)pin;
}

/* ------------------------------------------------------------- Pin */

MP_REGISTER_ROOT_POINTER(mp_obj_t td_machine_pin_irq[TD_HW_MAX_PINS * 2]);   /* handler, Pin */

static mp_obj_t pin_new(int pin)
{
    machine_pin_obj_t *self = mp_obj_malloc(machine_pin_obj_t, &machine_pin_type);
    self->pin = (uint8_t)pin;
    return MP_OBJ_FROM_PTR(self);
}

static void pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)kind;
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "Pin(%u)", self->pin);
}

/* pin.init(mode=-1, pull=-1, *, value=None) */
static void pin_init_helper(machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum
    {
        ARG_mode,
        ARG_pull,
        ARG_value
    };
    static const mp_arg_t allowed[] = {
        {MP_QSTR_mode, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_pull, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed), allowed, args);
    if (args[ARG_mode].u_obj == mp_const_none && args[ARG_pull].u_obj == mp_const_none &&
        args[ARG_value].u_obj == mp_const_none)
        return;   /* Pin(n): as it is */
    int mode = args[ARG_mode].u_obj == mp_const_none ? TD_HW_IN : mp_obj_get_int(args[ARG_mode].u_obj);
    int pull = args[ARG_pull].u_obj == mp_const_none ? TD_HW_PULL_NONE : mp_obj_get_int(args[ARG_pull].u_obj);
    if (mode != TD_HW_OUT && mode != TD_HW_OPEN_DRAIN)
        mode = TD_HW_IN;
    if (mode != TD_HW_IN)
        td_machine_check(td_hw_pin_use(self->pin, true));
    if (args[ARG_value].u_obj != mp_const_none)
        td_hw_pin_write(self->pin, mp_obj_is_true(args[ARG_value].u_obj));
    td_machine_check(td_hw_pin_mode(self->pin, mode, pull));
}

/* Pin(id, mode=-1, pull=-1, *, value=None) */
static mp_obj_t pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    (void)type;
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    int pin = td_machine_pin_arg((void *)args[0], 0);
    mp_obj_t self = pin_new(pin);
    mp_map_t kw;
    mp_map_init_fixed_table(&kw, n_kw, args + n_args);
    pin_init_helper(MP_OBJ_TO_PTR(self), n_args - 1, args + 1, &kw);
    return self;
}

/* pin() reads, pin(x) writes */
static mp_obj_t pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (n_args == 0)
        return MP_OBJ_NEW_SMALL_INT(td_hw_pin_read(self->pin));
    td_hw_pin_write(self->pin, mp_obj_is_true(args[0]));
    return mp_const_none;
}

static mp_obj_t pin_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args)
{
    pin_init_helper(MP_OBJ_TO_PTR(args[0]), n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pin_init_obj, 1, pin_init);

static mp_obj_t pin_value(size_t n_args, const mp_obj_t *args)
{
    return pin_call(args[0], n_args - 1, 0, args + 1);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pin_value_obj, 1, 2, pin_value);

static mp_obj_t pin_on(mp_obj_t self_in)
{
    td_hw_pin_write(((machine_pin_obj_t *)MP_OBJ_TO_PTR(self_in))->pin, 1);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pin_on_obj, pin_on);

static mp_obj_t pin_off(mp_obj_t self_in)
{
    td_hw_pin_write(((machine_pin_obj_t *)MP_OBJ_TO_PTR(self_in))->pin, 0);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pin_off_obj, pin_off);

static mp_obj_t pin_toggle(mp_obj_t self_in)
{
    int pin = ((machine_pin_obj_t *)MP_OBJ_TO_PTR(self_in))->pin;
    td_hw_pin_write(pin, !td_hw_pin_read(pin));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pin_toggle_obj, pin_toggle);

#define IRQ_RISING  1
#define IRQ_FALLING 2

/* In interrupt context: the handler runs later, between bytecodes. */
static void pin_isr(int pin)
{
    mp_obj_t handler = MP_STATE_PORT(td_machine_pin_irq[pin * 2]);
    if (handler != MP_OBJ_NULL && handler != mp_const_none)
        mp_sched_schedule(handler, MP_STATE_PORT(td_machine_pin_irq[pin * 2 + 1]));
}

/* pin.irq(handler=None, trigger=IRQ_FALLING | IRQ_RISING) */
static mp_obj_t pin_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum
    {
        ARG_handler,
        ARG_trigger
    };
    static const mp_arg_t allowed[] = {
        {MP_QSTR_handler, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_trigger, MP_ARG_INT, {.u_int = IRQ_RISING | IRQ_FALLING}},
    };
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed), allowed, args);
    mp_obj_t handler = args[ARG_handler].u_obj;
    int trigger = (int)args[ARG_trigger].u_int;
    bool on = handler != mp_const_none && (trigger & (IRQ_RISING | IRQ_FALLING));
    MP_STATE_PORT(td_machine_pin_irq[self->pin * 2]) = on ? handler : mp_const_none;
    MP_STATE_PORT(td_machine_pin_irq[self->pin * 2 + 1]) = pos_args[0];
    td_machine_check(td_hw_pin_irq(self->pin, on && (trigger & IRQ_RISING), on && (trigger & IRQ_FALLING),
                                   on ? pin_isr : NULL));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(pin_irq_obj, 1, pin_irq);

static const mp_rom_map_elem_t pin_locals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pin_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&pin_value_obj)},
    {MP_ROM_QSTR(MP_QSTR_on), MP_ROM_PTR(&pin_on_obj)},
    {MP_ROM_QSTR(MP_QSTR_off), MP_ROM_PTR(&pin_off_obj)},
    {MP_ROM_QSTR(MP_QSTR_toggle), MP_ROM_PTR(&pin_toggle_obj)},
    {MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&pin_irq_obj)},
    {MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(TD_HW_IN)},
    {MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(TD_HW_OUT)},
    {MP_ROM_QSTR(MP_QSTR_OPEN_DRAIN), MP_ROM_INT(TD_HW_OPEN_DRAIN)},
    {MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(TD_HW_PULL_UP)},
    {MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(TD_HW_PULL_DOWN)},
    {MP_ROM_QSTR(MP_QSTR_IRQ_RISING), MP_ROM_INT(IRQ_RISING)},
    {MP_ROM_QSTR(MP_QSTR_IRQ_FALLING), MP_ROM_INT(IRQ_FALLING)},
};
static MP_DEFINE_CONST_DICT(pin_locals_dict, pin_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(machine_pin_type, MP_QSTR_Pin, MP_TYPE_FLAG_NONE, make_new, pin_make_new, print, pin_print,
                         call, pin_call, locals_dict, &pin_locals_dict);

void td_machine_release(void)
{
    td_hw_release_all();   /* first: no interrupt sees the table cleared */
    for (int i = 0; i < TD_HW_MAX_PINS * 2; i++)
        MP_STATE_PORT(td_machine_pin_irq[i]) = MP_OBJ_NULL;
}

/* -------------------------------------------------------- bitstream */

void machine_bitstream_high_low(mp_hal_pin_obj_t pin, uint32_t *timing_ns, const uint8_t *buf, size_t len)
{
    td_machine_check(td_hw_bitstream(pin, timing_ns, buf, len));
}

/* ------------------------------------------------------ I2C (hardware) */

typedef struct
{
    mp_obj_base_t base;
    uint8_t id;
    uint8_t scl, sda;
    uint32_t freq, timeout_us;
} machine_i2c_obj_t;

static void i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)kind;
    machine_i2c_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "I2C(%u, scl=%u, sda=%u, freq=%u)", self->id, self->scl, self->sda, (unsigned)self->freq);
}

static void i2c_init(mp_obj_base_t *obj, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    machine_i2c_obj_t *self = (machine_i2c_obj_t *)obj;
    enum
    {
        ARG_scl,
        ARG_sda,
        ARG_freq,
        ARG_timeout
    };
    static const mp_arg_t allowed[] = {
        {MP_QSTR_scl, MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_sda, MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_freq, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 400000}},
        {MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 50000}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed), allowed, args);
    self->scl = (uint8_t)td_machine_pin_arg((void *)args[ARG_scl].u_obj, 1);
    self->sda = (uint8_t)td_machine_pin_arg((void *)args[ARG_sda].u_obj, 1);
    if (args[ARG_freq].u_int <= 0 || args[ARG_freq].u_int > 1000000)
        mp_raise_ValueError(MP_ERROR_TEXT("freq: 1 to 1000000 Hz"));
    self->freq = (uint32_t)args[ARG_freq].u_int;
    self->timeout_us = (uint32_t)args[ARG_timeout].u_int;
    td_machine_check(td_hw_i2c_open(self->id, self->scl, self->sda, self->freq, self->timeout_us));
}

/* I2C(id, *, scl, sda, freq=400000, timeout=50000) */
static mp_obj_t i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    MP_MACHINE_I2C_CHECK_FOR_LEGACY_SOFTI2C_CONSTRUCTION(n_args, n_kw, args);
    mp_arg_check_num(n_args, n_kw, 1, 1, true);
    td_machine_need_root();
    mp_int_t id = mp_obj_get_int(args[0]);
    if (id < 0 || id >= td_hw_i2c_count())
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("I2C(%d) doesn't exist"), (int)id);
    machine_i2c_obj_t *self = mp_obj_malloc(machine_i2c_obj_t, type);
    self->id = (uint8_t)id;
    mp_map_t kw;
    mp_map_init_fixed_table(&kw, n_kw, args + n_args);
    i2c_init(&self->base, n_args - 1, args + 1, &kw);
    return MP_OBJ_FROM_PTR(self);
}

static void i2c_deinit(mp_obj_base_t *obj)
{
    td_hw_i2c_close(((machine_i2c_obj_t *)obj)->id);
}

MP_STATIC_ASSERT(sizeof(td_hw_buf_t) == sizeof(mp_machine_i2c_buf_t));
MP_STATIC_ASSERT(TD_HW_I2C_READ == MP_MACHINE_I2C_FLAG_READ && TD_HW_I2C_STOP == MP_MACHINE_I2C_FLAG_STOP &&
                 TD_HW_I2C_WRITE1 == MP_MACHINE_I2C_FLAG_WRITE1);

static int i2c_transfer(mp_obj_base_t *obj, uint16_t addr, size_t n, mp_machine_i2c_buf_t *bufs, unsigned int flags)
{
    return td_hw_i2c_transfer(((machine_i2c_obj_t *)obj)->id, addr, n, (td_hw_buf_t *)bufs, flags);
}

static const mp_machine_i2c_p_t i2c_p = {
    .transfer_supports_write1 = true,
    .init = i2c_init,
    .deinit = i2c_deinit,
    .transfer = i2c_transfer,
};

MP_DEFINE_CONST_OBJ_TYPE(machine_i2c_type, MP_QSTR_I2C, MP_TYPE_FLAG_NONE, make_new, i2c_make_new, print, i2c_print,
                         protocol, &i2c_p, locals_dict, &mp_machine_i2c_locals_dict);

/* ------------------------------------------------------ SPI (hardware) */

typedef struct
{
    mp_obj_base_t base;
    uint8_t id;
    int8_t sck, mosi, miso;
    uint8_t polarity, phase, firstbit;
    uint32_t baudrate;
} machine_spi_obj_t;

static void spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)kind;
    machine_spi_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "SPI(%u, baudrate=%u, polarity=%u, phase=%u, sck=%d, mosi=%d, miso=%d)", self->id,
              (unsigned)self->baudrate, self->polarity, self->phase, self->sck, self->mosi, self->miso);
}

static int spi_pin(mp_obj_t o, int output)
{
    return o == mp_const_none ? -1 : td_machine_pin_arg((void *)o, output);
}

static void spi_init(mp_obj_base_t *obj, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    machine_spi_obj_t *self = (machine_spi_obj_t *)obj;
    enum
    {
        ARG_baudrate,
        ARG_polarity,
        ARG_phase,
        ARG_bits,
        ARG_firstbit,
        ARG_sck,
        ARG_mosi,
        ARG_miso
    };
    static const mp_arg_t allowed[] = {
        {MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 1000000}},
        {MP_QSTR_polarity, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0}},
        {MP_QSTR_phase, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0}},
        {MP_QSTR_bits, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 8}},
        {MP_QSTR_firstbit, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = MICROPY_PY_MACHINE_SPI_MSB}},
        {MP_QSTR_sck, MP_ARG_KW_ONLY | MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_mosi, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_miso, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed), allowed, args);
    if (args[ARG_bits].u_int != 8)
        mp_raise_ValueError(MP_ERROR_TEXT("bits: only 8"));
    self->sck = (int8_t)spi_pin(args[ARG_sck].u_obj, 1);
    self->mosi = (int8_t)spi_pin(args[ARG_mosi].u_obj, 1);
    self->miso = (int8_t)spi_pin(args[ARG_miso].u_obj, 0);
    self->baudrate = (uint32_t)args[ARG_baudrate].u_int;
    self->polarity = args[ARG_polarity].u_int ? 1 : 0;
    self->phase = args[ARG_phase].u_int ? 1 : 0;
    self->firstbit = (uint8_t)args[ARG_firstbit].u_int;
    td_machine_check(td_hw_spi_open(self->id, self->sck, self->mosi, self->miso, self->baudrate, self->polarity,
                                    self->phase, self->firstbit == MICROPY_PY_MACHINE_SPI_LSB));
}

/* SPI(id, baudrate=1000000, *, polarity=0, phase=0, bits=8, firstbit=MSB, sck, mosi=None, miso=None) */
static mp_obj_t spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    MP_MACHINE_SPI_CHECK_FOR_LEGACY_SOFTSPI_CONSTRUCTION(n_args, n_kw, args);
    mp_arg_check_num(n_args, n_kw, 1, 2, true);
    td_machine_need_root();
    mp_int_t id = mp_obj_get_int(args[0]);
    if (id < 1 || id > td_hw_spi_count())
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("SPI(%d) doesn't exist"), (int)id);
    machine_spi_obj_t *self = mp_obj_malloc(machine_spi_obj_t, type);
    self->id = (uint8_t)id;
    mp_map_t kw;
    mp_map_init_fixed_table(&kw, n_kw, args + n_args);
    spi_init(&self->base, n_args - 1, args + 1, &kw);
    return MP_OBJ_FROM_PTR(self);
}

static void spi_deinit(mp_obj_base_t *obj)
{
    td_hw_spi_close(((machine_spi_obj_t *)obj)->id);
}

static void spi_transfer(mp_obj_base_t *obj, size_t len, const uint8_t *src, uint8_t *dest)
{
    td_machine_check(td_hw_spi_transfer(((machine_spi_obj_t *)obj)->id, len, src, dest));
}

static const mp_machine_spi_p_t spi_p = {
    .init = spi_init,
    .deinit = spi_deinit,
    .transfer = spi_transfer,
};

MP_DEFINE_CONST_OBJ_TYPE(machine_spi_type, MP_QSTR_SPI, MP_TYPE_FLAG_NONE, make_new, spi_make_new, print, spi_print,
                         protocol, &spi_p, locals_dict, &mp_machine_spi_locals_dict);
