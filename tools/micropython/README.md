# Regenerating third_party/micropython

TinyDesk builds MicroPython from the sources in
[`third_party/micropython`](../../third_party/micropython/README.md). Some
of them are generated from the configuration: the qstr table (every name
the interpreter knows), the list of built-in modules, the root pointers,
and the frozen Python modules (`asyncio` and `neopixel`, compiled to
bytecode).
Regenerate them after changing any of these:

* [`ports/common/python/mpconfigport.h`](../../ports/common/python/mpconfigport.h)
  (features and modules);
* a `MP_QSTR_...`, `MP_REGISTER_MODULE` or `MP_REGISTER_ROOT_POINTER` in
  `ports/common/python/*.c` (a new function in the `tinydesk` module, for
  example);
* [`ports/common/python/manifest.py`](../../ports/common/python/manifest.py)
  (the frozen modules) or
  [`qstrdefsport.h`](../../ports/common/python/qstrdefsport.h);
* the MicroPython release (also update the version in
  `third_party/micropython/README.md`).

It needs make, gcc and python3, so run it on Linux, or in WSL on Windows:

```bash
git clone --depth 1 --branch v1.29.0 https://github.com/micropython/micropython.git /tmp/micropython
tools/micropython/gen.sh /tmp/micropython
```

`gen.sh` runs this directory's `Makefile` (MicroPython's own build rules,
the way its `ports/embed` does it). The first run also builds `mpy-cross`
in the checkout, which compiles the frozen modules; the same `mpy-cross`
(`mpy-cross/build/mpy-cross`) makes `.mpy` files for `py` (see the shell
documentation). It replaces everything in
`third_party/micropython` except its README. Commit the result with the
change that needed it.

The preprocessor here runs without any platform macros, so code inside
`#ifdef ESP_PLATFORM` or `#ifdef _WIN32` in the port files must not use
qstrs: a name that only one platform sees would be missing from the table.
Names the generator cannot see in use go in `qstrdefsport.h` instead:
the ones from code that needs mbedTLS (`hashlib.sha1`, `hashlib.md5`), and
names with escaped characters such as `MP_QSTR__slash_lib` ("/lib").
The native-code emitters are per chip; the `Makefile` defines
`TD_PY_GENHDR`, which turns on both that TinyDesk uses (Xtensa and x64),
so the tables have the names of each.

The generator reads `ports/common/python/*.c` (and the `td_py_machine_*.h`
files that MicroPython's `extmod/machine_*.c` include), so those must not
include ESP-IDF headers: the `machine` module's Python side calls the
plain-C interface `td_py_hw.h`, whose implementations live in
`ports/common/python/hw/` (`td_py_hw_esp.c` with ESP-IDF's drivers,
`td_py_hw_host.c` for the PC programs), which the generator does not read.
`drivers/bus/softspi.c` (for `SoftSPI`) is packaged too.
