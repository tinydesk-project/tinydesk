/*
 * td_py_hw.h - what the machine module needs from the chip.
 *
 * Plain C without ESP-IDF headers, so MicroPython's generator (which
 * preprocesses the port files on a PC) can read the files that use it.
 * hw/td_py_hw_esp.c implements it on the boards; hw/td_py_hw_host.c in the
 * PC programs, where every call says there is no hardware.
 *
 * Functions returning const char * give NULL on success, else a message
 * for the script's exception. Everything a script opens is remembered and
 * closed by td_hw_release_all() when it ends.
 */
#ifndef TD_PY_HW_H
#define TD_PY_HW_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* GPIO numbers are below this on every supported chip. */
#define TD_HW_MAX_PINS 49

/* ------------------------------------------------------------ pins */

enum
{
    TD_HW_IN,
    TD_HW_OUT,
    TD_HW_OPEN_DRAIN
};
enum
{
    TD_HW_PULL_NONE,
    TD_HW_PULL_UP,
    TD_HW_PULL_DOWN
};

/* May the script use the pin (as an output)? Refuses pins the chip, the
 * desktop's link or the board configuration use; prints a warning, once,
 * for a strapping pin. The pin is reset when the script ends. */
const char *td_hw_pin_use(int pin, bool output);
const char *td_hw_pin_mode(int pin, int mode, int pull);
void td_hw_pin_write(int pin, int value);
int td_hw_pin_read(int pin);

/* Edge interrupts: isr(pin) runs in interrupt context. isr NULL: off. */
const char *td_hw_pin_irq(int pin, bool rising, bool falling, void (*isr)(int pin));

/* ---------------------------------------------------- ADC (12-bit) */

/* *handle receives the handle for td_hw_adc_read(). */
const char *td_hw_adc_open(int pin, int *handle);
const char *td_hw_adc_read(int handle, int *raw, int *microvolts);

/* --------------------------------------------------------- PWM */

/* A PWM output on pin (an already open one is returned again). */
const char *td_hw_pwm_open(int pin, uint32_t freq, int *handle);
const char *td_hw_pwm_set_freq(int handle, uint32_t freq);
uint32_t td_hw_pwm_freq(int handle);
void td_hw_pwm_set_duty(int handle, uint32_t duty_u16);
uint32_t td_hw_pwm_duty(int handle);
void td_hw_pwm_close(int handle);

/* ---------------------------------------------------------- I2C */

/* Same layout as MicroPython's mp_machine_i2c_buf_t. */
typedef struct
{
    size_t len;
    uint8_t *buf;
} td_hw_buf_t;

#define TD_HW_I2C_READ   0x01
#define TD_HW_I2C_STOP   0x02
#define TD_HW_I2C_WRITE1 0x04

int td_hw_i2c_count(void);
const char *td_hw_i2c_open(int id, int scl, int sda, uint32_t freq, uint32_t timeout_us);
/* Bytes moved, or a negative MP_E* error number. */
int td_hw_i2c_transfer(int id, uint16_t addr, size_t n, td_hw_buf_t *bufs, unsigned flags);
void td_hw_i2c_close(int id);

/* ---------------------------------------------------------- SPI */

int td_hw_spi_count(void);   /* ids 1..count */
const char *td_hw_spi_open(int id, int sck, int mosi, int miso, uint32_t baud, int polarity, int phase, bool lsb_first);
const char *td_hw_spi_transfer(int id, size_t len, const uint8_t *src, uint8_t *dest);
void td_hw_spi_close(int id);

/* --------------------------------------------------------- UART */

int td_hw_uart_count(void);
/* parity: 0 none, 1 odd, 2 even; stop: 1 or 2. */
const char *td_hw_uart_open(int id, int tx, int rx, uint32_t baud, int bits, int parity, int stop, int rxbuf);
int td_hw_uart_read(int id, uint8_t *buf, size_t len, int timeout_ms);
int td_hw_uart_write(int id, const uint8_t *buf, size_t len);
int td_hw_uart_any(int id);
bool td_hw_uart_txdone(int id);
void td_hw_uart_close(int id);

/* ----------------------------------------------------------- misc */

/* WS2812-style pulses: timing_ns = high0, low0, high1, low1. */
const char *td_hw_bitstream(int pin, const uint32_t *timing_ns, const uint8_t *buf, size_t len);

/* CPU frequency in Hz (0: unknown); a 6-byte factory ID (false: none). */
uint32_t td_hw_cpu_freq(void);
bool td_hw_unique_id(uint8_t id[6]);

/* Short critical section, also safe against the pin interrupts. */
uint32_t td_hw_atomic_begin(void);
void td_hw_atomic_end(uint32_t state);

/* The script ended: close and reset everything it opened. */
void td_hw_release_all(void);

#endif /* TD_PY_HW_H */
