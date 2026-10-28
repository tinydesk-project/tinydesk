# The TinyDesk application shared by the ESP-IDF board projects
# (ports/esp32c6, ports/esp32, ports/esp32-4mb). A board's main/CMakeLists.txt
# includes this file and registers its main component with these sources and
# its desktop link (link_usj.c on the ESP32-C6, link_uart.c here on the
# classic ESP32):
#
#   include("${CMAKE_CURRENT_LIST_DIR}/../../esp_idf/app/app.cmake")
#   idf_component_register(
#       SRCS "link_usj.c" ${TD_ESP_APP_SRCS}
#       EMBED_TXTFILES "${TD_BOARD_BUILTIN_CONF}"
#       INCLUDE_DIRS "." "${TD_ESP_APP_DIR}"
#       REQUIRES ${TD_ESP_APP_REQUIRES} esp_driver_usb_serial_jtag)

set(TD_ESP_APP_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(TD_ESP_APP_SRCS
    "${TD_ESP_APP_DIR}/main.c"
    "${TD_ESP_APP_DIR}/hal_mux.c"
    "${TD_ESP_APP_DIR}/tdsh_bridge_esp.c"
    "${TD_ESP_APP_DIR}/net_esp.c"
    "${TD_ESP_APP_DIR}/telnet.c"
    "${TD_ESP_APP_DIR}/ota_esp.c"
    "${TD_ESP_APP_DIR}/rs485.c"
    "${TD_ESP_APP_DIR}/../../common/td_proto_cmds.c"
)
set(TD_ESP_APP_REQUIRES
    tinydesk
    tdsh
    micropython
    nvs_flash
    esp_driver_uart
    app_update
    esp_https_ota
    json
    esp_http_client
    bootloader_support
    mbedtls
    esp_timer
    esp_netif
    esp_event
    lwip
)

# The board configuration built into the firmware (pins and other hardware
# settings; see the Board configuration page of the documentation):
# board.conf next to the board project if it exists (your own wiring; it is
# not in git), else its board.example.conf. /fs/etc/board.conf on the device
# overrides it at run time.
set(TD_BOARD_CONF "${COMPONENT_DIR}/../board.conf")
option(TD_USE_EXAMPLE_BOARD "Build generic firmware without private board.conf" OFF)
if(TD_USE_EXAMPLE_BOARD OR NOT EXISTS "${TD_BOARD_CONF}")
    set(TD_BOARD_CONF "${COMPONENT_DIR}/../board.example.conf")
endif()
set(TD_BOARD_BUILTIN_CONF "${CMAKE_CURRENT_BINARY_DIR}/board_builtin.conf")
configure_file("${TD_BOARD_CONF}" "${TD_BOARD_BUILTIN_CONF}" COPYONLY)
message(STATUS "tinydesk board configuration: ${TD_BOARD_CONF}")
