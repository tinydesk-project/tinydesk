#!/usr/bin/env python3
"""make_release.py - package the built firmware and programs for people without a toolchain.

Two editions, like a desktop and a server install of an operating system:

    TinyDesk Desktop   the windowed desktop with TinyDesk Shell in its Terminal
    TinyDesk Shell     the shell on its own, on the board's console

Run it after building the ESP-IDF projects (ports/esp32c6, ports/esp32,
ports/esp32-4mb, and for the shell edition third_party/tdsh and
third_party/tdsh/projects/esp32). For each board it merges what
`idf.py flash` writes (read from build/flash_args) into one "factory"
image, and writes a flat set of files, as they are attached to a GitHub
release:

    dist/tinydesk-<version>/
        tinydesk-<edition>-<version>-<board>-factory.bin   esptool, offset 0x0
        manifest-<edition>-<board>.json                   ESP Web Tools, one per board
        tinydesk-<edition>-<version>-<board>-app.bin       the app alone, for updates
        update-<edition>-<board>.json                     the update feed boards read
                                                           (boards with two app slots)
        tinydesk-desktop-linux-x86_64.tar.gz ...          PC programs (--host-*)
        SHA256SUMS.txt, README.txt

With --site DIR it also fills the web installer of a documentation site
(DIR/install/): the manifests, firmware/ with the images and downloads/
with the PC programs. The installer site is kept outside this repository;
see its README.

    python tools/make_release.py [--site ../tinydesk-site/site] [--out dist] [--allow-board-conf]
        [--host-linux-desktop build/tinydesk] [--host-linux-shell build-shell/tdsh_host]
        [--host-windows-desktop build/tinydesk.exe] [--host-windows-shell build-shell/tdsh_host.exe]

The shell edition is optional: boards whose build is missing are left out
(with a note). Needs esptool (in the ESP-IDF Python environment; `pip
install esptool` otherwise).
"""
import argparse
import hashlib
import io
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import time
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

EDITIONS = [
    # edition id, title, required, boards:
    #   (board id, project dir, esptool chip, ESP Web Tools chipFamily, what it needs)
    ("desktop", "TinyDesk Desktop", True, [
        ("esp32c6", "ports/esp32c6", "esp32c6", "ESP32-C6", "ESP32-C6 with 8 MB flash (e.g. ESP32-C6-DevKitC-1-N8)"),
        ("esp32", "ports/esp32", "esp32", "ESP32", "ESP32 with 16 MB flash and PSRAM (e.g. ESP32-WROVER-IE N16R8)"),
        ("esp32-4mb", "ports/esp32-4mb", "esp32", "ESP32", "ESP32 with 4 MB flash, no PSRAM (e.g. ESP32-WROOM-32 DevKitC)"),
    ]),
    ("shell", "TinyDesk Shell", False, [
        ("esp32c6", "third_party/tdsh", "esp32c6", "ESP32-C6", "ESP32-C6 with 8 MB flash"),
        ("esp32", "third_party/tdsh/projects/esp32", "esp32", "ESP32", "any ESP32 with 4 MB flash or more (PSRAM optional)"),
    ]),
]


def read_flash_args(build):
    """Return (esptool options, [(offset, path)]) from build/flash_args."""
    with open(os.path.join(build, "flash_args")) as f:
        lines = [l.strip() for l in f if l.strip()]
    opts = lines[0].split()
    parts = []
    for line in lines[1:]:
        off, path = line.split(None, 1)
        parts.append((int(off, 16), os.path.join(build, path)))
    parts.sort()
    return opts, parts


def project_version(build):
    with open(os.path.join(build, "project_description.json")) as f:
        return json.load(f)["project_version"]


def builtin_settings(build):
    """Keys set in the board configuration built into the firmware."""
    path = os.path.join(build, "esp-idf", "main", "board_builtin.conf")
    if not os.path.exists(path):
        return []
    keys = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.split("#", 1)[0].strip()
            if "=" in line and line.split("=", 1)[1].strip():
                keys.append(line.split("=", 1)[0].strip())
    return keys


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(65536), b""):
            h.update(block)
    return h.hexdigest()


def collect(args):
    """The builds per edition: {edition: (version, [(board..., build dir)])}."""
    found = {}
    for edition, title, required, boards in EDITIONS:
        builds, versions = [], set()
        for board, proj, chip, family, needs in boards:
            build = os.path.join(ROOT, proj, args.build_dir)
            if not os.path.exists(os.path.join(build, "flash_args")):
                if required:
                    sys.exit("%s for %s is not built (run idf.py build in %s)" % (title, board, proj))
                print("note: %s for %s is not built (%s), left out" % (title, board, proj))
                continue
            # Release images must carry only the example board configuration: a
            # private board.conf would publish your pins and drive them on other
            # people's boards.
            settings = builtin_settings(build)
            if settings and not args.allow_board_conf:
                sys.exit("%s for %s was built with board settings (%s): remove %s/board.conf and rebuild, "
                         "or pass --allow-board-conf for a private image" % (title, board, ", ".join(settings[:3]), proj))
            versions.add(project_version(build))
            builds.append((board, proj, chip, family, needs, build))
        if len(versions) > 1:
            sys.exit("%s: the boards are built from different versions: %s" % (title, sorted(versions)))
        if builds:
            found[edition] = (versions.pop(), builds)
    return found


def package_firmware(out, edition, title, version, builds):
    """One factory image and one ESP Web Tools manifest per board. The
    manifest names the image as firmware/<image>, where the web installer
    keeps it (ESP Web Tools picks a build by chip family only, so two ESP32
    builds cannot share a manifest)."""
    lines = []
    for board, proj, chip, family, needs, build in builds:
        opts, parts = read_flash_args(build)
        factory = "tinydesk-%s-%s-%s-factory.bin" % (edition, version, board)
        cmd = [sys.executable, "-m", "esptool", "--chip", chip, "merge_bin", "-o", os.path.join(out, factory)] + opts
        for off, src in parts:
            cmd += ["0x%x" % off, src]
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
        manifest = {
            "name": "%s (%s)" % (title, needs.split(" (")[0]),
            "version": version,
            "new_install_prompt_erase": True,
            "builds": [{"chipFamily": family, "parts": [{"path": "firmware/" + factory, "offset": 0}]}],
        }
        with open(os.path.join(out, "manifest-%s-%s.json" % (edition, board)), "w") as f:
            json.dump(manifest, f, indent=2)
        lines += ["  %s: %s" % (board, needs), "    esptool.py --chip %s write_flash 0x0 %s" % (chip, factory)]
        print("%-8s %-10s %s" % (edition, board, factory))
        # Boards that update themselves (two app slots, an otadata partition)
        # get the app image alone and an update feed: Software Update's
        # "Check for official updates" reads it from the web installer's site.
        if any("ota_data_initial" in os.path.basename(src) for _, src in parts):
            app_src = next(src for off, src in parts if off == 0x10000)
            app = "tinydesk-%s-%s-%s-app.bin" % (edition, version, board)
            shutil.copy2(app_src, os.path.join(out, app))
            feed = {
                "name": title,
                "edition": edition,
                "board": board,
                "version": version,
                "date": time.strftime("%Y-%m-%d", time.gmtime()),
                "image": "firmware/" + app,
                "size": os.path.getsize(app_src),
                "sha256": sha256(app_src),
                "notes": "https://github.com/tinydesk-project/tinydesk/releases/tag/v%s" % version,
            }
            with open(os.path.join(out, "update-%s-%s.json" % (edition, board)), "w") as f:
                json.dump(feed, f, indent=2)
            print("%-8s %-10s %s (update feed)" % (edition, board, app))
    return lines


def package_host(out, name, binary, exe_name, licences, notes):
    """One PC program as <name>.tar.gz (Linux) or <name>.zip (Windows)."""
    if not os.path.exists(binary):
        sys.exit("no such program: %s" % binary)
    readme = "\n".join(notes) + "\n"
    if name.endswith("windows-x64"):
        path = os.path.join(out, name + ".zip")
        with zipfile.ZipFile(path, "w", zipfile.ZIP_DEFLATED) as z:
            z.write(binary, "%s/%s" % (name, exe_name))
            z.writestr("%s/README.txt" % name, readme.replace("\n", "\r\n"))
            for src, dst in licences:
                z.write(src, "%s/%s" % (name, dst))
    else:
        path = os.path.join(out, name + ".tar.gz")
        with tarfile.open(path, "w:gz") as t:
            info = t.gettarinfo(binary, "%s/%s" % (name, exe_name))
            info.mode = 0o755
            with open(binary, "rb") as f:
                t.addfile(info, f)
            data = readme.encode()
            ti = tarfile.TarInfo("%s/README.txt" % name)
            ti.size = len(data)
            ti.mode = 0o644
            ti.mtime = int(time.time())
            t.addfile(ti, io.BytesIO(data))
            for src, dst in licences:
                t.add(src, "%s/%s" % (name, dst))
    print("host     %s" % os.path.basename(path))


def install_into_site(release, site):
    """Copy a release (the flat files above) into site/install/: the
    manifests next to index.html, the images in firmware/, the PC programs
    in downloads/. Replaces what an earlier release put there."""
    dest = os.path.join(site, "install")
    if not os.path.exists(os.path.join(dest, "index.html")):
        sys.exit("%s is not a documentation site (no install/index.html)" % site)
    for name in os.listdir(dest):
        p = os.path.join(dest, name)
        if name in ("firmware", "downloads", "desktop", "shell", "esp32c6", "esp32") and os.path.isdir(p):
            shutil.rmtree(p)
        elif re.match(r"(manifest|update)-.*\.json$|SHA256SUMS\.txt$|README\.txt$", name):
            os.remove(p)
    os.makedirs(os.path.join(dest, "firmware"))
    os.makedirs(os.path.join(dest, "downloads"))
    for name in sorted(os.listdir(release)):
        src = os.path.join(release, name)
        if name.endswith(".bin"):
            shutil.copy2(src, os.path.join(dest, "firmware", name))
        elif name.endswith((".tar.gz", ".zip")):
            shutil.copy2(src, os.path.join(dest, "downloads", name))
        elif name.startswith(("manifest-", "update-")) or name in ("SHA256SUMS.txt", "README.txt"):
            shutil.copy2(src, os.path.join(dest, name))
    print("installer updated: %s" % dest)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--out", default=os.path.join(ROOT, "dist"))
    ap.add_argument("--build-dir", default="build", help="build folder inside each firmware project")
    ap.add_argument("--site", help="also fill the web installer of this documentation site (its install/ folder)")
    ap.add_argument("--allow-board-conf", action="store_true",
                    help="package images built with a board.conf (private use only)")
    ap.add_argument("--host-linux-desktop", help="the Linux desktop program (build/tinydesk)")
    ap.add_argument("--host-linux-shell", help="the Linux shell program (build-shell/tdsh_host)")
    ap.add_argument("--host-windows-desktop", help="the Windows desktop program (build/tinydesk.exe)")
    ap.add_argument("--host-windows-shell", help="the Windows shell program (build-shell/tdsh_host.exe)")
    args = ap.parse_args()

    found = collect(args)
    version = found["desktop"][0]
    out = os.path.join(args.out, "tinydesk-" + version)
    if os.path.exists(out):
        shutil.rmtree(out)
    os.makedirs(out)

    readme = ["TinyDesk %s" % version, ""]
    for edition, title, _, _ in EDITIONS:
        if edition not in found:
            continue
        ed_version, builds = found[edition]
        readme += ["%s %s (flash at 0x0 with esptool, or use the web installer):" % (title, ed_version)]
        readme += package_firmware(out, edition, title, ed_version, builds)
        readme += [""]
    readme += [
        "A factory image rewrites everything below the file system, NVS included,",
        "so users, Wi-Fi networks and the SSH host key start fresh. When switching a",
        "board between the editions, erase it first (esptool.py erase_flash, or",
        "\"Erase device\" in the web installer); on 4 MB ESP32 boards both editions",
        "share one flash layout and /fs keeps its files.",
        "To update a desktop board that is set up, use Software Update / `ota install`.",
        "",
        "After installing: open the board's serial port in a terminal (UTF-8; the",
        "desktop needs 921600 baud on the ESP32, the shell 115200; any speed on the",
        "ESP32-C6's USB port) and press a key. You start as root; the factory root",
        "password is TinyDesk. Change it locally with passwd before remote access.",
        "Telnet starts disabled; enable it explicitly in Network (root login only).",
        "SSH provides a separate encrypted shell, not the windowed desktop (the",
        "4 MB ESP32 Desktop build has no SSH server).",
        "",
        "Pins (RS-485, Ethernet, SD card) are set on the board with the `board`",
        "command; see the Board configuration page of the documentation.",
    ]

    licence = [(os.path.join(ROOT, "LICENSE"), "LICENSE")]
    shell_licence = [(os.path.join(ROOT, "third_party", "tdsh", "LICENSE"), "LICENSE-TinyDesk-Shell")]
    # The desktop programs include MicroPython (the py command).
    python_licence = [(os.path.join(ROOT, "third_party", "micropython", "LICENSE"), "LICENSE-MicroPython")]
    hosts = []
    if args.host_linux_desktop:
        hosts.append(("tinydesk-desktop-linux-x86_64", args.host_linux_desktop, "tinydesk", licence + shell_licence + python_licence, [
            "TinyDesk Desktop %s for Linux (x86_64)" % version, "",
            "Run it in a terminal window with mouse support (GNOME Terminal, Konsole,",
            "xterm, Windows Terminal over SSH...):", "", "  ./tinydesk", "",
            "Files live in ./tinydesk_fs, settings in ./tinydesk_settings.bin (the",
            "directory you start it in). Quit: Start > Exit, or Ctrl+C."]))
    if args.host_linux_shell:
        hosts.append(("tinydesk-shell-linux-x86_64", args.host_linux_shell, "tdsh", shell_licence, [
            "TinyDesk Shell %s for Linux (x86_64)" % found.get("shell", (version,))[0], "",
            "  ./tdsh", "",
            "Its files live in ~/.local/share/tdsh/rootfs (your real home is not used).",
            "Type 'help' for the commands, 'exit' to leave."]))
    if args.host_windows_desktop:
        hosts.append(("tinydesk-desktop-windows-x64", args.host_windows_desktop, "tinydesk.exe", licence + shell_licence + python_licence, [
            "TinyDesk Desktop %s for Windows (x64)" % version, "",
            "Start tinydesk.exe inside Windows Terminal (it needs a terminal with mouse",
            "and VT support; the old console window works without the mouse).", "",
            "Files live in .\\tinydesk_fs next to where you start it. Quit: Start > Exit."]))
    if args.host_windows_shell:
        hosts.append(("tinydesk-shell-windows-x64", args.host_windows_shell, "tdsh.exe", shell_licence, [
            "TinyDesk Shell %s for Windows (x64)" % found.get("shell", (version,))[0], "",
            "Start tdsh.exe in Windows Terminal (or any console window):", "", "  .\\tdsh.exe", "",
            "Its files live in %LOCALAPPDATA%\\tdsh\\rootfs (your real files are not used).",
            "Type 'help' for the commands, 'exit' to leave."]))
    for name, binary, exe, lic, notes in hosts:
        package_host(out, name, binary, exe, lic, notes)
    if hosts:
        readme += ["", "PC programs: " + ", ".join(h[0] for h in hosts) + " (README.txt inside each)."]

    with open(os.path.join(out, "README.txt"), "w") as f:
        f.write("\n".join(readme) + "\n")

    sums = []
    for d, _, files in os.walk(out):
        for name in sorted(files):
            if name == "SHA256SUMS.txt":
                continue
            p = os.path.join(d, name)
            sums.append("%s  %s" % (sha256(p), os.path.relpath(p, out).replace(os.sep, "/")))
    with open(os.path.join(out, "SHA256SUMS.txt"), "w") as f:
        f.write("\n".join(sorted(sums, key=lambda s: s.split("  ")[1])) + "\n")

    if args.site:
        install_into_site(out, args.site)
    print("release in %s" % out)


if __name__ == "__main__":
    main()
