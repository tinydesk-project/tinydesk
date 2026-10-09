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

## Not released yet (branch experiment/pixelbuffer)

- Pixel canvas and Pixels: the plasma and the palette on the ESP32-C6
  (built-in USB), the ESP32-S3 (built-in USB) and the ESP32 with PSRAM
  (UART at 921600), read back with vtshot; tests/test_canvas.c (90
  checks, with dithering and Fine detail). Pictures per second, plasma: default window (42x30 pixels) C6
  30, S3 32, ESP32 9; full screen at 80x25 C6 18 (425 KB/s), S3 24
  (564 KB/s); at 160x50 S3 6 (633 KB/s), ESP32 1 (87 KB/s). On the
  ESP32 and the S3 every canvas is allocated in PSRAM (Log Viewer: canvas:
  ... in PSRAM). No restart on any board.
- Flicker: before, a terminal reading slower than the C6 sent (my reader
  at 20 and 8 KB/s) caused a screen clear after every dropped frame (10-13
  in 10 s). Now: none at full speed, 55 KB/s or with pauses of 200-400 ms
  every second (60 s each); one setup with a clear only when nothing got
  through for 3 s. Real PuTTY, 25 s recorded at 30 fps: no dark frame.
  tests/test_link.c (33 checks) fails without each rule.
- Frame pacing, real PuTTY on the C6 while Pixels animates, a key sent
  to PuTTY until the picture changed: median 2.4 s (worst 4.7 s) without
  pacing, 0.11 s (worst 0.2 s) with it; PuTTY shows about 10 pictures a
  second (86 KB/s).
- ESP32-S3 port on an ESP32-S3R8 (rev 0.2, 8 MB octal PSRAM, 16 MB
  flash) over its built-in USB: boots to the desktop (root@esp32s3),
  joins Wi-Fi, `ota feed` names update-desktop-esp32s3.json, About 199 KB
  internal + 7373 KB PSRAM free, every app opens one after another (13 of
  13, then the Task Manager), no restart. Not on hardware: SSH, RS-485,
  Ethernet, SD card. After moving link_usj.c to ports/esp_idf/app the C6,
  ESP32 and 4 MB ESP32 still build.
- Image Viewer: tests/test_image.c (102 checks: every BMP kind, binary
  and plain PPM/PGM, 16-bit samples, a plain file longer than the 4 KB
  window, the errors). tdsim on Windows: a 320x200 24-bit BMP on the
  desktop opens from a double-click, right way up, right colours. ESP32-S3:
  a 64x40 plain PPM typed in through the Terminal's `write` (53 lines,
  18,540 bytes, 24 s) and the 2x2 `echo` example from the guide open from
  the desktop icon; white and pure colours stay solid with dithering on
  (black too, in the tests). The C6, ESP32 and 4 MB ESP32 build with it; not opened on them.
- Fine pictures (four pixels per cell) on the ESP32-S3 at 120x40: a
  160x100 test card full screen shows 1-pixel stripes as stripes (Normal:
  grey), round circles in both details; the setting survives a restart;
  Pixels' plasma 29 pictures/s in the default window (Normal 32), 7/s
  full screen at 240x78 pixels (Normal 10/s at 120x78). Real PuTTY with
  its default font (Courier New): the picture is right, but the
  quadrant characters Courier New lacks show as small boxes along edges;
  Courier New, Consolas and Lucida Console have no quadrants, Cascadia
  Mono and Cascadia Code have all ten (checked in the font files). Not
  seen in PuTTY with Cascadia Mono.

## Not validated yet

- The Editor's save and reopen on every board.
- MQTT and Modbus with live devices.
- The Shell firmware on fresh boards.
- Installations from the web installer
  (https://tinydesk-project.github.io/install/) on every board it
  offers, and installations by other people.

The release process is in [RELEASE_CHECKLIST.md](RELEASE_CHECKLIST.md).
