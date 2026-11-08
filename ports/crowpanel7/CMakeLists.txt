# tinydesk firmware for the Elecrow CrowPanel 7.0 (DIS08070H): an
# ESP32-S3-WROOM-1-N4R8 (4 MB flash, 8 MB octal PSRAM) with an 800x480 RGB
# LCD and a GT911 touch screen, ESP-IDF 5.3.1.
#
#   idf.py build
#   idf.py -p COMx -b 921600 flash      (close PuTTY first)
#
# The desktop is drawn on the LCD (main/display.c, 100x30 cells) and the
# touch screen is its mouse; the keyboard is a terminal on UART0 (the
# board's USB-C port) at 921600 baud, which also shows the picture (set the
# terminal to 100x30). The application is shared with the other ESP-IDF
# projects (../esp_idf/app); this project brings the LCD/touch/UART link,
# the PSRAM placement (main/extram.lf), the partitions and the sdkconfig.
cmake_minimum_required(VERSION 3.16)

get_filename_component(TD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(TDSH_SDK_ROOT "${TD_ROOT}/third_party/tdsh")

set(IDF_TARGET "esp32s3" CACHE STRING "tinydesk target")
list(APPEND EXTRA_COMPONENT_DIRS
    "${TDSH_SDK_ROOT}/ports/esp_idf/components"
    "${TD_ROOT}/ports/esp_idf/components")

if(DEFINED ENV{TD_VERSION})
    set(PROJECT_VER "$ENV{TD_VERSION}")
else()
    set(PROJECT_VER "0.3.0")
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tinydesk)

# Fresh build date and time in every firmware image.
include("${TD_ROOT}/ports/common/app_desc_stamp.cmake")

include("${TDSH_SDK_ROOT}/ports/esp_idf/tdsh_espidf_project_setup.cmake")
