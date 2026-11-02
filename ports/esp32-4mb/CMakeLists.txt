# tinydesk firmware for the classic ESP32 with 4 MB flash and no PSRAM
# (ESP32-WROOM-32 / ESP32-DevKitC style boards), ESP-IDF 5.3.1.
#
#   idf.py build
#   idf.py -p COMx flash        (close PuTTY first)
#
# The desktop is served on UART0 (the board's USB-UART chip) at 921600 baud;
# the ESP-IDF console is off (logs go to the Log Viewer). The application
# is shared with the other ESP-IDF projects (../esp_idf/app), with the same
# UART link as ports/esp32. Differences to ports/esp32: no PSRAM (screen at
# most 80x25, all buffers in internal RAM), one 2.5 MB app slot (no
# over-the-air updates), 1.4 MB /fs.
cmake_minimum_required(VERSION 3.16)

get_filename_component(TD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(TDSH_SDK_ROOT "${TD_ROOT}/third_party/tdsh")

set(IDF_TARGET "esp32" CACHE STRING "tinydesk target")
list(APPEND EXTRA_COMPONENT_DIRS
    "${TDSH_SDK_ROOT}/ports/esp_idf/components"
    "${TD_ROOT}/ports/esp_idf/components")

if(DEFINED ENV{TD_VERSION})
    set(PROJECT_VER "$ENV{TD_VERSION}")
else()
    set(PROJECT_VER "0.2.0")
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tinydesk)

# Fresh build date and time in every firmware image.
include("${TD_ROOT}/ports/common/app_desc_stamp.cmake")

include("${TDSH_SDK_ROOT}/ports/esp_idf/tdsh_espidf_project_setup.cmake")

