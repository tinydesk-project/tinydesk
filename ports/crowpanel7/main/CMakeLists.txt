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
