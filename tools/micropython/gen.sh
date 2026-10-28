#!/bin/sh
# Regenerate third_party/micropython (see README.md next to this file).
#
#   tools/micropython/gen.sh /path/to/micropython
#
# The checkout must be the release named in third_party/micropython/README.md
# (or the one you are moving to). Needs make, gcc and python3: Linux, or WSL
# on Windows.
set -e
if [ $# -ne 1 ] || [ ! -f "$1/py/mkenv.mk" ]; then
    echo "usage: $0 /path/to/micropython" >&2
    exit 2
fi
top=$(cd "$1" && pwd)
cd "$(dirname "$0")"
make clean MICROPYTHON_TOP="$top" >/dev/null
make MICROPYTHON_TOP="$top"
echo "Done. Check: git -C ../.. status third_party/micropython"
