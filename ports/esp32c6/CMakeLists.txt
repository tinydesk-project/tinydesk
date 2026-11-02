# tinydesk firmware for the ESP32-C6 (ESP-IDF 5.3.1).
#
#   idf.py set-target esp32c6
#   idf.py build
#   idf.py -p COMx flash        (close PuTTY first)
#
# The desktop is served on the built-in USB Serial/JTAG port; ESP-IDF logs
# and the console go to UART0. TinyDesk Shell runs inside the Terminal app.
# The application is shared with the classic ESP32 projects
# (../esp_idf/app); this project brings the link (main/link_usj.c), the
# partitions and the sdkconfig.
cmake_minimum_required(VERSION 3.16)

get_filename_component(TD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
set(TDSH_SDK_ROOT "${TD_ROOT}/third_party/tdsh")

set(IDF_TARGET "esp32c6" CACHE STRING "tinydesk target")
list(APPEND EXTRA_COMPONENT_DIRS
    "${TDSH_SDK_ROOT}/ports/esp_idf/components"
    "${TD_ROOT}/ports/esp_idf/components")

# The firmware version shown by Software Update and `ota status`.
# Override for a test build: set TD_VERSION=0.1.6 before idf.py build.
if(DEFINED ENV{TD_VERSION})
    set(PROJECT_VER "$ENV{TD_VERSION}")
else()
    set(PROJECT_VER "0.2.0")
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tinydesk)

# Fresh build date and time in every firmware image.
include("${TD_ROOT}/ports/common/app_desc_stamp.cmake")

# Compatibility settings for TinyDesk Shell's pinned SMB / wolfSSL dependencies.
include("${TDSH_SDK_ROOT}/ports/esp_idf/tdsh_espidf_project_setup.cmake")
