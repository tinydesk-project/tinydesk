# MicroPython for TinyDesk: TinyDesk Shell's (third_party/tdsh: the
# interpreter, the py command, machine) with TinyDesk's own modules
# (tinydesk, _tdui; tdui and tdapp frozen) and the tables generated for
# both in ports/common/python/gen (tools/micropython/gen.sh). This
# component replaces the shell's micropython component.
get_filename_component(TD_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
include("${TD_ROOT}/third_party/tdsh/ports/esp_idf/components/micropython/micropython_component.cmake")
tdsh_micropython_component(
    GEN_DIR "${TD_ROOT}/ports/common/python/gen"
    EXTRA_SRCS "${TD_ROOT}/ports/common/python/td_py_mod.c" "${TD_ROOT}/ports/common/python/td_py_tdui.c"
    EXTRA_INCLUDES "${TD_ROOT}/ports/common/python"
    EXTRA_REQUIRES tinydesk
    BOARD_NAME TinyDesk
    HELP_EXTERNAL   # td_py_mod.c's help() text
)
