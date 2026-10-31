# TinyDesk developer preview

A mouse-operated desktop running on an ESP32, displayed in your terminal.
Drag and resize windows, browse files, edit and save text, and use the
embedded shell. MQTT and Modbus apps connect the desktop to real projects.

This release is a developer preview: expect rough edges, and please report
what breaks.

## New in 0.2.0

* **ESP32-S3:** the Desktop edition for an ESP32-S3 with 16 MB flash and
  8 MB Octal PSRAM (screens up to 256×96, SSH, over-the-air updates), and
  the Shell edition for any ESP32-S3 with 4 MB flash or more, both on the
  board's USB-UART port. The port came from @SanjayG-xls (#9).
* **Python scripts with MicroPython:** `py script.py`, `py -c ...` or the
  prompt, in the Terminal and over SSH, with TinyDesk's files and paths.
  The `tinydesk` module runs shell commands, publishes MQTT messages and
  reads or writes Modbus registers; `asyncio`, `hashlib` and precompiled
  `.mpy` files are included, and root can use native and viper code.
* **Pins and buses from Python (`machine`, root only):** `Pin` with
  interrupts, `ADC`, `PWM`, `I2C`, `SPI`, `UART`, their software versions
  and `neopixel`. The pins TinyDesk uses are refused, and everything a
  script opened is released when it ends.
* **Windows from Python (`tdui`):** a script opens desktop windows with
  buttons, text boxes, lists, canvases, dialogs and menus, and handles
  their events in Python: a small app without writing C.
* **The Editor for Python:** colours, line numbers, indents and
  completion for `.py` files; F5 runs the file with the output under the
  text, F8 goes to the line of an error; find and replace, go to a line.
  The Editor's buffer now follows the file's size, which leaves more RAM
  on small boards (#6). `.py` files run from the desktop and Files.
* **Files of the port on new Desktops:** a port can add its own files,
  such as a `Credits.txt`, next to `Welcome.txt` (#5, an idea of
  @aquasock).
* **Fixes:** pasted text no longer picks up the terminal's size reply,
  and long pastes arrive whole on the ESP32-C6 (its USB receive buffer
  grew from 1 KB to 8 KB).

Also since 0.1.0: nano with UTF-8 text, `peek` and `poke`, and script
memory limits for small boards (0.1.6); the new home at github.com/tinydesk-project, update
information that can move, and only root changes the network policy
(0.1.5); Software Update offers to save built-in pins before
an official update, non-ASCII names line up, long command lines wrap
(0.1.4); official updates in Software Update and fixes for boards
without PSRAM (0.1.3); the SD card at `/sd` and in Files (0.1.2);
`ping -c`, MQTT over TLS in the PC programs, and `tdsh.exe` for Windows.

## Install

* **ESP boards, from the browser:** <https://tinydesk-project.github.io/install/>
  (Chrome or Edge on a computer). Pick the edition and your exact board.
* **Open the board in the browser:** <https://tinydesk-project.github.io/console/>
  (a real terminal with mouse support; the installer's own Logs & Console
  cannot show the desktop).
* **Without the browser:** each `*-factory.bin` below is flashed at offset 0
  with esptool; see the installer page for the commands.
* **PC programs:** `tinydesk-desktop-windows-x64.zip` and
  `tinydesk-shell-windows-x64.zip` (Windows 10 or later, run them in Windows
  Terminal), `tinydesk-desktop-linux-x86_64.tar.gz` and
  `tinydesk-shell-linux-x86_64.tar.gz` (x86_64, built on Ubuntu 22.04).

Check the files against `SHA256SUMS.txt`. The firmware is not signed.
Factory flashing can erase user data: back up an existing board first.

## Before connecting

Use a UTF-8 terminal with ANSI/VT cursor control and xterm mouse reporting,
or the web terminal above. ESP32-C6 uses its built-in USB port (any speed);
the classic ESP32 and the ESP32-S3 (its USB-UART port) use 921600 baud in
the Desktop edition and 115200 in the Shell edition. On the local console, run `passwd` as root (initial password:
`TinyDesk`). Remote access is blocked until that password changes. Telnet
starts disabled; enable it explicitly in Network only on a trusted network.
It is unencrypted and root-only because it takes over the existing desktop.
SSH provides a separate encrypted shell, not the windowed desktop (the 4 MB
ESP32 Desktop build has no SSH server).

Physical password recovery is unavailable after a Telnet takeover until the
board reboots. Remote takeover is refused while physical recovery is active.

## Choose the exact board

| Desktop target | Required hardware | Screen | Limits |
| --- | --- | --- | --- |
| ESP32-C6 | 8 MB flash, built-in USB Serial/JTAG | up to 80×25 | |
| ESP32 with PSRAM | 16 MB flash plus PSRAM (e.g. WROVER-IE N16R8) | up to 256×96 | |
| ESP32 4 MB | 4 MB flash, no PSRAM needed (e.g. WROOM-32) | up to 80×25 | no SSH server, no over-the-air updates |
| ESP32-S3 | 16 MB flash plus 8 MB Octal PSRAM (e.g. ESP32-S3-DevKitC-1 N16R8) | up to 256×96 | |
| Windows, Linux | x86_64 PC | up to 400×150 | no Wi-Fi, OTA or users (the PC's network is used) |

Measured on our boards with this firmware (About → Free RAM, idle desktop
after start-up, Wi-Fi connected unless noted):

| Board | Firmware size (app slot) | Free RAM |
| --- | --- | --- |
| ESP32-C6 | 1.83 MB (2.5 MB slot, 27 % free) | about 167 KB |
| ESP32 with PSRAM | 1.73 MB (3 MB slot, 42 % free) | about 155 KB internal + 3.2 MB PSRAM |
| ESP32 4 MB | 1.66 MB (2.5 MB slot, 34 % free) | about 96 KB, Wi-Fi not connected |
| ESP32-S3 | 1.67 MB (3 MB slot, 44 % free) | about 203 KB internal + 7.2 MB PSRAM, Wi-Fi not connected |

## What is in this preview

- Desktop: overlapping windows (move, resize, minimise, maximise, full
  screen), taskbar, start menu, desktop icons, drag and drop, copy and paste
  with the PC; Dark theme by default; the screen follows the terminal's size.
- Apps: Terminal (TinyDesk Shell), Files, Editor, Network, MQTT (TLS),
  Modbus TCP/RTU with a TCP server, System Monitor, Task Manager, Log Viewer,
  Settings, Software Update (OTA with rollback), About.
- Shell scripts (`.tdsh`): variables, `if`/`while`/`for`, functions, pipes
  and redirection; right-click → Run on the desktop.
- Python scripts (MicroPython): `py`, pins and buses with `machine`,
  desktop windows with `tdui`, and the Editor's Python mode.
- Two editions: TinyDesk Desktop, and TinyDesk Shell on its own.
- Security: physical recovery uses a transport trust policy, not just shell
  task identity; Telnet is opt-in; Telnet, SSH and FTP reject the factory
  password.
- PC programs: the Windows desktop follows Windows Terminal when it is
  resized or maximised, and its shell has `ifconfig`, `ping`, `date`, `cal`
  and `tz`.
- Board pins come from a configuration file (`board` command), not from
  the code.
- Tests gate release builds; every release starts as a draft pre-release.

## Licences

TinyDesk and TinyDesk Shell are MIT. The firmware images also contain
third-party code under its own licence: wolfSSH and wolfSSL (GPLv3) for the
SSH/SFTP server, so the firmware as a whole is distributed under the GPLv3;
and MicroPython (MIT, Copyright (c) 2013-2026 Damien P. George) for the
`py` command, whose SHA-256 code from
`lib/crypto-algorithms` is public domain. The source archive has every
licence text: `LICENSE`, `third_party/tdsh/THIRD_PARTY_LICENSES.md` and
`third_party/micropython/LICENSE`. The PC programs include theirs.
