/*
 * td_py_hw_host.c - td_py_hw.h for the PC programs: there is no hardware,
 * so `import machine` works and using a pin or a bus raises OSError.
 */
#include "td_py_hw.h"

static const char s_none[] = "machine: no hardware in this program";

const char *td_hw_pin_use(int pin, bool output)
{
    (void)pin;
    (void)output;
    return s_none;
}

const char *td_hw_pin_mode(int pin, int mode, int pull)
{
    (void)pin;
    (void)mode;
    (void)pull;
    return s_none;
}

void td_hw_pin_write(int pin, int value)
{
    (void)pin;
    (void)value;
}

int td_hw_pin_read(int pin)
{
    (void)pin;
    return 0;
}

const char *td_hw_pin_irq(int pin, bool rising, bool falling, void (*isr)(int pin))
{
    (void)pin;
    (void)rising;
    (void)falling;
    (void)isr;
    return s_none;
}

const char *td_hw_adc_open(int pin, int *handle)
{
    (void)pin;
    (void)handle;
    return s_none;
}

const char *td_hw_adc_read(int handle, int *raw, int *microvolts)
{
    (void)handle;
    (void)raw;
    (void)microvolts;
    return s_none;
}

const char *td_hw_pwm_open(int pin, uint32_t freq, int *handle)
{
    (void)pin;
    (void)freq;
    (void)handle;
    return s_none;
}

const char *td_hw_pwm_set_freq(int handle, uint32_t freq)
{
    (void)handle;
    (void)freq;
    return s_none;
}

uint32_t td_hw_pwm_freq(int handle)
{
    (void)handle;
    return 0;
}

void td_hw_pwm_set_duty(int handle, uint32_t duty_u16)
{
    (void)handle;
    (void)duty_u16;
}

uint32_t td_hw_pwm_duty(int handle)
{
    (void)handle;
    return 0;
}

void td_hw_pwm_close(int handle)
{
    (void)handle;
}

int td_hw_i2c_count(void)
{
    return 1;   /* so I2C(0, ...) gets as far as saying there is no hardware */
}

const char *td_hw_i2c_open(int id, int scl, int sda, uint32_t freq, uint32_t timeout_us)
{
    (void)id;
    (void)scl;
    (void)sda;
    (void)freq;
    (void)timeout_us;
    return s_none;
}

int td_hw_i2c_transfer(int id, uint16_t addr, size_t n, td_hw_buf_t *bufs, unsigned flags)
{
    (void)id;
    (void)addr;
    (void)n;
    (void)bufs;
    (void)flags;
    return -5;   /* MP_EIO */
}

void td_hw_i2c_close(int id)
{
    (void)id;
}

int td_hw_spi_count(void)
{
    return 1;
}

const char *td_hw_spi_open(int id, int sck, int mosi, int miso, uint32_t baud, int polarity, int phase, bool lsb_first)
{
    (void)id;
    (void)sck;
    (void)mosi;
    (void)miso;
    (void)baud;
    (void)polarity;
    (void)phase;
    (void)lsb_first;
    return s_none;
}

const char *td_hw_spi_transfer(int id, size_t len, const uint8_t *src, uint8_t *dest)
{
    (void)id;
    (void)len;
    (void)src;
    (void)dest;
    return s_none;
}

void td_hw_spi_close(int id)
{
    (void)id;
}

int td_hw_uart_count(void)
{
    return 1;
}

const char *td_hw_uart_open(int id, int tx, int rx, uint32_t baud, int bits, int parity, int stop, int rxbuf)
{
    (void)id;
    (void)tx;
    (void)rx;
    (void)baud;
    (void)bits;
    (void)parity;
    (void)stop;
    (void)rxbuf;
    return s_none;
}

int td_hw_uart_read(int id, uint8_t *buf, size_t len, int timeout_ms)
{
    (void)id;
    (void)buf;
    (void)len;
    (void)timeout_ms;
    return 0;
}

int td_hw_uart_write(int id, const uint8_t *buf, size_t len)
{
    (void)id;
    (void)buf;
    (void)len;
    return -1;
}

int td_hw_uart_any(int id)
{
    (void)id;
    return 0;
}

bool td_hw_uart_txdone(int id)
{
    (void)id;
    return true;
}

void td_hw_uart_close(int id)
{
    (void)id;
}

const char *td_hw_bitstream(int pin, const uint32_t *timing_ns, const uint8_t *buf, size_t len)
{
    (void)pin;
    (void)timing_ns;
    (void)buf;
    (void)len;
    return s_none;
}

uint32_t td_hw_cpu_freq(void)
{
    return 0;
}

bool td_hw_unique_id(uint8_t id[6])
{
    (void)id;
    return false;
}

uint32_t td_hw_atomic_begin(void)
{
    return 0;   /* no interrupts call into the interpreter here */
}

void td_hw_atomic_end(uint32_t state)
{
    (void)state;
}

void td_hw_release_all(void)
{
}
