# The ESP32-S3 with PSRAM: the shared application (ports/esp_idf/app) with
# the desktop on the built-in USB Serial/JTAG port (link_usj.c, as on the
# ESP32-C6), and the static buffers of TinyDesk and TinyDesk Shell in PSRAM
# (extram.lf).
include("${CMAKE_CURRENT_LIST_DIR}/../../esp_idf/app/app.cmake")

idf_component_register(
    SRCS "${TD_ESP_APP_DIR}/link_usj.c" ${TD_ESP_APP_SRCS}
    EMBED_TXTFILES "${TD_BOARD_BUILTIN_CONF}"
    INCLUDE_DIRS "." "${TD_ESP_APP_DIR}"
    LDFRAGMENTS "extram.lf"
    REQUIRES ${TD_ESP_APP_REQUIRES} esp_driver_usb_serial_jtag
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
