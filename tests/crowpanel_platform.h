/* Host stand-ins for the RGB peripheral and RTOS, used by the actual LCD
 * painter regression. Frame completion is advanced explicitly by the test. */
#ifndef CROWPANEL_TEST_PLATFORM_H
#define CROWPANEL_TEST_PLATFORM_H
#undef NDEBUG /* the checks are asserts: keep them in Release builds */
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define IRAM_ATTR
#define pdTRUE              1
#define pdFALSE             0
#define pdPASS              1
#define portMAX_DELAY       UINT32_MAX
#define pdMS_TO_TICKS(x)    (x)
#define MALLOC_CAP_SPIRAM   1
#define LCD_CLK_SRC_DEFAULT 0
#define GPIO_MODE_OUTPUT    1
#define BIT64(n)            (UINT64_C(1) << (n))
#define ESP_OK              0
static inline void test_log(const char *tag, const char *format, ...)
{
    (void)tag;
    (void)format;
}
#define ESP_LOGE(...) test_log(__VA_ARGS__)
#define ESP_LOGI(...) test_log(__VA_ARGS__)
typedef int BaseType_t;
typedef uint32_t TickType_t;
typedef int esp_err_t;
static inline const char *esp_err_to_name(esp_err_t error)
{
    (void)error;
    return "mock error";
}
typedef int portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(p) \
    do                        \
    {                         \
        assert(!*(p));        \
        *(p) = 1;             \
    } while (0)
#define portEXIT_CRITICAL(p) \
    do                       \
    {                        \
        assert(*(p));        \
        *(p) = 0;            \
    } while (0)
#define portENTER_CRITICAL_ISR(p) portENTER_CRITICAL(p)
#define portEXIT_CRITICAL_ISR(p)  portEXIT_CRITICAL(p)
typedef struct
{
    int count;
} *SemaphoreHandle_t;
static inline SemaphoreHandle_t xSemaphoreCreateBinary(void)
{
    return calloc(1, sizeof(struct { int count; }));
}
static inline SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    SemaphoreHandle_t s = xSemaphoreCreateBinary();
    s->count = 1;
    return s;
}
static inline int xSemaphoreTake(SemaphoreHandle_t s, uint32_t timeout)
{
    if (!s->count)
    {
        assert(timeout != portMAX_DELAY);
        return pdFALSE;
    }
    s->count = 0;
    return pdTRUE;
}
static inline void xSemaphoreGive(SemaphoreHandle_t s)
{
    assert(!s->count);
    s->count = 1;
}
static inline void xSemaphoreGiveFromISR(SemaphoreHandle_t s, BaseType_t *wake)
{
    xSemaphoreGive(s);
    *wake = pdTRUE;
}
static TickType_t test_ticks;
static inline int64_t esp_timer_get_time(void)
{
    return (int64_t)test_ticks * 1000;
}
static inline unsigned xPortGetCoreID(void)
{
    return 0;
}
static inline TickType_t xTaskGetTickCount(void)
{
    return test_ticks;
}
static inline void vTaskDelay(TickType_t ticks)
{
    test_ticks += ticks;
}
static inline int xTaskCreate(void (*fn)(void *), const char *name, int stack, void *user, int priority, void *handle)
{
    (void)fn;
    (void)name;
    (void)stack;
    (void)user;
    (void)priority;
    (void)handle;
    return pdPASS;
}
static int test_painter_core = -1;
static inline int xTaskCreatePinnedToCore(void (*fn)(void *), const char *name, int stack, void *user, int priority, void *handle, int core)
{
    test_painter_core = core;
    return xTaskCreate(fn, name, stack, user, priority, handle);
}
static inline void *heap_caps_malloc(size_t n, int caps)
{
    (void)caps;
    return malloc(n);
}
static inline void *heap_caps_calloc(size_t n, size_t size, int caps)
{
    (void)caps;
    return calloc(n, size);
}
static inline void heap_caps_free(void *p)
{
    free(p);
}
typedef struct
{
    uint64_t pin_bit_mask;
    int mode;
} gpio_config_t;
static inline void gpio_config(const gpio_config_t *c)
{
    (void)c;
}
static inline void gpio_set_level(int pin, int level)
{
    (void)pin;
    (void)level;
}
static inline void esp_gpio_reserve(uint64_t pins)
{
    (void)pins;
}
static int test_board_font;
static inline int tdsh_board_int(const char *key, int fallback)
{
    return test_board_font && !strcmp(key, "lcd.font") ? test_board_font : fallback;
}
typedef void *esp_lcd_panel_handle_t;
typedef struct
{
    int unused;
} esp_lcd_rgb_panel_event_data_t;
typedef bool (*test_frame_callback_t)(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t *, void *);
typedef struct
{
    test_frame_callback_t on_bounce_frame_finish, on_vsync;
} esp_lcd_rgb_panel_event_callbacks_t;
typedef struct
{
    int clk_src;
    struct
    {
        uint32_t pclk_hz;
        int h_res, v_res, hsync_pulse_width, hsync_back_porch, hsync_front_porch, vsync_pulse_width, vsync_back_porch, vsync_front_porch;
        struct
        {
            int pclk_active_neg;
        } flags;
    } timings;
    int data_width, bits_per_pixel, num_fbs, bounce_buffer_size_px, psram_trans_align;
    int hsync_gpio_num, vsync_gpio_num, de_gpio_num, pclk_gpio_num, disp_gpio_num;
    struct
    {
        int fb_in_psram;
    } flags;
    int data_gpio_nums[16];
} esp_lcd_rgb_panel_config_t;
static uint16_t *test_buffers[2], *test_visible, *test_requested;
static unsigned test_present_count;
static test_frame_callback_t test_frame_callback;
static bool test_early_boundary;
static inline esp_err_t esp_lcd_new_rgb_panel(const esp_lcd_rgb_panel_config_t *c, esp_lcd_panel_handle_t *p)
{
    assert(c->num_fbs == 2);
    assert(c->bounce_buffer_size_px > 0);
    test_buffers[0] = calloc((size_t)c->timings.h_res * c->timings.v_res, 2);
    test_buffers[1] = calloc((size_t)c->timings.h_res * c->timings.v_res, 2);
    *p = (void *)1;
    return ESP_OK;
}
static inline esp_err_t esp_lcd_panel_reset(esp_lcd_panel_handle_t p)
{
    (void)p;
    return ESP_OK;
}
static inline esp_err_t esp_lcd_panel_init(esp_lcd_panel_handle_t p)
{
    (void)p;
    test_visible = test_buffers[0];
    return ESP_OK;
}
static inline esp_err_t esp_lcd_rgb_panel_get_frame_buffer(esp_lcd_panel_handle_t p, unsigned n, void **first, ...)
{
    (void)p;
    assert(n == 2);
    *first = test_buffers[0];
    va_list ap;
    va_start(ap, first);
    *va_arg(ap, void **) = test_buffers[1];
    va_end(ap);
    return ESP_OK;
}
static inline esp_err_t esp_lcd_rgb_panel_register_event_callbacks(esp_lcd_panel_handle_t p, const esp_lcd_rgb_panel_event_callbacks_t *c, void *user)
{
    (void)p;
    (void)user;
    test_frame_callback = c->on_bounce_frame_finish;
    return ESP_OK;
}
static inline esp_err_t esp_lcd_panel_draw_bitmap(esp_lcd_panel_handle_t p, int x, int y, int w, int h, const void *pixels)
{
    (void)p;
    (void)x;
    (void)y;
    (void)w;
    (void)h;
    assert(pixels != test_visible);
    test_requested = (uint16_t *)pixels;
    test_present_count++;
    if (test_early_boundary)
    {
        test_visible = test_requested;
        test_frame_callback(p, NULL, NULL);
        test_early_boundary = false;
    }
    return ESP_OK;
}
#endif
