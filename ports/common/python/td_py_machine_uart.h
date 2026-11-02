/*
 * td_py_machine_uart.h - machine.UART, included by extmod/machine_uart.c
 * (MICROPY_PY_MACHINE_UART_INCLUDEFILE). Only a UART that is neither the
 * console nor an RS-485 line; tx and rx pins are given by the script.
 * Waiting for data sleeps in short slices, so Ctrl+C still works.
 */
#include "py/mperrno.h"
#include "py/stream.h"
#include "td_py_machine.h"

#define MICROPY_PY_MACHINE_UART_CLASS_CONSTANTS

struct _machine_uart_obj_t
{
    mp_obj_base_t base;
    uint8_t id;
    bool open;
    int8_t tx, rx;
    uint32_t baudrate;
    uint8_t bits, parity, stop;
    uint16_t rxbuf;
    uint16_t timeout, timeout_char;   /* ms */
};

static void mp_machine_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)kind;
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    static const char *const parity[] = {"None", "1", "0"};
    mp_printf(print, "UART(%u, baudrate=%u, bits=%u, parity=%s, stop=%u, tx=%d, rx=%d, timeout=%u)", self->id,
              (unsigned)self->baudrate, self->bits, parity[self->parity], self->stop, self->tx, self->rx,
              self->timeout);
}

static void mp_machine_uart_init_helper(machine_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args,
                                        mp_map_t *kw_args)
{
    enum
    {
        ARG_baudrate,
        ARG_bits,
        ARG_parity,
        ARG_stop,
        ARG_tx,
        ARG_rx,
        ARG_rxbuf,
        ARG_timeout,
        ARG_timeout_char
    };
    static const mp_arg_t allowed[] = {
        {MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 115200}},
        {MP_QSTR_bits, MP_ARG_INT, {.u_int = 8}},
        {MP_QSTR_parity, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_stop, MP_ARG_INT, {.u_int = 1}},
        {MP_QSTR_tx, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_rx, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_rxbuf, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 512}},
        {MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0}},
        {MP_QSTR_timeout_char, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 2}},
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed), allowed, args);
    mp_obj_t tx = args[ARG_tx].u_obj, rx = args[ARG_rx].u_obj;
    if (tx == mp_const_none && rx == mp_const_none)
        mp_raise_ValueError(MP_ERROR_TEXT("give tx=, rx= or both"));
    if (args[ARG_bits].u_int < 5 || args[ARG_bits].u_int > 8)
        mp_raise_ValueError(MP_ERROR_TEXT("bits: 5 to 8"));
    if (args[ARG_stop].u_int != 1 && args[ARG_stop].u_int != 2)
        mp_raise_ValueError(MP_ERROR_TEXT("stop: 1 or 2"));
    self->tx = (int8_t)(tx == mp_const_none ? -1 : td_machine_pin_arg((void *)tx, 1));
    self->rx = (int8_t)(rx == mp_const_none ? -1 : td_machine_pin_arg((void *)rx, 0));
    self->baudrate = (uint32_t)args[ARG_baudrate].u_int;
    self->bits = (uint8_t)args[ARG_bits].u_int;
    /* parity: None, 1 (odd) or 0 (even) */
    self->parity = args[ARG_parity].u_obj == mp_const_none ? 0 : mp_obj_get_int(args[ARG_parity].u_obj) & 1 ? 1
                                                                                                            : 2;
    self->stop = (uint8_t)args[ARG_stop].u_int;
    self->rxbuf = (uint16_t)(args[ARG_rxbuf].u_int < 256 ? 256 : args[ARG_rxbuf].u_int > 8192 ? 8192
                                                                                              : args[ARG_rxbuf].u_int);
    self->timeout = (uint16_t)args[ARG_timeout].u_int;
    self->timeout_char = (uint16_t)args[ARG_timeout_char].u_int;
    td_machine_check(td_hw_uart_open(self->id, self->tx, self->rx, self->baudrate, self->bits, self->parity,
                                     self->stop, self->rxbuf));
    self->open = true;
}

/* UART(id, baudrate=115200, bits=8, parity=None, stop=1, *, tx, rx, ...) */
static mp_obj_t mp_machine_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    td_machine_need_root();
    mp_int_t id = mp_obj_get_int(args[0]);
    if (id < 0 || id >= td_hw_uart_count())
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("UART(%d) doesn't exist"), (int)id);
    machine_uart_obj_t *self = mp_obj_malloc(machine_uart_obj_t, type);
    self->id = (uint8_t)id;
    self->open = false;
    mp_map_t kw;
    mp_map_init_fixed_table(&kw, n_kw, args + n_args);
    mp_machine_uart_init_helper(self, n_args - 1, args + 1, &kw);
    return MP_OBJ_FROM_PTR(self);
}

static void mp_machine_uart_deinit(machine_uart_obj_t *self)
{
    if (self->open)
        td_hw_uart_close(self->id);
    self->open = false;
}

static void uart_need_open(machine_uart_obj_t *self)
{
    if (!self->open)
        mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("UART is closed"));
}

static mp_int_t mp_machine_uart_any(machine_uart_obj_t *self)
{
    uart_need_open(self);
    return td_hw_uart_any(self->id);
}

static bool mp_machine_uart_txdone(machine_uart_obj_t *self)
{
    uart_need_open(self);
    return td_hw_uart_txdone(self->id);
}

/* Up to `size` bytes: waits up to `timeout` ms for the first, then
 * `timeout_char` ms for each next one. */
static mp_uint_t mp_machine_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode)
{
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uart_need_open(self);
    uint8_t *buf = buf_in;
    mp_uint_t got = 0;
    mp_uint_t start = mp_hal_ticks_ms();
    while (got < size)
    {
        int wait = got ? self->timeout_char : self->timeout;
        int n = td_hw_uart_read(self->id, buf + got, size - got, 0);
        if (n > 0)
        {
            got += (mp_uint_t)n;
            start = mp_hal_ticks_ms();
            continue;
        }
        if ((int)(mp_hal_ticks_ms() - start) >= wait)
            break;
        mp_hal_delay_ms(wait < 10 ? 1 : 5);   /* also handles Ctrl+C */
    }
    if (got == 0 && size > 0)
    {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }
    return got;
}

static mp_uint_t mp_machine_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode)
{
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uart_need_open(self);
    int n = td_hw_uart_write(self->id, buf_in, size);
    if (n < 0)
    {
        *errcode = MP_EIO;
        return MP_STREAM_ERROR;
    }
    return (mp_uint_t)n;
}

static mp_uint_t mp_machine_uart_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode)
{
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (request == MP_STREAM_POLL)
    {
        mp_uint_t ret = 0;
        if ((arg & MP_STREAM_POLL_RD) && self->open && td_hw_uart_any(self->id) > 0)
            ret |= MP_STREAM_POLL_RD;
        if ((arg & MP_STREAM_POLL_WR) && self->open)
            ret |= MP_STREAM_POLL_WR;
        return ret;
    }
    if (request == MP_STREAM_FLUSH)
    {
        uart_need_open(self);
        while (!td_hw_uart_txdone(self->id))
            mp_hal_delay_ms(1);
        return 0;
    }
    *errcode = MP_EINVAL;
    return MP_STREAM_ERROR;
}
