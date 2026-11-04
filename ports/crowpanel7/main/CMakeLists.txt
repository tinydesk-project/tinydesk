# The CrowPanel 7.0: the shared application (ports/esp_idf/app) with the
# desktop on the LCD and touch screen and the keyboard on UART0
# (link_crowpanel.c), and the static buffers in PSRAM (extram.lf).
include("${CMAKE_CURRENT_LIST_DIR}/../../esp_idf/app/app.cmake")

idf_component_register(
    SRCS "link_crowpanel.c" "display.c" "touch.c" "font8x16.c" "font10x20.c" "font12x24.c" "font16x32.c" ${TD_ESP_APP_SRCS}
    EMBED_TXTFILES "${TD_BOARD_BUILTIN_CONF}"
    INCLUDE_DIRS "." "${TD_ESP_APP_DIR}"
    LDFRAGMENTS "extram.lf"
    REQUIRES ${TD_ESP_APP_REQUIRES} esp_lcd esp_driver_i2c esp_driver_gpio
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
# Keep bulk pixel writes on the same CPU that allocates/services RGB DMA.
# Otherwise both CPUs can stream through the shared PSRAM cache while the
# bounce interrupt is trying to refill its next internal-RAM buffer.
set(TD_CROWPANEL_RENDER_CORE "1" CACHE STRING "CrowPanel UI/LCD CPU; -1 leaves tasks unpinned")
if(NOT TD_CROWPANEL_RENDER_CORE MATCHES "^(-1|0|1)$")
    message(FATAL_ERROR "TD_CROWPANEL_RENDER_CORE must be -1, 0 or 1")
endif()
target_compile_definitions(${COMPONENT_LIB} PRIVATE TD_CROWPANEL_RENDER_CORE=${TD_CROWPANEL_RENDER_CORE})
if(NOT DEFINED TD_RENDER_OPTIMIZE OR TD_RENDER_OPTIMIZE)
    set_source_files_properties(display.c PROPERTIES COMPILE_OPTIONS "-O2")
endif()

# uxTaskGetSystemState scans every task stack with interrupts masked on the
# dual-core IDF kernel. That can starve RGB bounce DMA and visibly shift the
# LCD. Add a short, no-stack-scan snapshot at the kernel's additions hook;
# preserve the SDK's original implementation and all its other APIs.
if(CONFIG_FREERTOS_SMP)
    message(FATAL_ERROR "CrowPanel task sampler currently supports ESP-IDF's default FreeRTOS kernel, not experimental SMP")
endif()
idf_component_get_property(td_freertos_lib freertos COMPONENT_LIB)
target_include_directories(${td_freertos_lib} BEFORE PRIVATE "${CMAKE_CURRENT_LIST_DIR}/freertos")
target_compile_definitions(${COMPONENT_LIB} PRIVATE TD_FAST_TASK_SNAPSHOT=1)
