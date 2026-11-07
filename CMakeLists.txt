# tinydesk host build: the core, the demo apps, a desktop port (POSIX or
# Windows console), unit tests and examples.
#
#   cmake -B build && cmake --build build && ./build/tinydesk
#   ctest --test-dir build --output-on-failure
#
# The ESP32 firmware is built by separate ESP-IDF projects: ports/esp32c6,
# ports/esp32, ports/esp32-4mb and ports/esp32s3, with their shared code in
# ports/esp_idf.
cmake_minimum_required(VERSION 3.16)
project(tinydesk C)

# On a PC the file root is <working directory>/tinydesk_fs: allow long paths.
add_compile_definitions(TD_PATH_MAX=512)

option(TD_WITH_TDSH "Embed TinyDesk Shell in the Terminal app" ON)
option(TD_WITH_PYTHON "Add MicroPython to TinyDesk Shell (the py command)" ON)
option(TD_BUILD_TESTS "Build unit tests" ON)
option(TD_BUILD_EXAMPLES "Build examples" ON)
option(TD_RENDER_OPTIMIZE "Optimize native rendering hot paths" ON)
if(TD_RENDER_OPTIMIZE AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(apps/pyui.c apps/pyui_cache.c src/picture.c
        src/canvas.c src/screen.c src/render.c src/wm.c
        tools/sim_lcd.c tests/test_crowpanel_display.c PROPERTIES COMPILE_OPTIONS "-O2")
endif()

set(TDSH_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tdsh)

if(MSVC)
    set(TD_WARNINGS /W4 /WX /utf-8)   # sources contain UTF-8 icon glyphs
else()
    set(TD_WARNINGS -Wall -Wextra -Wpedantic -Werror)
endif()

# ---------------------------------------------------------------- core
add_library(tinydesk_core STATIC
    src/canvas.c
    src/picture.c
    src/font.c
    src/draw.c
    src/event.c
    src/input.c
    src/render.c
    src/screen.c
    src/td.c
    src/theme.c
    src/timer.c
    src/utf8.c
    src/vterm.c
    src/widgets.c
    src/wm.c
)
target_include_directories(tinydesk_core PUBLIC include)
target_compile_definitions(tinydesk_core PUBLIC TD_PICTURE_MAX=128)
set_target_properties(tinydesk_core PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_compile_options(tinydesk_core PRIVATE ${TD_WARNINGS})

# ------------------------------------------------ protocols (MQTT, Modbus)
add_library(tinydesk_proto STATIC
    proto/td_modbus.c
    proto/td_mqtt.c
    proto/td_mqtt_conf.c
    proto/td_sock.c
    proto/td_tls.c
)
target_include_directories(tinydesk_proto PUBLIC proto)
set_target_properties(tinydesk_proto PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_compile_options(tinydesk_proto PRIVATE ${TD_WARNINGS})
if(WIN32)
    target_link_libraries(tinydesk_proto PUBLIC ws2_32)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(tinydesk_proto PUBLIC Threads::Threads)
endif()

# MQTT over TLS needs mbedTLS 3.x. Point TD_MBEDTLS_DIR at its source tree,
# or leave it empty to use ESP-IDF's copy when one is installed. Without it
# the host build still works, only mqtts is refused.
set(TD_MBEDTLS_DIR "" CACHE PATH "mbedTLS 3.x source tree (for MQTT over TLS)")
# Release builds set this, so a program without TLS is never published.
option(TD_REQUIRE_TLS "Stop when no mbedTLS source tree is found" OFF)
if(NOT TD_MBEDTLS_DIR)
    foreach(cand "$ENV{IDF_PATH}/components/mbedtls/mbedtls"
                 "C:/Espressif/frameworks/esp-idf-v5.3.1/components/mbedtls/mbedtls"
                 "/mnt/c/Espressif/frameworks/esp-idf-v5.3.1/components/mbedtls/mbedtls"
                 "$ENV{HOME}/esp/esp-idf/components/mbedtls/mbedtls")
        if(EXISTS "${cand}/include/mbedtls/ssl.h")
            set(TD_MBEDTLS_DIR "${cand}")
            break()
        endif()
    endforeach()
endif()
if(TD_MBEDTLS_DIR)
    message(STATUS "tinydesk: MQTT over TLS with mbedTLS from ${TD_MBEDTLS_DIR}")
    set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(MBEDTLS_FATAL_WARNINGS OFF CACHE BOOL "" FORCE)
    set(GEN_FILES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${TD_MBEDTLS_DIR} ${CMAKE_BINARY_DIR}/mbedtls EXCLUDE_FROM_ALL)
    target_link_libraries(tinydesk_proto PUBLIC mbedtls mbedx509 mbedcrypto)
    target_compile_definitions(tinydesk_proto PUBLIC TD_HAVE_TLS)
    if(WIN32)
        target_link_libraries(tinydesk_proto PUBLIC crypt32 bcrypt)
    endif()
elseif(TD_REQUIRE_TLS)
    message(FATAL_ERROR "tinydesk: TD_REQUIRE_TLS is set but no mbedTLS source tree was found; "
                        "set TD_MBEDTLS_DIR (for example a checkout of https://github.com/Mbed-TLS/mbedtls, v3.6.x)")
else()
    message(STATUS "tinydesk: no mbedTLS found, MQTT over TLS disabled")
endif()

# ---------------------------------------------------------------- apps
add_library(tinydesk_apps STATIC
    apps/about.c
    apps/apps.c
    apps/counter.c
    apps/pixels.c
    apps/imagefile.c
    apps/imageview.c
    apps/mouse.c
    apps/keyboard.c
    apps/datetime.c
    apps/desktop.c
    apps/editor.c
    apps/editor_py.c
    apps/files.c
    apps/logview.c
    apps/modbus.c
    apps/mqtt.c
    apps/network.c
    apps/displays.c
    apps/pyui.c
    apps/pyui_cache.c
    apps/pyui_font.c
    apps/trash.c
    apps/search.c
    apps/kiosk.c
    apps/session.c
    apps/login.c
    apps/users.c
    apps/osk.c
    apps/store.c
    apps/remote.c
    apps/shortcut.c
    apps/settings.c
    apps/sysmon.c
    apps/taskmgr.c
    apps/terminal.c
    apps/update.c
)
target_include_directories(tinydesk_apps PUBLIC apps)
# Python canvases may cover a whole large terminal several times (pages), and
# have pixels and images, as with PSRAM on a board.
set(TD_HOST_PYUI_MAX_CELLS 32000 CACHE STRING "Host canvas cells (2000 to exercise small-board limits)")
set(TD_HOST_PYUI_MAX_PIXELS 2097152 CACHE STRING "Host pixel bytes (16384 to exercise small-board limits)")
target_compile_definitions(tinydesk_apps PUBLIC TD_PYUI_MAX_CELLS=${TD_HOST_PYUI_MAX_CELLS} TD_PYUI_MAX_PIXELS=${TD_HOST_PYUI_MAX_PIXELS})
target_link_libraries(tinydesk_apps PUBLIC tinydesk_core tinydesk_proto)
set_target_properties(tinydesk_apps PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
target_compile_options(tinydesk_apps PRIVATE ${TD_WARNINGS})

# ------------------------------------------------------------ host HAL
if(WIN32)
    add_library(tinydesk_hal STATIC ports/windows/hal_win.c)
else()
    add_library(tinydesk_hal STATIC ports/posix/hal_posix.c)
endif()
target_include_directories(tinydesk_hal PUBLIC ports/common)
target_link_libraries(tinydesk_hal PUBLIC tinydesk_core)
set_target_properties(tinydesk_hal PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
target_compile_options(tinydesk_hal PRIVATE ${TD_WARNINGS})

# -------------------------------------------------------------- TinyDesk Shell
if(TD_WITH_TDSH)
    set(TDSH_CORE_SRCS
        ${TDSH_ROOT}/src/core/tdsh_builtin.c
        ${TDSH_ROOT}/src/core/tdsh_core.c
        ${TDSH_ROOT}/src/core/tdsh_parser.c
        ${TDSH_ROOT}/src/core/tdsh_path.c
        ${TDSH_ROOT}/src/core/tdsh_script.c
        ${TDSH_ROOT}/src/core/tdsh_terminal.c
        ${TDSH_ROOT}/src/core/tdsh_board.c
    )
    if(WIN32)
        # The TinyDesk Shell core expects newlib/glibc stdio; the compat layer adds
        # per-thread stdin/stdout and funopen() on the Windows C runtime.
        # (TinyDesk Shell's Windows port, ports/windows in the submodule.)
        set(TDSH_WIN_DIR ${TDSH_ROOT}/ports/windows)
        # The compat layer itself must see the real C-runtime stdio, so it is
        # built separately, without the forced include.
        add_library(tdsh_wincompat STATIC ${TDSH_WIN_DIR}/tdsh_win_compat.c)
        target_include_directories(tdsh_wincompat PUBLIC ${TDSH_WIN_DIR})
        add_library(tdsh STATIC
            ${TDSH_CORE_SRCS}
            ${TDSH_WIN_DIR}/tdsh_platform_win.c
            ${TDSH_WIN_DIR}/tdsh_win_commands.c
        )
        target_include_directories(tdsh PUBLIC ${TDSH_ROOT}/include ${TDSH_WIN_DIR})
        # ifconfig and ping use the IP Helper API.
        target_link_libraries(tdsh PUBLIC tdsh_wincompat iphlpapi ws2_32)
        set(TDSH_COMPAT_FLAG -include ${TDSH_WIN_DIR}/tdsh_win_compat.h)
        target_compile_options(tdsh PRIVATE ${TDSH_COMPAT_FLAG})
    else()
        find_package(Threads REQUIRED)
        add_library(tdsh STATIC
            ${TDSH_CORE_SRCS}
            ${TDSH_ROOT}/ports/posix/tdsh_posix.c
            ${TDSH_ROOT}/ports/posix/tdsh_posix_commands.c
            ${TDSH_ROOT}/ports/posix/tdsh_nano_posix.c
        )
        target_include_directories(tdsh PUBLIC ${TDSH_ROOT}/include ${TDSH_ROOT}/ports/posix)
        target_link_libraries(tdsh PUBLIC Threads::Threads)
    endif()
    # Third-party code: built as GNU C11, without tinydesk's -Werror.
    set_target_properties(tdsh PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
    target_compile_options(tdsh PRIVATE -w)
endif()

# ------------------------------------------------- MicroPython (py)
# The interpreter from third_party/micropython (generated by
# tools/micropython) and TinyDesk's port of it in ports/common/python.
if(TD_WITH_TDSH AND TD_WITH_PYTHON)
    set(TD_MICROPYTHON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/micropython)
    include(${TD_MICROPYTHON_DIR}/sources.cmake)
    set(TD_PY_PORT_SRCS
        ports/common/python/td_py.c
        ports/common/python/td_py_machine.c
        ports/common/python/td_py_mod.c
        ports/common/python/td_py_tdui.c
        ports/common/python/td_py_vfs.c
        ports/common/python/hw/td_py_hw_host.c   # machine: no hardware on a PC
    )
    # One library: the interpreter's module table refers to the tinydesk
    # module in the port files.
    add_library(td_python STATIC ${TD_MICROPYTHON_SRCS} ${TD_PY_PORT_SRCS})
    target_include_directories(td_python PUBLIC ${TD_MICROPYTHON_DIR} ports/common/python)
    target_link_libraries(td_python PUBLIC tdsh tinydesk_apps)
    if(NOT WIN32)
        target_link_libraries(td_python PUBLIC m)   # floats: libm is separate on Linux
    endif()
    target_compile_definitions(td_python PUBLIC TD_WITH_PYTHON)
    if(TD_MBEDTLS_DIR)
        # hashlib: sha1 and md5 come from mbedTLS (sha256 without it too)
        target_compile_definitions(td_python PRIVATE TD_PY_MBEDTLS)
        target_link_libraries(td_python PRIVATE mbedcrypto)
    endif()
    set_target_properties(td_python PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
    set_source_files_properties(${TD_MICROPYTHON_SRCS} PROPERTIES COMPILE_OPTIONS -w)   # third-party code
    # vfs_posix_file.c also defines sys.stdin/stdout/stderr on file
    # descriptors 0-2 (the process's console, the UART on a board). Rename
    # them, so sys_stdio_mphal.c's, which go through the shell session, are
    # the ones linked.
    set_source_files_properties(${TD_MICROPYTHON_DIR}/extmod/vfs_posix_file.c PROPERTIES COMPILE_DEFINITIONS
        "mp_sys_stdin_obj=td_py_posix_stdin_unused;mp_sys_stdout_obj=td_py_posix_stdout_unused;mp_sys_stderr_obj=td_py_posix_stderr_unused")
    # -Wno-unused-parameter: MicroPython's headers have inline stubs.
    set(TD_PY_WARNINGS -Wall -Wextra -Wno-unused-parameter -Werror)
    set_source_files_properties(${TD_PY_PORT_SRCS} PROPERTIES COMPILE_OPTIONS "${TD_PY_WARNINGS}")
    if(WIN32)
        # td_py.c reads and writes the shell thread's stdin/stdout.
        set_source_files_properties(ports/common/python/td_py.c PROPERTIES
            COMPILE_OPTIONS "${TD_PY_WARNINGS};${TDSH_COMPAT_FLAG}")
    endif()
endif()

# ---------------------------------------------------------- executable
# Downloads (ports/common/host_fetch.c) run in a thread.
find_package(Threads REQUIRED)
add_executable(tinydesk
    ports/common/host_main.c
    ports/common/td_fs_stdio.c
    ports/common/host_fetch.c
    $<IF:$<BOOL:${WIN32}>,ports/windows/main.c,ports/posix/main.c>
)
target_link_libraries(tinydesk PRIVATE tinydesk_apps tinydesk_hal Threads::Threads)
set_target_properties(tinydesk PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
target_compile_options(tinydesk PRIVATE ${TD_WARNINGS})
if(TD_WITH_TDSH)
    target_sources(tinydesk PRIVATE ports/common/tdsh_bridge_host.c ports/common/td_proto_cmds.c)
    target_compile_definitions(tinydesk PRIVATE TD_WITH_TDSH)
    target_link_libraries(tinydesk PRIVATE tdsh)
    if(TD_WITH_PYTHON)
        target_link_libraries(tinydesk PRIVATE td_python)
    endif()
    if(WIN32)
        # The bridge assigns stdin/stdout for the shell thread, and the
        # commands print through the shell thread's stdout.
        set_source_files_properties(ports/common/tdsh_bridge_host.c ports/common/td_proto_cmds.c PROPERTIES
            COMPILE_OPTIONS "${TDSH_COMPAT_FLAG}")
    endif()
endif()

# Simulated terminal for end-to-end checks (see tools/tdsim.c).
add_executable(tdsim
    tools/tdsim.c
    tools/net_fake.c
    ports/common/host_main.c
    ports/common/td_fs_stdio.c
    ports/common/host_fetch.c
)
target_link_libraries(tdsim PRIVATE tinydesk_apps tinydesk_hal Threads::Threads)
set_target_properties(tdsim PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
target_compile_options(tdsim PRIVATE ${TD_WARNINGS})
if(TD_WITH_TDSH)
    target_sources(tdsim PRIVATE ports/common/tdsh_bridge_host.c ports/common/td_proto_cmds.c)
    target_compile_definitions(tdsim PRIVATE TD_WITH_TDSH)
    target_link_libraries(tdsim PRIVATE tdsh)
    if(TD_WITH_PYTHON)
        target_link_libraries(tdsim PRIVATE td_python)
    endif()
endif()

# Render a raw capture from real hardware (see tools/serial_probe.py).
if(TD_BUILD_TESTS)
    add_executable(tdsim_lcd tools/tdsim.c tools/sim_lcd.c tools/net_fake.c
        ports/common/host_main.c ports/common/td_fs_stdio.c ports/common/host_fetch.c
        ports/crowpanel7/main/font8x16.c ports/crowpanel7/main/font10x20.c
        ports/crowpanel7/main/font12x24.c ports/crowpanel7/main/font16x32.c)
    target_include_directories(tdsim_lcd PRIVATE tests)
    target_compile_definitions(tdsim_lcd PRIVATE TD_SIM_LCD)
    target_link_libraries(tdsim_lcd PRIVATE tinydesk_apps tinydesk_hal Threads::Threads)
    set_target_properties(tdsim_lcd PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
    target_compile_options(tdsim_lcd PRIVATE ${TD_WARNINGS})
    if(TD_WITH_TDSH)
        target_sources(tdsim_lcd PRIVATE ports/common/tdsh_bridge_host.c ports/common/td_proto_cmds.c)
        target_compile_definitions(tdsim_lcd PRIVATE TD_WITH_TDSH)
        target_link_libraries(tdsim_lcd PRIVATE tdsh)
        if(TD_WITH_PYTHON)
            target_link_libraries(tdsim_lcd PRIVATE td_python)
        endif()
    endif()
endif()

add_executable(vtshot tools/vtshot.c)
target_link_libraries(vtshot PRIVATE tinydesk_core)
set_target_properties(vtshot PROPERTIES C_STANDARD 11)
target_compile_options(vtshot PRIVATE ${TD_WARNINGS})

# ------------------------------------------------------------ examples
if(TD_BUILD_EXAMPLES)
    foreach(ex hello_window counter_app)
        add_executable(${ex} examples/${ex}.c)
        target_link_libraries(${ex} PRIVATE tinydesk_core tinydesk_hal)
        set_target_properties(${ex} PROPERTIES C_STANDARD 11)
        target_compile_options(${ex} PRIVATE ${TD_WARNINGS})
    endforeach()
endif()

# --------------------------------------------------------------- tests
if(TD_BUILD_TESTS)
    enable_testing()
    add_executable(test_console_access ${TDSH_ROOT}/tests/test_console_access.c)
    target_include_directories(test_console_access PRIVATE ${TDSH_ROOT}/include)
    set_target_properties(test_console_access PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
    target_compile_options(test_console_access PRIVATE ${TD_WARNINGS})
    add_test(NAME test_console_access COMMAND test_console_access)
    add_executable(test_permissions ${TDSH_ROOT}/tests/test_permissions.c)
    target_link_libraries(test_permissions PRIVATE tdsh)
    add_test(NAME test_permissions COMMAND test_permissions)
    if(TARGET mbedcrypto AND NOT MSVC)
        add_executable(test_users_storage ${TDSH_ROOT}/tests/test_users_storage.c)
        target_include_directories(test_users_storage PRIVATE ${TDSH_ROOT}/tests/users_stubs)
        target_link_libraries(test_users_storage PRIVATE tdsh mbedcrypto)
        target_compile_options(test_users_storage PRIVATE -Wall -Wextra -Werror)
        add_test(NAME test_users_storage COMMAND test_users_storage)
    endif()
    foreach(t test_utf8 test_input test_render test_vterm test_wm test_select test_widgets test_canvas test_link test_font)
        add_executable(${t} tests/${t}.c)
        target_link_libraries(${t} PRIVATE tinydesk_core)
        set_target_properties(${t} PROPERTIES C_STANDARD 11)
        target_compile_options(${t} PRIVATE ${TD_WARNINGS})
        add_test(NAME ${t} COMMAND ${t})
    endforeach()
    add_executable(test_proto tests/test_proto.c)
    target_link_libraries(test_proto PRIVATE tinydesk_proto)
    set_target_properties(test_proto PROPERTIES C_STANDARD 11)
    target_compile_options(test_proto PRIVATE ${TD_WARNINGS})
    add_test(NAME test_proto COMMAND test_proto)
    # The tdui contract file (for app makers) against the C and Python code.
    find_package(Python3 COMPONENTS Interpreter QUIET)
    if(Python3_Interpreter_FOUND)
        add_test(NAME tdui_contract COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_tdui_contract.py)
        # The tdui frozen into the firmware is tdui.py (run in the simulator).
        if(TD_WITH_TDSH AND TD_WITH_PYTHON)
            add_test(NAME tdui_frozen COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_tdui_frozen.py
                     $<TARGET_FILE:tdsim>)
            # The app library (tdapp) works there: pages, links, bindings.
            add_test(NAME tdapp_works COMMAND Python3::Interpreter ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_tdapp.py
                     $<TARGET_FILE:tdsim>)
        endif()
    endif()
    add_executable(test_editor tests/test_editor.c)
    target_link_libraries(test_editor PRIVATE tinydesk_apps)
    set_target_properties(test_editor PROPERTIES C_STANDARD 11)
    target_compile_options(test_editor PRIVATE ${TD_WARNINGS})
    add_test(NAME test_editor COMMAND test_editor)
    add_executable(test_columns tests/test_columns.c)
    target_link_libraries(test_columns PRIVATE tinydesk_apps)
    set_target_properties(test_columns PROPERTIES C_STANDARD 11)
    target_compile_options(test_columns PRIVATE ${TD_WARNINGS})
    add_test(NAME test_columns COMMAND test_columns)
    if(TD_WITH_TDSH AND TD_WITH_PYTHON)
        add_executable(test_py_pins tests/test_py_pins.c ports/common/python/hw/td_py_hw_pins.c)
        target_include_directories(test_py_pins PRIVATE ports/common/python/hw)
        target_link_libraries(test_py_pins PRIVATE tdsh)
        set_target_properties(test_py_pins PROPERTIES C_STANDARD 11)
        target_compile_options(test_py_pins PRIVATE ${TD_WARNINGS})
        add_test(NAME test_py_pins COMMAND test_py_pins)
    endif()
    add_executable(test_desktop tests/test_desktop.c)
    target_link_libraries(test_desktop PRIVATE tinydesk_apps)
    set_target_properties(test_desktop PROPERTIES C_STANDARD 11)
    target_compile_options(test_desktop PRIVATE ${TD_WARNINGS})
    add_test(NAME test_desktop COMMAND test_desktop)
    add_executable(test_trash tests/test_trash.c ports/common/td_fs_stdio.c)
    target_include_directories(test_trash PRIVATE ports/common)
    target_link_libraries(test_trash PRIVATE tinydesk_apps)
    set_target_properties(test_trash PROPERTIES C_STANDARD 11)
    target_compile_options(test_trash PRIVATE ${TD_WARNINGS})
    add_test(NAME test_trash COMMAND test_trash)
    add_executable(test_osk tests/test_osk.c)
    target_link_libraries(test_osk PRIVATE tinydesk_apps)
    set_target_properties(test_osk PROPERTIES C_STANDARD 11)
    target_compile_options(test_osk PRIVATE ${TD_WARNINGS})
    add_test(NAME test_osk COMMAND test_osk)
    add_executable(test_screens tests/test_screens.c)
    target_link_libraries(test_screens PRIVATE tinydesk_apps)
    set_target_properties(test_screens PROPERTIES C_STANDARD 11)
    target_compile_options(test_screens PRIVATE ${TD_WARNINGS})
    add_test(NAME test_screens COMMAND test_screens)
    add_executable(test_store tests/test_store.c)
    target_link_libraries(test_store PRIVATE tinydesk_apps)
    set_target_properties(test_store PROPERTIES C_STANDARD 11)
    target_compile_options(test_store PRIVATE ${TD_WARNINGS})
    add_test(NAME test_store COMMAND test_store)
    add_executable(test_pyui tests/test_pyui.c)
    target_link_libraries(test_pyui PRIVATE tinydesk_apps)
    set_target_properties(test_pyui PROPERTIES C_STANDARD 11)
    target_compile_options(test_pyui PRIVATE ${TD_WARNINGS})
    add_test(NAME test_pyui COMMAND test_pyui)
    add_executable(test_session tests/test_session.c)
    target_link_libraries(test_session PRIVATE tinydesk_apps)
    target_compile_options(test_session PRIVATE ${TD_WARNINGS})
    add_test(NAME test_session COMMAND test_session)
    add_executable(test_remembered_login tests/test_remembered_login.c)
    target_link_libraries(test_remembered_login PRIVATE tinydesk_apps)
    target_compile_options(test_remembered_login PRIVATE ${TD_WARNINGS})
    add_test(NAME test_remembered_login COMMAND test_remembered_login)
    add_test(NAME test_remembered_timeout COMMAND test_remembered_login timeout)
    add_executable(test_image tests/test_image.c ports/common/td_fs_stdio.c)
    target_include_directories(test_image PRIVATE ports/common)
    target_link_libraries(test_image PRIVATE tinydesk_apps)
    set_target_properties(test_image PROPERTIES C_STANDARD 11)
    target_compile_options(test_image PRIVATE ${TD_WARNINGS})
    add_test(NAME test_image COMMAND test_image)
    add_executable(test_native_apps tests/test_native_apps.c)
    target_link_libraries(test_native_apps PRIVATE tinydesk_apps)
    add_test(NAME test_native_apps COMMAND test_native_apps)
    add_executable(test_task_snapshot tests/test_task_snapshot.c)
    target_include_directories(test_task_snapshot PRIVATE tests)
    target_link_libraries(test_task_snapshot PRIVATE tinydesk_core)
    add_test(NAME test_task_snapshot COMMAND test_task_snapshot)
    add_executable(test_crowpanel_display tests/test_crowpanel_display.c)
    target_include_directories(test_crowpanel_display PRIVATE tests)
    target_link_libraries(test_crowpanel_display PRIVATE tinydesk_core)
    set_target_properties(test_crowpanel_display PROPERTIES C_STANDARD 11)
    target_compile_options(test_crowpanel_display PRIVATE ${TD_WARNINGS})
    add_test(NAME test_crowpanel_display COMMAND test_crowpanel_display)
    add_executable(test_pyui_cache tests/test_pyui_cache.c apps/pyui_cache.c)
    target_include_directories(test_pyui_cache PRIVATE tests apps)
    set_target_properties(test_pyui_cache PROPERTIES C_STANDARD 11)
    target_compile_options(test_pyui_cache PRIVATE ${TD_WARNINGS})
    add_test(NAME test_pyui_cache COMMAND test_pyui_cache)
endif()
