# tinydesk firmware for the ESP32-S3 (16 MB flash, 8 MB Octal PSRAM),
# ESP-IDF 5.3.1.
#
#   idf.py set-target esp32s3
#   idf.py build
#   idf.py -p COMx -b 921600 flash        (close PuTTY first)
#
# The desktop is served on UART0 (the board's USB-UART chip) at 921600 baud;
# the ESP-IDF console is off (logs go to the Log Viewer). The application
# is shared with the other ESP-IDF projects (../esp_idf/app); this project
# brings the UART link, the PSRAM placement (main/extram.lf), the
# partitions and the sdkconfig.
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
    set(PROJECT_VER "0.1.6")
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tinydesk)

# Fresh build date and time in every firmware image.
include("${TD_ROOT}/ports/common/app_desc_stamp.cmake")

include("${TDSH_SDK_ROOT}/ports/esp_idf/tdsh_espidf_project_setup.cmake")
