# The Arduino Nano ESP32: the shared application (ports/esp_idf/app) with
# the desktop on the built-in USB Serial/JTAG port (link_usj.c, shared with
# the ESP32-C6), and the static buffers of TinyDesk and TinyDesk Shell in
# PSRAM (extram.lf). No USB keyboard: the USB-C port carries the desktop.
include("${CMAKE_CURRENT_LIST_DIR}/../../esp_idf/app/app.cmake")

idf_component_register(
    SRCS "${TD_ESP_APP_DIR}/link_usj.c" ${TD_ESP_APP_SRCS}
    EMBED_TXTFILES "${TD_BOARD_BUILTIN_CONF}"
    INCLUDE_DIRS "." "${TD_ESP_APP_DIR}"
    LDFRAGMENTS "extram.lf"
    REQUIRES ${TD_ESP_APP_REQUIRES} esp_driver_usb_serial_jtag
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)

# The chip is the same ESP32-S3 as ports/esp32s3, but that image serves the
# desktop on UART0 through a bridge chip this board does not have: taking it
# from Software Update would leave a working firmware with no way to show
# it. Name this port's own board (its feed: update-desktop-nanoesp32.json).
target_compile_definitions(${COMPONENT_LIB} PRIVATE TD_UPDATE_BOARD="nanoesp32")
