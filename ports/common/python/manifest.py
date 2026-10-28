# Python modules frozen into TinyDesk's MicroPython (compiled to bytecode by
# tools/micropython into third_party/micropython/frozen_content.c).

# asyncio; its C part (_asyncio) and select are built in (mpconfigport.h).
package(
    "asyncio",
    ("__init__.py", "core.py", "event.py", "funcs.py", "lock.py", "stream.py"),
    base_path="$(MPY_DIR)/extmod",
    opt=3,
)
