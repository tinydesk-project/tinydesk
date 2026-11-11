# The classic ESP32 with PSRAM: the shared application (ports/esp_idf/app)
# with the desktop on UART0 (link_uart.c), and the static buffers of
# TinyDesk and TinyDesk Shell in PSRAM (extram.lf). Bluetooth is Bluedroid in
# dual mode here (Classic and LE, bt_hid_bluedroid.c), NimBLE elsewhere.
include("${CMAKE_CURRENT_LIST_DIR}/../../esp_idf/app/app.cmake")

idf_component_register(
    SRCS "${TD_ESP_APP_DIR}/link_uart.c" ${TD_ESP_APP_SRCS}
    EMBED_TXTFILES "${TD_BOARD_BUILTIN_CONF}"
    INCLUDE_DIRS "." "${TD_ESP_APP_DIR}"
    LDFRAGMENTS "extram.lf"
    REQUIRES ${TD_ESP_APP_REQUIRES} esp_hid   # Bluetooth Classic and LE (bt_hid_bluedroid.c)
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
