# API stability

From 1.0, TinyDesk follows [semantic versioning](https://semver.org): an
app, script, `.tdapp` package or board configuration made for 1.x keeps
working on every later 1.x release.

| Version | May |
| --- | --- |
| patch (1.0.1) | fix bugs; no API change |
| minor (1.1.0) | add functions, types, struct members at the end, constants, widgets, events, Python names, board keys |
| major (2.0.0) | remove or change anything public |

`TD_VERSION` is the version as text, `TD_VERSION_MAJOR`, `_MINOR` and
`_PATCH` as numbers for `#if`. The embedded TinyDesk Shell has its own
version and the same rules (`third_party/tdsh/STABILITY.md`).

## What is public

| Part | Where | Checked by (ctest) |
| --- | --- | --- |
| C API | `include/tinydesk/*.h`, `apps/td_apps.h`, `proto/td_modbus.h`, `td_mqtt.h`, `td_sock.h`, `td_tls.h`, `ports/common/td_fs_stdio.h`, `td_host_hal.h`, `ports/common/python/td_py_desktop.h` | `api_stable`: the shell's `tools/check_api.py` against `api/tinydesk.api` |
| `tdui`, `tdapp` (Python) | `ports/common/python/tdui_contract.json`, `tdapp_contract.json` (their own api numbers and policy) | `tdui_contract`, `tdui_frozen`, `tdapp_works` |
| `.tdapp` packages | package format 1 (`packageFormats` in the tdui contract) | `tdapp_works` |
| `tinydesk` (Python) | `version`, `shell`, `log`, `mqtt_publish`, `modbus_read`, `modbus_write` | (reviews) |
| The shell's Python modules | `tdsh`, `network`, `socket`, ... | the shell's `tdsh_py_contract` |
| board.conf keys | the site's board configuration page | `board_keys`: the shell's `tools/check_board_keys.py` against `api/board_keys.txt` |
| Stored data | settings, desktop layout, accounts | (migrations keep reading older data) |

Not public, free to change in any release: `src/` (the core's shared
declarations are in `src/td_internal.h`: the renderer's terminal output,
the picture transport, the calls between the window manager and the
widgets), the apps' own sources, the generated tables in
`ports/common/python/gen`, and the look of the desktop.

## Changing the API

The checks fail on any difference, so every change is deliberate and shows
in the diff of the recorded file:

```sh
python third_party/tdsh/tools/check_api.py --record api/tinydesk.api <the headers above>
python third_party/tdsh/tools/check_board_keys.py --record api/board_keys.txt apps ports proto src
python tools/check_tdui_contract.py --record
```

Within 1.x, a recorded change may only add. Something to retire is marked
deprecated in its comment and in the changelog, keeps working, and goes in
the next major version. A struct that apps allocate gets new members only at
its end.
