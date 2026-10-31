# The platform-independent tinydesk core and apps, used straight from the
# repository's src/, include/ and apps/ (single source of truth).
get_filename_component(TD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)

idf_component_register(
    SRCS
        "${TD_ROOT}/src/draw.c"
        "${TD_ROOT}/src/event.c"
        "${TD_ROOT}/src/input.c"
        "${TD_ROOT}/src/render.c"
        "${TD_ROOT}/src/screen.c"
        "${TD_ROOT}/src/td.c"
        "${TD_ROOT}/src/theme.c"
        "${TD_ROOT}/src/timer.c"
        "${TD_ROOT}/src/utf8.c"
        "${TD_ROOT}/src/vterm.c"
        "${TD_ROOT}/src/widgets.c"
        "${TD_ROOT}/src/wm.c"
        "${TD_ROOT}/apps/about.c"
        "${TD_ROOT}/apps/apps.c"
        "${TD_ROOT}/apps/counter.c"
        "${TD_ROOT}/apps/datetime.c"
        "${TD_ROOT}/apps/desktop.c"
        "${TD_ROOT}/apps/editor.c"
        "${TD_ROOT}/apps/editor_py.c"
        "${TD_ROOT}/apps/files.c"
        "${TD_ROOT}/apps/logview.c"
        "${TD_ROOT}/apps/modbus.c"
        "${TD_ROOT}/apps/mqtt.c"
        "${TD_ROOT}/apps/network.c"
        "${TD_ROOT}/apps/pyui.c"
        "${TD_ROOT}/apps/session.c"
        "${TD_ROOT}/apps/settings.c"
        "${TD_ROOT}/apps/sysmon.c"
        "${TD_ROOT}/apps/taskmgr.c"
        "${TD_ROOT}/apps/terminal.c"
        "${TD_ROOT}/apps/update.c"
        "${TD_ROOT}/ports/common/td_fs_stdio.c"
        "${TD_ROOT}/proto/td_modbus.c"
        "${TD_ROOT}/proto/td_mqtt.c"
        "${TD_ROOT}/proto/td_mqtt_conf.c"
        "${TD_ROOT}/proto/td_sock.c"
        "${TD_ROOT}/proto/td_tls.c"
    INCLUDE_DIRS
        "${TD_ROOT}/include"
        "${TD_ROOT}/apps"
        "${TD_ROOT}/ports/common"
        "${TD_ROOT}/proto"
    REQUIRES vfs lwip esp_timer mbedtls
)

# Screen and terminal limits. The screen buffers cost 2 x cols x rows x 8
# bytes (32 KB at 80x25); the Terminal adds cols x rows x 4 bytes plus 4
# bytes per cell of scrollback. The desktop follows the terminal's size up
# to TD_MAX_COLS x TD_MAX_ROWS (tinydesk asks for the size every second).
#
# Without PSRAM (the C6) these come out of internal RAM, which TinyDesk Shell, Wi-Fi
# and SSH need, so the limit is 80x25 (100x30 would cost +16 KB). With PSRAM
# (the classic ESP32 port, whose tinydesk statics are in PSRAM, see
# ports/esp32/main/extram.lf) a maximised PuTTY fits (about 190x50 on a
# 1920-pixel screen): 256x96 costs about 390 KB of screen buffers and 300 KB
# of terminal with 200 lines of scrollback, out of 4 MB.
#
# The widget pool is shared by all windows (about 144 bytes a widget on the
# ESP32; all apps open need about 115): 96 without PSRAM (14 KB of internal
# RAM), the default 128 with it. A window that still does not fit, or gets
# no timer, is closed with a message instead of opening with missing parts.
if(CONFIG_SPIRAM)
    set(TD_SCREEN_LIMITS TD_MAX_COLS=256 TD_MAX_ROWS=96 TD_VT_SCROLLBACK=200)
else()
    set(TD_SCREEN_LIMITS TD_MAX_COLS=80 TD_MAX_ROWS=25 TD_VT_SCROLLBACK=12 TD_MAX_WIDGETS=96)
endif()
target_compile_definitions(${COMPONENT_LIB} PUBLIC
    TD_HAVE_TLS
    ${TD_SCREEN_LIMITS}
    TD_OUT_BUF_SIZE=1024
    TD_TEXT_MAX=48
    TD_LOG_LINES=32
    TD_LOG_LINE_MAX=88
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)
