# Regenerating third_party/micropython

TinyDesk builds MicroPython from the sources in
[`third_party/micropython`](../../third_party/micropython/README.md). Some
of them are generated from the configuration: the qstr table (every name
the interpreter knows), the list of built-in modules and the root pointers.
Regenerate them after changing any of these:

* [`ports/common/python/mpconfigport.h`](../../ports/common/python/mpconfigport.h)
  (features and modules);
* a `MP_QSTR_...`, `MP_REGISTER_MODULE` or `MP_REGISTER_ROOT_POINTER` in
  `ports/common/python/*.c` (a new function in the `tinydesk` module, for
  example);
* the MicroPython release (also update the version in
  `third_party/micropython/README.md`).

It needs make, gcc and python3, so run it on Linux, or in WSL on Windows:

```bash
git clone --depth 1 --branch v1.29.0 https://github.com/micropython/micropython.git /tmp/micropython
tools/micropython/gen.sh /tmp/micropython
```

`gen.sh` runs this directory's `Makefile` (MicroPython's own build rules,
the way its `ports/embed` does it) and replaces everything in
`third_party/micropython` except its README. Commit the result with the
change that needed it.

The preprocessor here runs without any platform macros, so code inside
`#ifdef ESP_PLATFORM` or `#ifdef _WIN32` in the port files must not use
qstrs: a name that only one platform sees would be missing from the table.
