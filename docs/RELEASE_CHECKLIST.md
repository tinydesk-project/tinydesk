# Release checklist

Source lives in `tinydesk-project/tinydesk` with `tinydesk-project/tinydesk-shell` pinned as a
submodule. The documentation and web installer live in
`tinydesk-project/tinydesk-project.github.io` and are published with GitHub Pages at
https://tinydesk-project.github.io/.

## Tests before a release

- Host CMake/CTest on Linux and Windows, and all POSIX shell tests.
- Builds: Desktop for ESP32-C6 8 MB, ESP32 16 MB with PSRAM, ESP32 4 MB and
  ESP32-S3 16 MB with Octal PSRAM; the Shell edition for ESP32-C6, ESP32 and
  ESP32-S3; the Windows and Linux programs
  of both editions (`tinydesk.exe`, `tdsh.exe`, and their Linux builds).
- The source revision and SHA-256 of every tested image are recorded. An
  older dist folder does not count for changed source.
- On each board: cold boot, drag/resize, open/save/reopen a file, a shell
  command, memory figures, and ten minutes of use and idle with no reset.
- With factory NVS: Telnet is off, and Telnet, SSH and FTP do not accept the
  factory root password. After a local password change, a service can be
  enabled explicitly.
- Recovery: physical recovery works; SSH, a non-root Telnet login, root
  Telnet and a disconnected session are refused; after a reboot local
  recovery works again. A takeover during either password prompt is refused.
- Hardware tests start from a backup of the board's flash, and test accounts
  and changed settings are restored afterwards. The backups hold the
  board's passwords and keys and are not published.
- Installer: for every board variant, the binary named by its manifest
  exists and matches `SHA256SUMS.txt`; missing and corrupt files, quick board
  changes and unavailable PC downloads are handled.
- A fresh install from the live HTTPS site, including the first serial
  connection and the password change, on each board the installer offers.

## Release process

1. Push shell changes first. For a shell release, set its `VERSION` file and
   tag `tinydesk-shell` with `v<VERSION>`: its release workflow builds the
   Shell firmware, `tdsh.exe` and the Linux program into a **draft
   prerelease**. Review and publish it.
2. Record the tested submodule revision in the desktop repository and push.
   CI must succeed from a clean recursive checkout.
3. Set the version: `include/tinydesk/td.h` (`TD_VERSION`), `PROJECT_VER`
   in `ports/esp32c6`, `ports/esp32`, `ports/esp32-4mb` and `ports/esp32s3`, the README line,
   `RELEASE_NOTES.md`, and the docs site's changelog and `api/core.md`. The
   site's cover and installer take theirs from the release when the Pages
   workflow runs. Then tag the tested revision. The release workflow calls
   CI, tests its release host binaries (MQTT over TLS must be built in),
   builds firmware, and creates a **draft prerelease**.
4. Review the draft: version, all seven board/edition images, the app images
   and update feeds of the boards that update over the air, manifests, the
   four PC downloads, checksums, source and licence material, and the
   release text.
5. Publish the draft, then run the docs site's *GitHub Pages* workflow
   (Actions, Run workflow): `tools/fetch_release.py` copies the newest
   published release into the installer after checking `SHA256SUMS.txt`, and
   `tools/check_links.py` checks the links.
   **Once, after publishing 0.1.5:** boards running 0.1.3 or 0.1.4 look for
   updates at the old address, `https://schikani.github.io/tinydesk-docs/install/`
   (repository `schikani/tinydesk-docs`). Run that repository's *GitHub
   Pages* workflow once, so it serves 0.1.5 (update information and images),
   then archive the repository on GitHub: its site stays online, read-only.
   Those boards update to 0.1.5, which reads the new address, and from there
   to every later release. Never delete or rename `schikani/tinydesk-docs`,
   or create `tinydesk` or `tinydesk-shell` under `schikani` (that breaks
   GitHub's redirects to `tinydesk-project`).
6. Install from the live site on each board the installer offers. ESP Web
   Tools requires HTTPS and resolves firmware paths relative to the
   manifest:
   [official integration documentation](https://esphome.github.io/esp-web-tools/).

Hardware tests are recorded in [VALIDATION.md](VALIDATION.md); a build alone
is not a passed hardware test.
