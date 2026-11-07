# Regenerating TinyDesk's MicroPython tables

TinyDesk's `py` is TinyDesk Shell's MicroPython
([`third_party/tdsh/python`](../../third_party/tdsh/python/README.md): the
interpreter, the `py` command, files, `machine`, `asyncio`, `neopixel`)
with TinyDesk's own modules added: `tinydesk` (`td_py_mod.c`), `_tdui`
(`td_py_tdui.c`) and the frozen `tdui` and `tdapp`
(`ports/common/python/modules`). MicroPython lists every module and name
in tables generated at build time, so TinyDesk keeps its own:
`ports/common/python/gen` (`genhdr/`, `frozen/`), used by every build
instead of the shell's.

Regenerate them after changing a `MP_QSTR_...`, `MP_REGISTER_MODULE` or
`MP_REGISTER_ROOT_POINTER` in `ports/common/python/*.c`,
`ports/common/python/manifest.py`, `qstrdefs_tinydesk.h`, `tdui.py` or
`tdapp.py`, and after the shell's Python changed in a way that needs it
(its `tools/micropython/README.md` says when):

```bash
git clone --depth 1 --branch v1.29.0 https://github.com/micropython/micropython.git ../mp
tools/micropython/gen.sh ../mp
```

`gen.sh` runs the shell's generator with TinyDesk's sources, qstrs and
manifest. It needs make, gcc and Python 3: Linux, WSL, or Git Bash on
Windows with MinGW-w64 (keep the checkout on a short path there). Commit
the result with the change that needed it. The ctests `tdui_frozen` and
`tdapp_works` fail when `tdui.py` or `tdapp.py` and the frozen ones
differ, that is when this was forgotten; after a change to their public
names, `tools/check_tdui_contract.py --record` writes them into the
contracts.
