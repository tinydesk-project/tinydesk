/*
 * td_py_hw_esp.c - td_py_hw.h on the ESP32 boards (ESP-IDF drivers).
 *
 * The rules for pins: a script may not use a pin the chip reserved (flash,
 * PSRAM, a driver's), the desktop's link (UART0 on the classic ESP32, USB
 * on the ESP32-C6), or any pin of the board configuration (console.*,
 * rs485.*, eth.*, sd.*). Strapping pins get a warning. Peripherals the
 * firmware has open (a UART with a driver, an initialised SPI bus) are
 * refused too. Everything is undone by td_hw_release_all().
 *
 * Runs in the shell task of the one script that runs; interrupts only
 * call the interpreter's scheduler.
 */
#include "td_py_hw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "driver/ledc.h"
#include "driver/rmt_tx.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_private/esp_clk.h"
#include "esp_private/esp_gpio_reserve.h"
#include "esp_private/spi_common_internal.h"
#include "freertos/FreeRTOS.h"
#include "td_py_hw_pins.h"

static char s_msg[112];
static uint64_t s_used;     /* pins this script may use */
static uint64_t s_touched;  /* pins it changed: reset at the end */
static uint64_t s_warned;   /* strapping pins it was warned about */

#define PIN_BIT(pin) (1ULL << (pin))

static const char *fail(const char *fmt, int a, const char *b)
{
    snprintf(s_msg, sizeof(s_msg), fmt, a, b ? b : "");
    return s_msg;
}

static const char *esp_fail(const char *what, esp_err_t err)
{
    snprintf(s_msg, sizeof(s_msg), "machine: %s: %s", what, esp_err_to_name(err));
    return s_msg;
}

/* ------------------------------------------------------------ pins */

#if CONFIG_IDF_TARGET_ESP32
#define CHIP TD_HW_CHIP_ESP32
#elif CONFIG_IDF_TARGET_ESP32C6
#define CHIP TD_HW_CHIP_ESP32C6
#else
#define CHIP TD_HW_CHIP_OTHER
#endif

/* The drivers' notes (pin set-ups and the like) would go to the script's
 * output: only their warnings and errors. */
static void quiet_logs(void)
{
    static bool done;
    if (done)
        return;
    static const char *const tags[] = {"gpio", "ledc", "i2c.master", "spi_master", "uart", "rmt", "adc_cali"};
    for (size_t i = 0; i < sizeof(tags) / sizeof(tags[0]); i++)
        esp_log_level_set(tags[i], ESP_LOG_WARN);
    done = true;
}

const char *td_hw_pin_use(int pin, bool output)
{
    quiet_logs();
    if (pin < 0 || pin >= GPIO_NUM_MAX || !GPIO_IS_VALID_GPIO(pin))
        return fail("machine: there is no GPIO %d", pin, NULL);
    if (output && !GPIO_IS_VALID_OUTPUT_GPIO(pin))
        return fail("machine: GPIO %d is input-only", pin, NULL);
    if (s_used & PIN_BIT(pin))
        return NULL;
    if (esp_gpio_is_reserved(PIN_BIT(pin)))
        return fail("machine: GPIO %d is used by the chip (flash, PSRAM or a driver)", pin, NULL);
    const char *owner = td_hw_pin_owner(pin, CHIP);
    if (owner)
        return fail("machine: GPIO %d is used by TinyDesk (%s)", pin, owner);
    if (td_hw_pin_strapping(pin, CHIP) && !(s_warned & PIN_BIT(pin)))
    {
        printf("machine: warning: GPIO %d is a strapping pin: what is wired to it can keep the board "
               "from starting\n",
               pin);
        s_warned |= PIN_BIT(pin);
    }
    s_used |= PIN_BIT(pin);
    return NULL;
}

static gpio_int_type_t s_irq_type[GPIO_NUM_MAX];

const char *td_hw_pin_mode(int pin, int mode, int pull)
{
    if (pin < 0 || pin >= GPIO_NUM_MAX || !(s_used & PIN_BIT(pin)))
        return "machine: pin not checked";
    if (mode != TD_HW_IN && !GPIO_IS_VALID_OUTPUT_GPIO(pin))
        return fail("machine: GPIO %d is input-only", pin, NULL);
    s_touched |= PIN_BIT(pin);
    gpio_config_t c = {
        .pin_bit_mask = PIN_BIT(pin),
        .mode = mode == TD_HW_OUT          ? GPIO_MODE_INPUT_OUTPUT
                : mode == TD_HW_OPEN_DRAIN ? GPIO_MODE_INPUT_OUTPUT_OD
                                           : GPIO_MODE_INPUT,
        .pull_up_en = pull == TD_HW_PULL_UP ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = pull == TD_HW_PULL_DOWN ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type = s_irq_type[pin],
    };
    esp_err_t err = gpio_config(&c);
    return err == ESP_OK ? NULL : esp_fail("Pin", err);
}

void td_hw_pin_write(int pin, int value)
{
    if (pin >= 0 && pin < GPIO_NUM_MAX && (s_used & PIN_BIT(pin)))
    {
        s_touched |= PIN_BIT(pin);
        gpio_set_level(pin, value ? 1 : 0);
    }
}

int td_hw_pin_read(int pin)
{
    return pin >= 0 && pin < GPIO_NUM_MAX && (s_used & PIN_BIT(pin)) ? gpio_get_level(pin) : 0;
}

static void (*s_isr)(int pin);
static uint64_t s_irq_pins;
static bool s_isr_service;   /* installed by us */

static void pin_isr(void *arg)
{
    void (*isr)(int) = s_isr;
    if (isr)
        isr((int)(intptr_t)arg);
}

const char *td_hw_pin_irq(int pin, bool rising, bool falling, void (*isr)(int pin))
{
    if (pin < 0 || pin >= GPIO_NUM_MAX || !(s_used & PIN_BIT(pin)))
        return "machine: pin not checked";
    if (s_irq_pins & PIN_BIT(pin))
    {
        gpio_intr_disable(pin);
        gpio_isr_handler_remove(pin);
        s_irq_pins &= ~PIN_BIT(pin);
    }
    s_touched |= PIN_BIT(pin);
    s_irq_type[pin] = GPIO_INTR_DISABLE;
    gpio_set_intr_type(pin, GPIO_INTR_DISABLE);
    if (!isr || (!rising && !falling))
        return NULL;
    if (!s_isr_service && !s_irq_pins)
    {
        esp_err_t err = gpio_install_isr_service(0);
        if (err == ESP_OK)
            s_isr_service = true;
        else if (err != ESP_ERR_INVALID_STATE)   /* someone else's: share it */
            return esp_fail("Pin.irq", err);
    }
    s_isr = isr;
    s_irq_type[pin] = rising && falling ? GPIO_INTR_ANYEDGE : rising ? GPIO_INTR_POSEDGE
                                                                     : GPIO_INTR_NEGEDGE;
    gpio_set_intr_type(pin, s_irq_type[pin]);
    esp_err_t err = gpio_isr_handler_add(pin, pin_isr, (void *)(intptr_t)pin);
    if (err != ESP_OK)
        return esp_fail("Pin.irq", err);
    s_irq_pins |= PIN_BIT(pin);
    gpio_intr_enable(pin);
    return NULL;
}

/* ------------------------------------------------------------- ADC */

#define ADC_MAX 10

static adc_oneshot_unit_handle_t s_adc;
static struct
{
    int8_t pin;
    adc_channel_t channel;
    adc_cali_handle_t cali;
} s_adc_ch[ADC_MAX];

const char *td_hw_adc_open(int pin, int *handle)
{
    adc_unit_t unit;
    adc_channel_t channel;
    if (adc_oneshot_io_to_channel(pin, &unit, &channel) != ESP_OK)
        return fail("machine: GPIO %d has no ADC", pin, NULL);
    if (unit != ADC_UNIT_1)
        return fail("machine: GPIO %d is on ADC2, which Wi-Fi uses: take an ADC1 pin", pin, NULL);
    int free_slot = -1;
    for (int i = 0; i < ADC_MAX; i++)
    {
        if (s_adc_ch[i].pin == pin + 1)
        {
            *handle = i;
            return NULL;
        }
        if (!s_adc_ch[i].pin && free_slot < 0)
            free_slot = i;
    }
    if (free_slot < 0)
        return "machine: too many ADC inputs";
    if (!s_adc)
    {
        adc_oneshot_unit_init_cfg_t u = {.unit_id = ADC_UNIT_1, .ulp_mode = ADC_ULP_MODE_DISABLE};
        esp_err_t err = adc_oneshot_new_unit(&u, &s_adc);
        if (err != ESP_OK)
            return esp_fail("ADC", err);
    }
    adc_oneshot_chan_cfg_t c = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12};
    esp_err_t err = adc_oneshot_config_channel(s_adc, channel, &c);
    if (err != ESP_OK)
        return esp_fail("ADC", err);
    s_adc_ch[free_slot].pin = (int8_t)(pin + 1);   /* 0: free */
    s_adc_ch[free_slot].channel = channel;
    s_adc_ch[free_slot].cali = NULL;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cc = {
        .unit_id = ADC_UNIT_1, .chan = channel, .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12};
    adc_cali_create_scheme_curve_fitting(&cc, &s_adc_ch[free_slot].cali);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t lc = {.unit_id = ADC_UNIT_1, .atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_12};
    adc_cali_create_scheme_line_fitting(&lc, &s_adc_ch[free_slot].cali);
#endif
    *handle = free_slot;
    return NULL;
}

const char *td_hw_adc_read(int handle, int *raw, int *microvolts)
{
    if (handle < 0 || handle >= ADC_MAX || !s_adc_ch[handle].pin || !s_adc)
        return "machine: ADC is closed";
    esp_err_t err = adc_oneshot_read(s_adc, s_adc_ch[handle].channel, raw);
    if (err != ESP_OK)
        return esp_fail("ADC", err);
    int mv = 0;
    if (s_adc_ch[handle].cali && adc_cali_raw_to_voltage(s_adc_ch[handle].cali, *raw, &mv) == ESP_OK)
        *microvolts = mv * 1000;
    else
        *microvolts = (int)((int64_t)*raw * 3100000 / 4095);   /* uncalibrated */
    return NULL;
}

static void adc_release(void)
{
    for (int i = 0; i < ADC_MAX; i++)
    {
        if (s_adc_ch[i].cali)
        {
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
            adc_cali_delete_scheme_curve_fitting(s_adc_ch[i].cali);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
            adc_cali_delete_scheme_line_fitting(s_adc_ch[i].cali);
#endif
        }
        s_adc_ch[i].pin = 0;
        s_adc_ch[i].cali = NULL;
    }
    if (s_adc)
        adc_oneshot_del_unit(s_adc);
    s_adc = NULL;
}

/* ------------------------------------------------------------- PWM */

#define PWM_CHANNELS SOC_LEDC_CHANNEL_NUM
#define PWM_TIMERS   LEDC_TIMER_MAX

static struct
{
    int8_t pin;   /* -1: free */
    int8_t timer;
    uint32_t duty_u16;
} s_pwm[PWM_CHANNELS];
static struct
{
    uint32_t freq;
    uint8_t bits;
    uint8_t users;
} s_timer[PWM_TIMERS];
static bool s_pwm_ready;

static void pwm_ready(void)
{
    if (s_pwm_ready)
        return;
    for (int i = 0; i < PWM_CHANNELS; i++)
        s_pwm[i].pin = -1;
    memset(s_timer, 0, sizeof(s_timer));
    s_pwm_ready = true;
}

/* The finest duty resolution the 80 MHz clock allows at freq (up to 14 bits). */
static uint8_t pwm_bits(uint32_t freq)
{
    uint8_t bits = 1;
    while (bits < 14 && (80000000u >> (bits + 1)) >= freq)
        bits++;
    return bits;
}

/* A timer running at freq (shared), or a free one set up for it. */
static const char *pwm_timer(uint32_t freq, int *timer)
{
    for (int t = 0; t < PWM_TIMERS; t++)
    {
        if (s_timer[t].users && s_timer[t].freq == freq)
        {
            *timer = t;
            return NULL;
        }
    }
    for (int t = 0; t < PWM_TIMERS; t++)
    {
        if (s_timer[t].users)
            continue;
        ledc_timer_config_t c = {
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .duty_resolution = pwm_bits(freq),
            .timer_num = t,
            .freq_hz = freq,
            .clk_cfg = LEDC_AUTO_CLK,
        };
        esp_err_t err = ledc_timer_config(&c);
        if (err != ESP_OK)
            return esp_fail("PWM frequency", err);
        s_timer[t].freq = freq;
        s_timer[t].bits = c.duty_resolution;
        *timer = t;
        return NULL;
    }
    return "machine: PWM: all 4 timers are in use (outputs with the same frequency share one)";
}

static void pwm_apply_duty(int h)
{
    uint32_t bits = s_timer[s_pwm[h].timer].bits;
    uint32_t d = s_pwm[h].duty_u16 >= 65535 ? (1u << bits) : (uint32_t)(((uint64_t)s_pwm[h].duty_u16 << bits) / 65535u);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, h, d);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, h);
}

const char *td_hw_pwm_open(int pin, uint32_t freq, int *handle)
{
    pwm_ready();
    for (int h = 0; h < PWM_CHANNELS; h++)
    {
        if (s_pwm[h].pin == pin)
        {
            *handle = h;
            return td_hw_pwm_set_freq(h, freq);
        }
    }
    int h = 0;
    while (h < PWM_CHANNELS && s_pwm[h].pin >= 0)
        h++;
    if (h == PWM_CHANNELS)
        return fail("machine: PWM: all %d outputs are in use", PWM_CHANNELS, NULL);
    int timer;
    const char *err = pwm_timer(freq, &timer);
    if (err)
        return err;
    s_touched |= PIN_BIT(pin);
    ledc_channel_config_t c = {
        .gpio_num = pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = h,
        .timer_sel = timer,
        .duty = 0,
    };
    esp_err_t e = ledc_channel_config(&c);
    if (e != ESP_OK)
        return esp_fail("PWM", e);
    s_timer[timer].users++;
    s_pwm[h].pin = (int8_t)pin;
    s_pwm[h].timer = (int8_t)timer;
    s_pwm[h].duty_u16 = 0;
    *handle = h;
    return NULL;
}

const char *td_hw_pwm_set_freq(int h, uint32_t freq)
{
    if (h < 0 || h >= PWM_CHANNELS || s_pwm[h].pin < 0)
        return "machine: PWM is closed";
    int old = s_pwm[h].timer;
    if (s_timer[old].freq == freq)
        return NULL;
    s_timer[old].users--;   /* free it while looking, so a lone user can reuse it */
    int timer;
    const char *err = pwm_timer(freq, &timer);
    if (err)
    {
        s_timer[old].users++;
        return err;
    }
    s_timer[timer].users++;
    s_pwm[h].timer = (int8_t)timer;
    ledc_bind_channel_timer(LEDC_LOW_SPEED_MODE, h, timer);
    pwm_apply_duty(h);
    return NULL;
}

uint32_t td_hw_pwm_freq(int h)
{
    return h >= 0 && h < PWM_CHANNELS && s_pwm[h].pin >= 0 ? s_timer[s_pwm[h].timer].freq : 0;
}

void td_hw_pwm_set_duty(int h, uint32_t duty_u16)
{
    if (h < 0 || h >= PWM_CHANNELS || s_pwm[h].pin < 0)
        return;
    s_pwm[h].duty_u16 = duty_u16 > 65535 ? 65535 : duty_u16;
    pwm_apply_duty(h);
}

uint32_t td_hw_pwm_duty(int h)
{
    return h >= 0 && h < PWM_CHANNELS && s_pwm[h].pin >= 0 ? s_pwm[h].duty_u16 : 0;
}

void td_hw_pwm_close(int h)
{
    if (h < 0 || h >= PWM_CHANNELS || s_pwm[h].pin < 0)
        return;
    ledc_stop(LEDC_LOW_SPEED_MODE, h, 0);
    s_timer[s_pwm[h].timer].users--;
    gpio_reset_pin(s_pwm[h].pin);
    s_pwm[h].pin = -1;
}

/* ------------------------------------------------------------- I2C */

/* ESP-IDF's I2C master driver: a bus per port, and a device for the
 * address of the last transfer (made again when the address changes). */
static struct
{
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t dev;
    uint16_t addr;
    uint32_t freq;
    int timeout_ms;
} s_i2c[SOC_HP_I2C_NUM];

int td_hw_i2c_count(void)
{
    return SOC_HP_I2C_NUM;
}

void td_hw_i2c_close(int id)
{
    if (id < 0 || id >= SOC_HP_I2C_NUM)
        return;
    if (s_i2c[id].dev)
        i2c_master_bus_rm_device(s_i2c[id].dev);
    if (s_i2c[id].bus)
        i2c_del_master_bus(s_i2c[id].bus);
    s_i2c[id].dev = NULL;
    s_i2c[id].bus = NULL;
}

const char *td_hw_i2c_open(int id, int scl, int sda, uint32_t freq, uint32_t timeout_us)
{
    td_hw_i2c_close(id);
    s_touched |= PIN_BIT(scl) | PIN_BIT(sda);
    i2c_master_bus_config_t c = {
        .i2c_port = id,
        .scl_io_num = scl,
        .sda_io_num = sda,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&c, &s_i2c[id].bus);
    if (err != ESP_OK)
    {
        s_i2c[id].bus = NULL;
        return esp_fail("I2C", err);
    }
    s_i2c[id].freq = freq;
    s_i2c[id].timeout_ms = (int)(timeout_us / 1000) + 10;
    return NULL;
}

static esp_err_t i2c_device(int id, uint16_t addr)
{
    if (s_i2c[id].dev && s_i2c[id].addr == addr)
        return ESP_OK;
    if (s_i2c[id].dev)
        i2c_master_bus_rm_device(s_i2c[id].dev);
    s_i2c[id].dev = NULL;
    i2c_device_config_t d = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = s_i2c[id].freq,
    };
    esp_err_t err = i2c_master_bus_add_device(s_i2c[id].bus, &d, &s_i2c[id].dev);
    if (err == ESP_OK)
        s_i2c[id].addr = addr;
    else
        s_i2c[id].dev = NULL;
    return err;
}

/* All the buffers as one (b is bufs[0] when there is only one). */
static uint8_t *i2c_join(size_t n, td_hw_buf_t *bufs, size_t *len, bool copy_in)
{
    *len = 0;
    for (size_t i = 0; i < n; i++)
        *len += bufs[i].len;
    if (n == 1)
        return bufs[0].buf;
    uint8_t *b = malloc(*len ? *len : 1);
    if (b && copy_in)
        for (size_t i = 0, at = 0; i < n; at += bufs[i].len, i++)
            memcpy(b + at, bufs[i].buf, bufs[i].len);
    return b;
}

static void i2c_split(size_t n, td_hw_buf_t *bufs, uint8_t *b)
{
    if (n == 1)
        return;
    for (size_t i = 0, at = 0; i < n; at += bufs[i].len, i++)
        memcpy(bufs[i].buf, b + at, bufs[i].len);
    free(b);
}

/* Every transfer ends with a stop (the driver's way); a write that is
 * followed by a read of the same device is one transfer (WRITE1). */
int td_hw_i2c_transfer(int id, uint16_t addr, size_t n, td_hw_buf_t *bufs, unsigned flags)
{
    if (id < 0 || id >= SOC_HP_I2C_NUM || !s_i2c[id].bus)
        return -5;   /* MP_EIO */
    int ms = s_i2c[id].timeout_ms;
    esp_err_t err;
    size_t len = 0;
    if (!(flags & (TD_HW_I2C_READ | TD_HW_I2C_WRITE1)) && n == 1 && bufs[0].len == 0)
    {   /* scan(): is anything there? */
        err = i2c_master_probe(s_i2c[id].bus, addr, ms);
        return err == ESP_OK ? 0 : err == ESP_ERR_TIMEOUT ? -110
                                                          : -19;   /* MP_ETIMEDOUT, MP_ENODEV */
    }
    err = i2c_device(id, addr);
    if (err != ESP_OK)
        return -5;
    if (flags & TD_HW_I2C_WRITE1)
    {
        size_t rlen;
        uint8_t *r = i2c_join(n - 1, bufs + 1, &rlen, false);
        if (!r)
            return -12;   /* MP_ENOMEM */
        err = i2c_master_transmit_receive(s_i2c[id].dev, bufs[0].buf, bufs[0].len, r, rlen, ms);
        if (err == ESP_OK)
            i2c_split(n - 1, bufs + 1, r);
        else if (n - 1 > 1)
            free(r);
        len = bufs[0].len + rlen;
    }
    else if (flags & TD_HW_I2C_READ)
    {
        uint8_t *r = i2c_join(n, bufs, &len, false);
        if (!r)
            return -12;
        err = i2c_master_receive(s_i2c[id].dev, r, len, ms);
        if (err == ESP_OK)
            i2c_split(n, bufs, r);
        else if (n > 1)
            free(r);
    }
    else
    {
        uint8_t *w = i2c_join(n, bufs, &len, true);
        if (!w)
            return -12;
        err = i2c_master_transmit(s_i2c[id].dev, w, len, ms);
        if (n > 1)
            free(w);
    }
    if (err == ESP_OK)
        return (int)len;
    return err == ESP_ERR_TIMEOUT ? -110 : -19;
}

/* ------------------------------------------------------------- SPI */

#define SPI_IDS (SOC_SPI_PERIPH_NUM - 1)   /* SPI1 is the flash */

static spi_device_handle_t s_spi[SPI_IDS];

int td_hw_spi_count(void)
{
    return SPI_IDS;
}

static spi_host_device_t spi_host(int id)
{
    return id == 1 ? SPI2_HOST : (spi_host_device_t)(SPI2_HOST + id - 1);
}

void td_hw_spi_close(int id)
{
    if (id < 1 || id > SPI_IDS || !s_spi[id - 1])
        return;
    spi_bus_remove_device(s_spi[id - 1]);
    spi_bus_free(spi_host(id));
    s_spi[id - 1] = NULL;
}

const char *td_hw_spi_open(int id, int sck, int mosi, int miso, uint32_t baud, int polarity, int phase, bool lsb_first)
{
    td_hw_spi_close(id);
    s_touched |= (sck >= 0 ? PIN_BIT(sck) : 0) | (mosi >= 0 ? PIN_BIT(mosi) : 0) |
                 (miso >= 0 ? PIN_BIT(miso) : 0);
    spi_bus_config_t bus = {
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .sclk_io_num = sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092,
    };
    if (spi_bus_get_attr(spi_host(id)))   /* initialised by TinyDesk */
        return fail("machine: SPI(%d) is used by TinyDesk (Ethernet or the SD card): use SoftSPI", id, NULL);
    esp_err_t err = spi_bus_initialize(spi_host(id), &bus, SPI_DMA_CH_AUTO);
    if (err == ESP_ERR_INVALID_STATE)
        return fail("machine: SPI(%d) is used by TinyDesk (Ethernet or the SD card): use SoftSPI", id, NULL);
    if (err != ESP_OK)
        return esp_fail("SPI", err);
    spi_device_interface_config_t dev = {
        .clock_speed_hz = (int)baud,
        .mode = (uint8_t)(polarity << 1 | phase),
        .spics_io_num = -1,
        .queue_size = 1,
        .flags = lsb_first ? SPI_DEVICE_BIT_LSBFIRST : 0,
    };
    err = spi_bus_add_device(spi_host(id), &dev, &s_spi[id - 1]);
    if (err != ESP_OK)
    {
        spi_bus_free(spi_host(id));
        s_spi[id - 1] = NULL;
        return esp_fail("SPI", err);
    }
    return NULL;
}

const char *td_hw_spi_transfer(int id, size_t len, const uint8_t *src, uint8_t *dest)
{
    if (id < 1 || id > SPI_IDS || !s_spi[id - 1])
        return "machine: SPI is closed";
    while (len)
    {
        size_t n = len > 4092 ? 4092 : len;
        spi_transaction_t t = {.length = n * 8, .tx_buffer = src, .rx_buffer = dest};
        esp_err_t err = spi_device_polling_transmit(s_spi[id - 1], &t);
        if (err != ESP_OK)
            return esp_fail("SPI", err);
        len -= n;
        if (src)
            src += n;
        if (dest)
            dest += n;
    }
    return NULL;
}

/* ------------------------------------------------------------ UART */

static uint8_t s_uart_open;   /* bit per port */

int td_hw_uart_count(void)
{
    return SOC_UART_HP_NUM;
}

void td_hw_uart_close(int id)
{
    if (id >= 0 && id < SOC_UART_HP_NUM && (s_uart_open & (1u << id)))
        uart_driver_delete(id);
    s_uart_open &= ~(1u << id);
}

const char *td_hw_uart_open(int id, int tx, int rx, uint32_t baud, int bits, int parity, int stop, int rxbuf)
{
    td_hw_uart_close(id);
    if (td_hw_uart_kept(id, CHIP) || uart_is_driver_installed(id))
        return fail("machine: UART(%d) is used by TinyDesk (console or RS-485)", id, NULL);
    s_touched |= (tx >= 0 ? PIN_BIT(tx) : 0) | (rx >= 0 ? PIN_BIT(rx) : 0);
    uart_config_t c = {
        .baud_rate = (int)baud,
        .data_bits = (uart_word_length_t)(UART_DATA_5_BITS + (bits - 5)),
        .parity = parity == 1 ? UART_PARITY_ODD : parity == 2 ? UART_PARITY_EVEN
                                                              : UART_PARITY_DISABLE,
        .stop_bits = stop == 2 ? UART_STOP_BITS_2 : UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_driver_install(id, rxbuf, 0, 0, NULL, 0);
    if (err == ESP_OK)
    {
        s_uart_open |= 1u << id;
        err = uart_param_config(id, &c);
    }
    if (err == ESP_OK)
        err = uart_set_pin(id, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK)
    {
        td_hw_uart_close(id);
        return esp_fail("UART", err);
    }
    return NULL;
}

int td_hw_uart_read(int id, uint8_t *buf, size_t len, int timeout_ms)
{
    if (id < 0 || id >= SOC_UART_HP_NUM || !(s_uart_open & (1u << id)))
        return -1;
    return uart_read_bytes(id, buf, len, pdMS_TO_TICKS(timeout_ms));
}

int td_hw_uart_write(int id, const uint8_t *buf, size_t len)
{
    if (id < 0 || id >= SOC_UART_HP_NUM || !(s_uart_open & (1u << id)))
        return -1;
    return uart_write_bytes(id, buf, len);
}

int td_hw_uart_any(int id)
{
    size_t n = 0;
    if (id >= 0 && id < SOC_UART_HP_NUM && (s_uart_open & (1u << id)))
        uart_get_buffered_data_len(id, &n);
    return (int)n;
}

bool td_hw_uart_txdone(int id)
{
    return !(id >= 0 && id < SOC_UART_HP_NUM && (s_uart_open & (1u << id))) || uart_wait_tx_done(id, 0) == ESP_OK;
}

/* ------------------------------------------------------- bitstream */

/* Through the RMT: a channel and a bytes encoder for this call. */
const char *td_hw_bitstream(int pin, const uint32_t *timing_ns, const uint8_t *buf, size_t len)
{
    rmt_channel_handle_t ch = NULL;
    rmt_encoder_handle_t enc = NULL;
    rmt_tx_channel_config_t cc = {
        .gpio_num = pin,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 40000000,   /* 25 ns ticks */
        .mem_block_symbols = SOC_RMT_MEM_WORDS_PER_CHANNEL,
        .trans_queue_depth = 1,
    };
#define TICKS(ns) ((uint16_t)(((ns) + 12) / 25 > 32767 ? 32767 : ((ns) + 12) / 25))
    rmt_bytes_encoder_config_t ec = {
        .bit0 = {.level0 = 1, .duration0 = TICKS(timing_ns[0]), .level1 = 0, .duration1 = TICKS(timing_ns[1])},
        .bit1 = {.level0 = 1, .duration0 = TICKS(timing_ns[2]), .level1 = 0, .duration1 = TICKS(timing_ns[3])},
        .flags.msb_first = 1,
    };
#undef TICKS
    rmt_transmit_config_t tc = {.loop_count = 0};
    const char *err = NULL;
    esp_err_t e = rmt_new_tx_channel(&cc, &ch);
    if (e == ESP_OK)
        e = rmt_new_bytes_encoder(&ec, &enc);
    if (e == ESP_OK)
        e = rmt_enable(ch);
    if (e == ESP_OK)
    {
        e = rmt_transmit(ch, enc, buf, len, &tc);
        if (e == ESP_OK)
            e = rmt_tx_wait_all_done(ch, (int)(100 + len / 10));
        rmt_disable(ch);
    }
    if (e != ESP_OK)
        err = esp_fail("bitstream", e);
    if (enc)
        rmt_del_encoder(enc);
    if (ch)
        rmt_del_channel(ch);
    /* The pin is a GPIO output again, low. */
    gpio_set_level(pin, 0);
    td_hw_pin_mode(pin, TD_HW_OUT, TD_HW_PULL_NONE);
    return err;
}

/* ------------------------------------------------------------ misc */

uint32_t td_hw_cpu_freq(void)
{
    return (uint32_t)esp_clk_cpu_freq();
}

bool td_hw_unique_id(uint8_t id[6])
{
    return esp_efuse_mac_get_default(id) == ESP_OK;
}

static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;

uint32_t td_hw_atomic_begin(void)
{
    portENTER_CRITICAL_SAFE(&s_mux);
    return 0;
}

void td_hw_atomic_end(uint32_t state)
{
    (void)state;
    portEXIT_CRITICAL_SAFE(&s_mux);
}

void td_hw_release_all(void)
{
    for (int pin = 0; pin < GPIO_NUM_MAX; pin++)
    {
        if (s_irq_pins & PIN_BIT(pin))
        {
            gpio_intr_disable(pin);
            gpio_isr_handler_remove(pin);
        }
        s_irq_type[pin] = GPIO_INTR_DISABLE;
    }
    s_irq_pins = 0;
    s_isr = NULL;
    if (s_isr_service)
        gpio_uninstall_isr_service();
    s_isr_service = false;

    if (s_pwm_ready)
        for (int h = 0; h < PWM_CHANNELS; h++)
            td_hw_pwm_close(h);
    for (int t = 0; t < PWM_TIMERS; t++)
    {
        if (s_timer[t].freq)
            ledc_timer_rst(LEDC_LOW_SPEED_MODE, t);
        s_timer[t].freq = 0;
        s_timer[t].users = 0;
    }
    adc_release();
    for (int id = 0; id < SOC_HP_I2C_NUM; id++)
        td_hw_i2c_close(id);
    for (int id = 1; id <= SPI_IDS; id++)
        td_hw_spi_close(id);
    for (int id = 0; id < SOC_UART_HP_NUM; id++)
        td_hw_uart_close(id);

    for (int pin = 0; pin < GPIO_NUM_MAX; pin++)
        if (s_touched & PIN_BIT(pin))
            gpio_reset_pin(pin);
    s_used = 0;
    s_touched = 0;
    s_warned = 0;
}
