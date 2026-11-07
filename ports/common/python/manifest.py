# Python modules frozen into TinyDesk's MicroPython: the shell's (asyncio,
# neopixel) and TinyDesk's own. tools/micropython/gen.sh compiles them to
# bytecode into ports/common/python/gen/frozen.
include("$(TDSH_PY_DIR)/manifest.py")

# tdui: windows on the desktop, over the _tdui module.
module("tdui.py", base_path="$(TD_PY_DIR)/modules", opt=3)

# tdapp: what most tdui apps share (pages, canvas elements, Modbus / UART
# links with bound widgets), so an app keeps only its own logic.
module("tdapp.py", base_path="$(TD_PY_DIR)/modules", opt=3)
