#!/bin/sh
# Regenerate ports/common/python/gen: MicroPython's generated headers and
# the frozen modules for TinyDesk, that is TinyDesk Shell's Python
# (third_party/tdsh/python) plus TinyDesk's own modules (tinydesk, _tdui,
# tdui, tdapp). Runs the shell's generator; see README.md next to this file.
#
#   tools/micropython/gen.sh /path/to/micropython
set -e
if [ $# -ne 1 ] || [ ! -f "$1/py/mkenv.mk" ]; then
    echo "usage: $0 /path/to/micropython" >&2
    exit 2
fi
root=$(cd "$(dirname "$0")/../.." && pwd)
case "$(uname -s)" in
MINGW* | MSYS*) root=$(cd "$root" && pwd -W) ;;   # native make needs Windows paths
esac
py="$root/ports/common/python"
exec sh "$root/third_party/tdsh/tools/micropython/gen.sh" "$1" \
    EXT_C="$py/td_py_mod.c $py/td_py_tdui.c" \
    EXT_CFLAGS="-I$py -I$root/include -I$root/apps -I$root/proto" \
    EXT_QSTR_DEFS="$py/qstrdefs_tinydesk.h" \
    MANIFEST="$py/manifest.py" \
    MANIFEST_VARS="TD_PY_DIR=$py" \
    OUT="$py/gen"
