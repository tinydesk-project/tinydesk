# Developer-preview validation

These are the checks completed during preparation of the initial source
release in September 2026. They are a record of tested behavior, not a claim
that every app, peripheral or terminal has been validated.

## Local checks

- Windows GCC/CMake desktop build: all 7 CTest programs passed.
- Linux/WSL shell build: all 7 CTest programs passed, including memory stress,
  scripting and shared-console recovery policy.
- Host desktop simulator: embedded shell executed a command.
- Source archive: private-file exclusion and opt-in documentation tests passed.
- Documentation site: all pages checked with no internal link errors; all 7
  release-import validation tests passed.

## Physical boards

| Detected hardware | Desktop checks completed |
| --- | --- |
| ESP32-C6 rev 0.2, 8 MB flash | Flash and boot, shell, file write/read/remove, mouse window drag, recovery and first-use networking checks |
| ESP32-D0WD-V3 rev 3.0, 16 MB flash, working PSRAM | Flash and boot, shell, file write/read/remove, System Monitor, uptime beyond 10 minutes |
| ESP32-D0WDQ6 rev 1.0, 4 MB flash | Flash and boot, shell, file write/read/remove |

On the C6, physical recovery succeeded; non-root Telnet login was refused;
remote takeover during recovery was refused; recovery over root Telnet and
after disconnect was refused. A reboot restored physical recovery access.
With fresh NVS, Telnet defaulted off and Telnet/SSH/FTP setup was blocked
until the factory root password changed. Original device settings were restored.

A successful build alone is not counted as a hardware test.

## After the first release

- Windows and Linux programs: the desktop follows the terminal's size
  (Windows Terminal maximised and restored); PuTTY resizing checked on a
  board.
- MQTT over TLS in the PC programs, built against mbedTLS v3.6.7: connected to
  `test.mosquitto.org`; the release workflows fail a build without TLS.
- `tdsh.exe` (TinyDesk Shell for Windows) in Windows Terminal: line editing,
  history, Tab completion, `ifconfig`, `ping`, `exit`; its CTest smoke test
  pipes a script through it. It needs only Windows system DLLs.

## 0.1.2

- SD card on the ESP32-C6 (16 GB SDHC on the W6100's SPI bus): `hwtest sd`,
  `sd mount`, write and read back a file with a long name, the card in Files
  and the Editor, `hwtest sd` on the mounted card, `sd umount`, mounting at
  boot (`sd.automount = 1`), and Ethernet pings with the card mounted and
  after unmounting. Internal RAM free with the card, LAN and Wi-Fi in use:
  140 KB. On the ESP32 with PSRAM and the 4 MB ESP32 (no card wired) `sd`
  reports the pins as not configured.
- `ping -c` and ping output in the Terminal window on all three boards.

## 0.1.3

- Every app opened one after another through the Start menu, then the Task
  Manager, on all three boards (scripted, reading the screen): ESP32 with
  PSRAM 13 of 13 apps, 4 MB ESP32 12, ESP32-C6 11; the rest said *Too many
  windows*; the Task Manager listed the tasks with everything open; no
  restart. Before: the 4 MB ESP32 restarted when the Log Viewer opened after
  seven apps. The PC build (tdsim): all 13.
- nano in the Terminal window (80x25 boards, 160x50): Ctrl+C shows
  "line 2/2, column 7" and the help lines are visible.
- `passwd` names the factory password only while root has it (WROVER yes;
  4 MB ESP32 and C6, whose root password was changed, no); `ssh start`
  names it.
- Official updates: before 0.1.3 was published, `ota official` and *Check
  for official updates* on the ESP32-C6 reached the official site over
  HTTPS and reported that it had no feed yet (404); the setting persists
  (`ota notify`). After publishing, the ESP32 with PSRAM read the 0.1.3
  feed: `ota official` and the button say "TinyDesk 0.1.3, the newest
  release, is installed." and fill in the image's URL. Not tested yet:
  finding a newer release and installing it from the feed.
- Software Update on the 4 MB ESP32 shows the new explanation; internal RAM
  free on the C6 with LAN, Wi-Fi and SD: 133.7 KB.

## 0.1.4

- Long command lines: a host test drives the line editor into a VT
  emulator at 20 columns (wrapping, Backspace and the arrows across rows,
  the right margin, UTF-8, a coloured prompt, the width query with and
  without an answer, type-ahead); the simulated desktop (tdsim) passes 8
  scenarios; the Terminal window passes 7 on all three boards (the 80x25
  boards wrap at the window's 76 columns, so the width comes from the
  window). The standalone Shell on the 4 MB ESP32, over serial with a
  40-column terminal: the width is asked and used, editing across two
  wraps works, an unanswered query leaves nothing stray, and two commands
  sent in one burst both run. Not on hardware: SSH, the Windows console.
- Non-ASCII names: the Network list with multibyte, 4-byte and invalid
  SSIDs (test_columns), Files and the Task Manager with `café.txt` and
  `中文名.txt` on all three boards, the Network window after real scans,
  desktop labels (test_wm; the C6's one-line icons and the WROVER's
  two-line labels). In xterm.js double-width characters take two columns:
  the columns after a name stay in place, but the character after each
  wide one is hidden (TinyDesk draws one cell per character).
- `board save`: test_board; on the 4 MB ESP32 with a test firmware that
  had one built-in key: `board show` names it, `board save` writes it, and
  the normal firmware (no built-in settings, like an official image) then
  has it from `/etc/board.conf`. On the C6 (19 built-in settings)
  Software Update asks before *Install* (*Cancel*: nothing saved or
  installed) and `ota install` refuses; on the WROVER neither asks.
- Every app opened one after another, then the Task Manager: 13, 12 and
  11 apps as before, no restart.
- Free RAM (About, idle desktop after start-up): ESP32-C6 with the release
  configuration 167 KB with Wi-Fi, the same as 0.1.3 built and measured
  the same way (the 179 KB in the 0.1.3 notes was an older figure); ESP32
  with PSRAM 155 KB + 3263 KB PSRAM; 4 MB ESP32 96 KB, Wi-Fi not
  connected.
- The first update over the air from one official release to the next:
  the ESP32 with PSRAM on the official 0.1.3 image. After 0.1.4 was
  published, *Check for official updates* showed "TinyDesk 0.1.4,
  2026-10-05, 1.73 MB (newer)" and filled in the image; *Install*
  downloaded it (1.73 MB) without asking (no built-in settings) and said
  "Version 0.1.4 is installed. Restart to use it." After the restart it
  runs 0.1.4 (the release build) from ota_1, confirmed itself, keeps
  0.1.3 in ota_0 for a rollback, and has its files and its four board
  settings in `/etc/board.conf`; every app opens as before.

## 0.1.5

- Update information that moves: on the ESP32 with PSRAM, a test firmware
  whose built-in feed answered only `{"moved": "<the official esp32
  feed>"}`: `ota official` followed the move and reported the official
  0.1.4; `ota feed` then named the official feed, "(moved there by the
  update information)", also after a restart; `ota feed reset` went back
  to the built-in address. Not on hardware: a board key `update.url`
  taking precedence over a move.
- The release build reads the new site: on the ESP32 with PSRAM, `ota
  feed` names `https://tinydesk-project.github.io/install/…` "(built in)",
  and `ota official` reads it ("Installed 0.1.5 is newer than the newest
  release (0.1.4)").
- `network autowifi` and `network mode` as root on the ESP32 with PSRAM
  and the 4 MB ESP32: shown and changed as before (and set back). On the
  ESP32-C6 after the update (checked by the owner): the non-root user
  cannot change `network autowifi`.
- PuTTY 0.83 on the ESP32 with PSRAM over serial, the window resized
  five times between 640x480 and 1500x900 pixels: each time the desktop
  fills the window, the taskbar is on the last row and the icons are laid
  out again.
- Every app opened one after another, then the Task Manager: ESP32 with
  PSRAM 13 apps, 4 MB ESP32 12, no restart.
- Free RAM (About, idle desktop after a restart): ESP32 with PSRAM 155 KB
  + 3263 KB PSRAM, 4 MB ESP32 96 KB (Wi-Fi not connected), as in 0.1.4.
- Idle desktop after that, then the shell: `uptime` 11 and 15 minutes, no
  restart.
- The ESP32-C6 on the owner's own build (labelled 0.1.3, 19 settings built
  in): *Save and install* in Software Update saved its settings to
  `/etc/board.conf` and installed the official 0.1.5 (done by the owner).
- Release files: both published releases (TinyDesk Shell 7 files, TinyDesk
  19) match `SHA256SUMS.txt`; the update information on the new site and
  on the old one (`schikani.github.io/tinydesk-docs`) names 0.1.5 for the
  ESP32-C6 and the ESP32, and both images match their size and SHA-256.
- The bridge from the old site: the ESP32 with PSRAM on the official 0.1.4
  (which reads the old address). *Check for official updates* showed
  "TinyDesk 0.1.5, 2026-10-06, 1.73 MB (newer)" from the old site;
  *Install* downloaded it and said "Version 0.1.5 is installed. Restart to
  use it." After the restart it runs the release build from ota_0,
  confirmed itself, keeps 0.1.4 in ota_1, reads the new site (`ota feed`:
  built in), keeps its four board settings in `/etc/board.conf`, and all
  13 apps open.

## 0.2.0 (2026-10-31)

Release candidate: `tinydesk` f879e28 with `tinydesk-shell` de3a729 (both
version 0.2.0), built clean with ESP-IDF v5.3.1 on 2026-10-31 in a fresh
checkout, as the release workflow builds them (`-DTD_USE_EXAMPLE_BOARD=ON`
for the desktop, no settings built in), and packaged with
`tools/make_release.py` (seven factory images, three app images with
their update feeds, seven manifests). The later commits of 0.2.0 change
only the release notes and this file. Images and their SHA-256:

| Image | SHA-256 |
| --- | --- |
| `ports/esp32c6` `tinydesk.bin` | `b469e97a0bcef3e4f4b114e94c1bab69a6176163b3101aafb80e4761afbf7702` |
| `ports/esp32` `tinydesk.bin` | `9551e578607bb1e35e837104b9fec9e1dddce79c0e8a97ae2715d8a88b899047` |
| `ports/esp32-4mb` `tinydesk.bin` | `ebcb40dba1c569b4165c4a4002469184eb4f5923fa7df8523b2032f58f6daf83` |
| `ports/esp32s3` `tinydesk.bin` | `d164401e19d891b79ccad5fdf84b12a8987ac9476e5218dc03c20012225e4142` |
| Shell `tdsh_c6.bin` (built, not run on a board) | `0ec8e7e1bac5c11b04793289cd9801532cd837d75b1fdbd962f10bdcdf4c2120` |
| Shell `tdsh_esp32.bin` (built, not run on a board) | `3a464f33f0e6d9c25f15857885172da40d79c19b0b5beb412d263be40118f628` |
| Shell `tdsh_esp32s3.bin` (built; the same source ran on the S3 before the version bump) | `de191d3fea46a1db9f4bc0e40cb5e82e4b1800742da31afd5f9a0aa4b30f0ada` |

The four desktop images were written with their bootloader, partition
table and boot selection (no erase), so settings, users and files stayed;
no whole-flash backup was taken this time. Then, as root, scripted over
the serial link and read back from the screen, on the ESP32-C6 (USB,
80x25), the ESP32 with PSRAM (UART 921600, 160x50), the 4 MB ESP32 (UART
921600, 80x25) and the ESP32-S3 (UART 921600, 160x50), all with Wi-Fi
connected:

- About shows TinyDesk 0.2.0 and TinyDesk Shell 0.2.0 on all four.
- `whoami` in the Terminal; a window dragged by its title (Counter) and
  resized from its corner (Editor); a file opened in the Editor, changed,
  saved and opened again, and the file on flash afterwards.
- No reset in 10.5 minutes of use and idle (uptime 17.5, 16.8, 12.6 and
  16.3 minutes at the end).
- Free RAM in About: ESP32-C6 113 KB; ESP32 with PSRAM 155 KB + 3260 KB
  PSRAM; 4 MB ESP32 59 KB; ESP32-S3 189 KB + 7357 KB PSRAM.

Before the version bump, on builds of the same Python code (tinydesk
a24e415 and 0eab9ff): the Python board tests passed on the ESP32 with
PSRAM, the ESP32-S3 and the ESP32-C6 (features with native code 6/6;
on the S3 and the ESP32 with PSRAM also `machine` 4/4, `tdui` 14/14,
window sizes 8/8, the Python Editor 11/11, long pastes 5/5), and CI
passed on tinydesk#11.

Known issue: `ssh start` refuses on the ESP32-C6 (it asks for about
160 KB of free internal RAM); see the release notes.

Release: TinyDesk Shell 0.2.0 (tag v0.2.0 on de3a729) and TinyDesk 0.2.0
(tag v0.2.0 on 3953c42), both built by their release workflows (CI on
main and on the tags passed) and published as pre-releases. The ten
firmware images and the PC programs of TinyDesk Shell's release, and the
26 files of TinyDesk's, were downloaded and matched their
`SHA256SUMS.txt`; the site's `tools/fetch_release.py` accepted the release
(ESP32-S3 manifests included) and a local installer offered 0.2.0 for all
seven board/edition entries. After the site's Pages workflow: the live
installer's seven manifests say 0.2.0 with the right chip family, its ten
images match the live `SHA256SUMS.txt`, the update feeds of the ESP32-C6,
the ESP32 with PSRAM and the ESP32-S3 offer 0.2.0, the cover shows 0.2.0,
and the web terminal lists the ESP32-S3. Not done for this release: a
fresh install from the live site on each board.

## 0.1.6 (2026-10-18)

Release candidate: `tinydesk` 97286c5 with `tinydesk-shell` e85ad85 (both
version 0.1.6), built clean with ESP-IDF v5.3.1 on 2026-10-18. Images and
their SHA-256:

| Image | SHA-256 |
| --- | --- |
| `ports/esp32c6` `tinydesk.bin` | `33d735b8156f2a9359a1d8d0419da7752acafb6fc2c6cdf9f2a6519a6196f28d` |
| `ports/esp32` `tinydesk.bin` | `621f56b5f1de2813eb64c57c6f2ebc539190b61d7ff1695b45a6e878ed0dd232` |
| `ports/esp32-4mb` `tinydesk.bin` | `9e8eeb169a4730432779c58b151256d0beab3a29a8e53c6a53d471023703853b` |
| Shell `tdsh_c6.bin` (built, not run on a board) | `0a6f741139a2e90a0ad60ecb39b184613257f55adb53cebd315f0807c4209646` |
| Shell `tdsh_esp32.bin` (built, not run on a board) | `d77198c84c3fdee65ac5633c707ec0b15be0472f9eaad0ed55618332cb07a11b` |

All three desktop images are built as the release workflow builds them
(`-DTD_USE_EXAMPLE_BOARD=ON`, no settings built in); a first C6 build
from 97286c5 had the developer's own `board.conf` built in and was
replaced (the ESP32 images never had one). On the C6 the pins then come
from its `/etc/board.conf`, as on a board updated by an official image.

The first candidate, 26ae9c8, passed the same tests but About cut the
TinyDesk Shell address to `.../tinydesk-s` (a label holds 47 characters on
the ESP32 builds); 97286c5 draws both addresses whole. The Shell images
are the same source in both and were built from 26ae9c8.

Each board's whole flash was backed up first (kept off the repositories);
the three desktop images were written to the app partition only, so
settings, users and files stayed. Then, as root, scripted over the serial
link and read back from the screen, on the ESP32-C6 (USB, 80x25), the ESP32
with PSRAM (UART 921600, 160x50) and the 4 MB ESP32 (UART 921600, 80x25):

- About shows TinyDesk 0.1.6, TinyDesk Shell 0.1.6 and both repository
  addresses in full on all three;
  `ota status` on the ESP32-C6 and the ESP32 with PSRAM: 0.1.6 in ota_0
  (the 4 MB ESP32 has no OTA).
- A window dragged by its title (Counter) and resized from its corner
  (Editor).
- A file made with `write`, opened from its desktop icon in the Editor, a
  line added, saved with Ctrl+S, closed, opened again with both lines;
  `cat` showed both lines from flash.
- Shell commands: `whoami`, `uptime`, `heap`, `cat`, `rm`.
- Memory, idle desktop: ESP32-C6 `heap` 132,568 bytes free (largest
  block 102,400); ESP32 with PSRAM 155 KB + 3263 KB PSRAM
  (158,471, largest 110,592); 4 MB ESP32 96 KB (98,900, largest 86,016;
  lowest during the tests 66,824).
- 10.5 minutes of use (About and the Terminal opened about once a minute)
  and idle: no reset; `uptime` at the end 12:58 (ESP32-C6), 17:14 (ESP32
  with PSRAM), 12:47 (4 MB ESP32).
- Cold boot (unplugged and plugged in again by the owner): the ESP32-C6
  and the 4 MB ESP32 started on their own into 0.1.6 (`uptime` matching
  the replug). The ESP32 with PSRAM (an ESP32-WROVER-IE board) stayed
  silent after 3 of 4 power-ons: nothing at 115200 from about 3 s after
  power-up, no answer at 921600, and not in download mode (esptool
  cannot connect without a reset); a pulse on EN started it normally.
  Two of those were with 0.1.6, one with the official 0.1.4 (`ota
  rollback`, replugged, then back to 0.1.6), and the fourth power-on
  started 0.1.6 on its own (`uptime` matching the replug): this board's
  power-up, not the firmware.
- The Shell firmware (`tdsh_esp32.bin` above) on the 4 MB ESP32, written
  to the app partition (same layout as the desktop): boots to root on the
  local console at 115200, `version` TinyDesk Shell 0.1.6, `heap` 210,700
  bytes free, a file written and read back, a file opened, saved and
  opened again in nano, a `.tdsh` script with arithmetic, `peek -l` (8
  regions), `peek` of the eFuse, `poke` to RAM refused; 10.5 minutes of
  commands and idle without a reset (`uptime` 12:21). Then the desktop
  image above was written back and About showed TinyDesk 0.1.6.
- The release packaged locally from these builds (`make_release.py
  --build-dir`, into a copy of the site): 17 files, all matching
  `SHA256SUMS.txt`; the five manifests name 0.1.6 and images that exist
  and match; the two update feeds name 0.1.6 and the very app images
  tested above (size and SHA-256). `fetch_release.py`'s check accepts the
  release and refuses a copy with one byte of an image changed; the
  site's tests (11) pass. The installer page from that copy: all five
  board and edition choices show "version 0.1.6" with Install; quick
  board changes end on the last one chosen; Linux programs not on the
  site point to the GitHub releases, Windows programs download from the
  site; a missing and an empty image each show "not on this site right
  now" without Install, the other boards still install.

- The TinyDesk Shell draft release `v0.1.6` (tag on e85ad85, built by
  the release workflow): its 7 files match its `SHA256SUMS.txt`; both
  manifests name 0.1.6 and the right chip; the PC archives hold `tdsh` /
  `tdsh.exe`, README and LICENSE. The app part of its ESP32 image,
  written to the 4 MB ESP32's app partition, runs: `version` 0.1.6,
  `idfinfo` ESP-IDF v5.3.1, a file written and read back, `peek -l`;
  then the desktop image was written back. Its ESP32-C6 image was not
  run on a board.

- The TinyDesk draft release `v0.1.6` (tag on b084b2d, which has the
  tested firmware source of 97286c5; built by the release workflow after
  CI passed): its 19 files match its `SHA256SUMS.txt` and nothing is
  unlisted; the five manifests name 0.1.6, the right chip and offset 0;
  both update feeds name 0.1.6 and their app images (size, SHA-256
  `1da7cedd49446229…` ESP32-C6, `e715ad1368181dd0…` ESP32); the four PC
  archives hold the program, README and licences; the release text is
  "New in 0.1.6". Its images on the boards (the ESP32-C6 and ESP32 app
  images, the app part of the 4 MB factory image, each written to the
  app partition): About shows TinyDesk 0.1.6, ESP-IDF v5.3.1, TinyDesk
  Shell 0.1.6 and both addresses in full; window drag and resize, the
  Editor open, save and reopen, shell commands, and 3 minutes of use and
  idle without a reset on all three.

- Published (both releases), then the site pushed and its GitHub Pages
  workflow run: the live cover and installer page say 0.1.6; the five
  manifests and their images, both update feeds with their app images,
  and the four PC downloads on https://tinydesk-project.github.io/install/
  match the release's `SHA256SUMS.txt`. `ota official` on the ESP32 with
  PSRAM and the ESP32-C6 reads the live update information: "TinyDesk
  0.1.6, the newest release, is installed."

Not done for this release yet (they need the owner's passwords):
factory NVS, recovery, and fresh installs from the live site. After tagging: the draft release's images against
`SHA256SUMS.txt`.

The `peek` and `poke` results below were on a build before the version
bump (shell efb4e1f code).

- `peek` and `poke` in the TinyDesk firmware, as root over the serial
  Terminal: `peek -l` lists 9 regions on the ESP32 with PSRAM (with
  `psram`), 8 on the 4 MB ESP32 (no `psram`) and 7 on the ESP32-C6. Reads
  in every region; the eFuse MAC words match the Wi-Fi MAC on the ESP32
  with PSRAM (90:38:0C:AD:45:60) and the ESP32-C6 (AC:EB:E6:D0:E7:C8).
  Refused: 8-bit reads of the ESP32's instruction-bus ROM, `poke` to RAM
  and to the read-only GPIO registers, 8- and 16-bit `poke`. A dump from
  `gpio_out` into `gpio` (overlapping regions) works. `poke` set and
  cleared the output latch of a pin whose output stays disabled (GPIO33
  on the ESP32, GPIO1 on the ESP32-C6), read back with `peek`; no pin was
  driven. The Shell firmware builds with the same table; not run on a
  board.
- A script that runs out of memory while it loads prints
  `uscript: <file>:<line>: not enough memory to load the script`
  (tinydesk-shell tests/test_script_oom.c, which fails without the fix).

## Not validated yet

- The Editor's save and reopen on every board.
- MQTT and Modbus with live devices.
- The Shell firmware on fresh boards.
- Installations from the web installer
  (https://tinydesk-project.github.io/install/) on every board it
  offers, and installations by other people.

The release process is in [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md).
