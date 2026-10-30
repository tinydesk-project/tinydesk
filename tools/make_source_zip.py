#!/usr/bin/env python3
"""make_source_zip.py - one zip of the whole project for review or sharing.

    python tools/make_source_zip.py [--site ../tinydesk-site] [--out ..]

Packs the tinydesk repository (TinyDesk Shell included, as third_party/tdsh)
and, only when explicitly selected with --site, the documentation site into
TinyDesk-<version>-source.zip, with a short guide at the top. Left out:
build output, downloaded components, per-machine configuration
(sdkconfig), logs, run-time data, release files, and every private
board.conf (only the board.example.conf files go in).
"""
import argparse
import os
import re
import sys
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

SKIP_DIRS = {"build", "dist", "managed_components", "tinydesk_fs", "local-review", "backups", "__pycache__", ".git", ".vscode", ".idea",
             "firmware", "downloads"}
SKIP_FILES = {".git", ".env", "board.conf", "sdkconfig", "sdkconfig.old", "tinydesk_settings.bin", ".DS_Store", "Thumbs.db"}
SKIP_EXT = (".log", ".bin", ".elf", ".map", ".pyc", ".zip", ".tar.gz")


def skipped_dir(name):
    return name in SKIP_DIRS or name.startswith("build-")


def add_tree(z, src, arc_root, counts):
    for dirpath, dirnames, filenames in os.walk(src):
        rel = os.path.relpath(dirpath, src).replace(os.sep, "/")
        # firmware/ and downloads/ are release files only inside the site's install/.
        dirnames[:] = sorted(d for d in dirnames if not skipped_dir(d) or
                             (d in ("firmware", "downloads") and not rel.endswith("install")))
        for f in sorted(filenames):
            if f in SKIP_FILES or f.endswith(SKIP_EXT):
                continue
            if rel.endswith("install") and re.match(r"manifest-.*\.json$|SHA256SUMS\.txt$|README\.txt$", f):
                continue
            path = os.path.join(dirpath, f)
            arc = "%s/%s" % (arc_root, (f if rel == "." else rel + "/" + f))
            z.write(path, arc)
            counts[0] += 1
            counts[1] += os.path.getsize(path)


def version():
    with open(os.path.join(ROOT, "include", "tinydesk", "td.h"), encoding="utf-8") as f:
        m = re.search(r'#define TD_VERSION "([^"]+)"', f.read())
    return m.group(1) if m else "dev"


GUIDE = """# TinyDesk {ver}: the whole project

TinyDesk is a windowed desktop for microcontrollers, drawn in a UTF-8
terminal with ANSI/VT cursor control and xterm mouse reporting.

| Folder | What it is | Where it lives |
| --- | --- | --- |
| `tinydesk/` | the desktop: portable C11 core (`src/`, `include/`), apps (`apps/`), protocols (`proto/`), ports (`ports/esp_idf` shared by `ports/esp32c6`, `ports/esp32`, `ports/esp32-4mb`, `ports/esp32s3`; host), tools, tests | the `tinydesk` git repository |
| `tinydesk/third_party/tdsh/` | TinyDesk Shell (`tdsh`), the shell inside the desktop's Terminal and a standalone firmware of its own (`projects/esp32` for the classic ESP32, `projects/esp32s3` for the ESP32-S3) | the `tinydesk-shell` repository, a git submodule of `tinydesk` |

Good starting points:

* `tinydesk/README.md`, then `tinydesk/docs/GETTING_STARTED.md`
* core API: `tinydesk/include/tinydesk/td.h`
* the shell: `tinydesk/third_party/tdsh/README.md`, `CHANGELOG.md`
* the board configuration (pins): each port's `board.example.conf`

Left out of this archive: build output, downloaded ESP-IDF components,
sdkconfig files (regenerated from sdkconfig.defaults), release images, and
private `board.conf` files, hardware backups and local review records
(the `board.example.conf` files are included). A documentation site is
included only when the archive was created with an explicit `--site` path.
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--site", help="Explicitly include this documentation site (may contain private material)")
    ap.add_argument("--out", default=os.path.dirname(ROOT))
    args = ap.parse_args()
    ver = version()
    top = "TinyDesk-%s" % ver
    out = os.path.join(args.out, "%s-source.zip" % top)
    counts = [0, 0]
    with zipfile.ZipFile(out, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        z.writestr(top + "/README.md", GUIDE.format(ver=ver))
        add_tree(z, ROOT, top + "/tinydesk", counts)
        if args.site and os.path.isdir(args.site):
            add_tree(z, args.site, top + "/tinydesk-site", counts)
        elif args.site:
            print("note: no site folder at %s" % args.site)
    print("%s: %d files, %.1f MB of sources, %.1f MB zipped"
          % (out, counts[0], counts[1] / 1e6, os.path.getsize(out) / 1e6))


if __name__ == "__main__":
    sys.exit(main())
