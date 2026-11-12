# TinyDesk developer preview

A mouse-operated desktop running on an ESP32, displayed in your terminal.
Drag and resize windows, browse files, edit and save text, and use the
embedded shell. MQTT and Modbus apps connect the desktop to real projects.

This release is a developer preview: expect rough edges, and please report
what breaks.

## New in 0.5.1

**Fix:** on the 4 MB ESP32, Python apps and `py` did not start on the
desktop while the network was off ("not enough RAM"). They run again; while
such a script runs, Wi-Fi and Ethernet cannot be started, and `py` says so.
Boards with more RAM are not affected.

## New in 0.5.0

* **No more cut-off text.** Message boxes word-wrap to fit the screen
  (lines were cut at 63 bytes, and after 8 lines), input-box prompts wrap,
  and a label keeps all of its text: one too narrow for it ends in a
  highlighted **…** you click to read everything. List items, window
  titles, taskbar buttons, Start menu entries, icon names and the apps'
  status lines end in **…** where they are cut; the taskbar shows **»**
  when it has more windows than buttons.
* **Python apps on the ESP32-C6 with Wi-Fi on:** Python's heap now grows as
  a script needs it and always leaves room for Wi-Fi and the desktop; the
  C6's Wi-Fi driver runs from flash, which gives its RAM to apps.
* **Store:** an app's details show its author, licence and source link.
  Three new apps in the Store: MQTT Dashboard, Modbus Monitor and Network
  Tools.

**Fixes**

* Installing an app from the Store restarted the ESP32-C6 and ESP32-S3
  (tinydesk #19): the desktop task ran out of stack. It has 8 KB on the C6
  and 12 KB on the other boards now.
* "Install on the SD card" failed with "Cannot write the file".
* The Store on the ESP32-C6 said "The catalog could not be read".
* Offline, the Store called installed apps "Needs a newer TinyDesk".

## New in 0.4.0

* **Bluetooth keyboards and mice** (`bt.enable = 1`): the **Bluetooth** app
  pairs a keyboard, a mouse and a combo device, which then reconnect by
  themselves; the `bt` command does the same from the shell. LE on the
  boards with PSRAM (ESP32-S3, CrowPanel, Nano ESP32), and Bluetooth
  Classic too on the ESP32 WROVER, so keyboards such as Logitech's K380
  work there. Keyboards that ask for a code show it in the app. The
  interface and the app come from @aquasock's TinyTang port (#7).
* **Wi-Fi access point (STA+AP):** the board's own network
  (`TinyDesk-XXXX`, 192.168.4.1) for Telnet, SSH and SFTP where there is no
  other one: off, always on, or on while there is no network. Its name and
  password (random per board at first) are set in the Network app or with
  `wifiap`.
* **SD card as a drive:** a card mounts by itself when inserted and
  unmounts when pulled out; the desktop shows an **SD card** icon (open,
  eject, drop files on it) and Files opens it. The Store can install apps on
  it, and `tdui` keeps decoded pictures there (`/sd/TinyDesk/cache`), so apps
  start faster.
* **Store:** **Uninstall** (was Remove), also offline, without a catalog.
* **Sharper pictures without Sixel** (PuTTY, the Windows console) and in the
  web terminal at any display scaling; right-click in the web terminal opens
  TinyDesk's menus, not the browser's.

**Fixes**

* FTP answered "550 Invalid path" to everything since 0.3.0 (Windows
  Explorer, FileZilla): its session lacked the account check the shell
  added in 0.3.0.
* The sign-in screen now follows the terminal's size: started small or
  maximised later, its fields and buttons stay with its text.
* The ESP32-C6 is built without Bluetooth, which took about 35 KB of its RAM
  even while off; it has 119 KB free again and `tdui` windows open.

**Good to know**

* The ESP32 WROVER image now needs an ESP32 revision 3 chip (every
  WROVER-E/IE, sold since 2019); older chips refuse it at start.
* `sd.automount` is now on wherever the SD card pins are set.
* No Bluetooth on the ESP32-C6 and the 4 MB ESP32 (no PSRAM for it).

## New in 0.3.1

* **`ssl` in the PC shell program:** 0.3.0's `tdsh_host` (Linux) and
  `tdsh_host.exe` (Windows) were built without TLS, so `py` had no `ssl`
  there (no HTTPS, MQTT over TLS or `upip`). They have it now, and a
  release build stops if it would leave it out. The firmware and the
  desktop programs had it already; nothing else changed.

## New in 0.3.0

The public API is frozen in this release as the candidate for 1.0
(`STABILITY.md`): please report anything that should change before 1.0
promises it.

* **Two new boards:** the **Elecrow CrowPanel 7.0** (the desktop on its
  800×480 touch screen, kiosk apps) and the **Arduino Nano ESP32** (the
  desktop on its USB-C port; a community port, contributed and tested by
  @aquasock, #15). Both are in the web installer and in every release.
* **Network in Python:** `socket`, `ssl` (TLS 1.2, server certificates
  checked against the firmware's CA bundle), `requests`, `umqtt.simple`
  and `umqtt.robust`, a `websocket` client, `aiohttp`, and a read-only
  `network` module for the connection's state (#13).
* **Python packages and virtual environments:** `upip install`, `uninstall`,
  `list` and `freeze` from micropython.org's index, GitHub or GitLab;
  `venv`, `activate`, `deactivate` (the prompt shows the environment).
* **MicroPython lives in TinyDesk Shell now**, so the Shell edition has
  `py` too, and TinyDesk adds its own modules on top.
* **Search** (Alt+S or Win+S) finds apps, files and folders as you type;
  a letter in Files, lists, the desktop and the start menu jumps to the
  next item starting with it.
* **New flash layouts, meant to stay for 1.x:** the 16 MB boards have two
  4 MB app slots and a 7.8 MB `/fs`, the ESP32-C6 two 3 MB slots and
  1.8 MB, the 4 MB ESP32 a 3 MB app (the same layout as TinyDesk Shell's,
  so it switches editions and keeps its files); the boards with room get a
  64 KB NVS for accounts and Wi-Fi networks and a crash-dump slot.
* **App Builder apps:** `tdapp`, the library App Builder's apps share, is
  built into the firmware; App Builder can export small packages that use it.
* **Remote Desktop:** hover works, and no stray characters at the login.
* **API stability:** the public C headers, Python modules and board keys
  are recorded and checked by the tests; `TD_VERSION_MAJOR`, `_MINOR`,
  `_PATCH`.

**Updating from 0.2.0:** Software Update installs 0.3.0 and keeps
everything, but keeps the old flash layout. For the larger app slots and
`/fs`, install from the web installer with *Erase device*: users, Wi-Fi
networks and files then start fresh, so copy off what you need first. The
4 MB ESP32 and the CrowPanel have no Software Update: install again
(without erasing, `/fs` keeps its files).

Earlier: the ESP32-S3, MicroPython with `machine` and `tdui` windows, and the Editor for Python (0.2.0); nano with UTF-8 text, `peek` and `poke`, and script
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
or the web terminal above. The ESP32-C6 and the Nano ESP32 use their
built-in USB port (any speed); the classic ESP32, the ESP32-S3 (its
USB-UART port) and the CrowPanel 7.0 (its USB-C port; the desktop is on
the touch screen) use 921600 baud in the Desktop edition and 115200 in the
Shell edition. On the local console, run `passwd` as root (initial password:
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
| ESP32-C6 | 8 MB flash, built-in USB Serial/JTAG | up to 80×25 | no SSH server in this release (see Known issues) |
| ESP32 with PSRAM | 16 MB flash plus PSRAM (e.g. WROVER-IE N16R8) | up to 256×96 | |
| ESP32 4 MB | 4 MB flash, no PSRAM needed (e.g. WROOM-32) | up to 80×25 | no SSH server, no over-the-air updates |
| ESP32-S3 | 16 MB flash plus 8 MB Octal PSRAM (e.g. ESP32-S3-DevKitC-1 N16R8) | up to 256×96 | USB keyboard and mouse: one device, no hubs |
| CrowPanel 7.0 | Elecrow DIS08070H (ESP32-S3, 4 MB flash, 8 MB PSRAM) | its 800×480 touch screen, 100×30 cells | no over-the-air updates; no Shell edition of its own (the ESP32-S3 one runs on UART0) |
| Arduino Nano ESP32 | the board itself (ESP32-S3, 16 MB flash, 8 MB PSRAM) | up to 256×96 | community port; replaces the Arduino partition table; no USB keyboard, no Shell edition |
| Windows, Linux | x86_64 PC | up to 400×150 | no Wi-Fi, OTA or users (the PC's network is used) |

Measured on our boards with this firmware (About → Free RAM, idle desktop
just after start-up, before Wi-Fi):

| Board | Firmware size (app slot) | Free RAM |
| --- | --- | --- |
| ESP32-C6 | 2.42 MB (3 MB slot, 19 % free) | about 157 KB |
| ESP32 with PSRAM | 2.26 MB (4 MB slot, 43 % free) | about 218 KB internal + 3.2 MB PSRAM |
| ESP32 4 MB | 2.18 MB (3 MB slot, 27 % free) | about 70 KB |
| ESP32-S3 | 2.25 MB (4 MB slot, 44 % free) | about 258 KB internal + 7.2 MB PSRAM |
| CrowPanel 7.0 | 2.36 MB (2.75 MB slot, 14 % free) | |
| Arduino Nano ESP32 | 2.19 MB (4 MB slot, 45 % free) | not measured here |

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

## Known issues

- **Python on the 4 MB ESP32:** without PSRAM, a script gets about 21 KB
  of RAM before Wi-Fi and about 16 KB with Wi-Fi connected: enough for
  small scripts and plain sockets, not for TLS (`https://`, `wss://`).
- **SSH on the ESP32-C6:** `ssh start` refuses with "insufficient internal
  RAM" on the ESP32-C6 (it has done so since 0.1.0). A fix is ready and
  tested and comes in a later release. The other boards with SSH are not
  affected.

## Licences

TinyDesk and TinyDesk Shell are MIT. The firmware images also contain
third-party code under its own licence: wolfSSH and wolfSSL (GPLv3) for the
SSH/SFTP server, so the firmware as a whole is distributed under the GPLv3;
MicroPython (MIT, Copyright (c) 2013-2026 Damien P. George) for the
`py` command, whose SHA-256 code from `lib/crypto-algorithms` is public
domain; and from micropython-lib (MIT) the frozen `requests`, `ssl`,
`umqtt`, `aiohttp` and `mip`. The source archive has every licence text:
`LICENSE`, `third_party/tdsh/THIRD_PARTY_LICENSES.md`,
`third_party/tdsh/third_party/micropython/LICENSE` and
`third_party/tdsh/licenses/micropython-lib-LICENSE.txt`. The PC programs
include theirs.
