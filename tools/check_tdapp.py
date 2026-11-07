"""check_tdapp.py - the frozen app library (tdapp) works in the firmware's MicroPython.

    python tools/check_tdapp.py PATH/TO/tdsim

Starts the simulated desktop (tools/tdsim.c) and runs a script that uses
tdapp the way apps do: an App with two pages (widgets hidden with
.visible, a page canvas moved off the window and back), a Field that
redraws, a simulated Modbus slave Link whose bound label follows put(),
require() on what the firmware has and on what it cannot have. The
script writes what it saw; this fails on anything unexpected. Run by
ctest.
"""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

PROBE = r'''import tdui, tdapp, json, sys
out = {"version": list(tdapp.VERSION)}
try:
    app = tdapp.App("Probe", 40, 12)
    main, other = app.page("Main"), app.page("Other")
    label = main.add(app.win.label(1, 1, "--"))
    cv = main.canvas(1, 3, 20, 6)
    note = other.add(app.win.label(1, 1, "other"))
    gauge = tdapp.Field(cv, 0, "gauge", {"x": 0, "y": 0, "w": 40, "h": 12, "value": 0, "maximum": 100})
    gauge.set(value=50)
    out["field"] = gauge.values["value"]
    link = app.link({"mode": "modbus_slave", "uart": 1, "tx": 17, "rx": 18, "unit": 9, "registerCount": 4},
                    simulated=True)
    app.bind(label, {"address": 2, "decimals": 1, "suffix": " bar", "max": 1000})
    app.show("Main")
    link.put("hr", 2, 42)
    out["bound"] = label.text
    out["get"] = link.get("hr", 2)
    out["crc"] = tdapp.crc16(b"\x01\x03\x00\x00\x00\x01")
    app.show("Other")
    out["other"] = [label.visible, note.visible, cv._tdapp_parked]
    app.show("Main")
    out["main"] = [label.visible, note.visible, cv._tdapp_parked]
    tdapp.require(cells=10, pixel_widgets=True)
    try:
        tdapp.require(cells=10 ** 9)
        out["require"] = "no error"
    except OSError as e:
        out["require"] = str(e)
    try:
        app.show("Nowhere")
    except ValueError:
        out["unknown"] = True
    link.close()
    app.win.close()
except Exception as e:
    import io
    s = io.StringIO()
    sys.print_exception(e, s)
    out["error"] = s.getvalue()
f = open("/root/tdapp_check.json", "w")
f.write(json.dumps(out))
f.close()
'''

WANT = {
    "version": None,   # filled from tdapp.py
    "field": 50,
    "bound": "42.0 bar",
    "get": [42],
    "crc": 0x0A84,
    "other": [False, True, True],
    "main": [True, False, False],
    "unknown": True,
}


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    tdsim = Path(sys.argv[1]).resolve()
    source = (Path(__file__).resolve().parents[1] / "ports/common/python/modules/tdapp.py").read_text(encoding="utf-8")
    import re
    m = re.search(r"^VERSION = \((\d+), (\d+)\)", source, re.M)
    WANT["version"] = [int(m.group(1)), int(m.group(2))]
    with tempfile.TemporaryDirectory(prefix="tdapp-check-") as work:
        root = Path(work) / "tinydesk_fs" / "root"
        root.mkdir(parents=True)
        (root / "probe.py").write_text(PROBE, encoding="utf-8")
        run = subprocess.run([str(tdsim), "wait=300", "key=Enter", "wait=1500", "type=py /root/probe.py\\r",
                              "wait=4000", "shot"], cwd=work, capture_output=True, text=True,
                             encoding="utf-8", errors="replace", timeout=120)
        result = root / "tdapp_check.json"
        if not result.exists():
            print(run.stdout[-3000:])
            sys.exit("check_tdapp: the probe did not run in the simulator (see the screen above)")
        got = json.loads(result.read_text(encoding="utf-8"))
    errors = []
    if "error" in got:
        errors.append("the probe failed:\n" + got["error"])
    for key, want in WANT.items():
        if got.get(key) != want:
            errors.append("%s: want %r, got %r" % (key, want, got.get(key)))
    if "canvas cells" not in str(got.get("require")):
        errors.append("require: want an OSError about canvas cells, got %r" % got.get("require"))
    for e in errors:
        print("check_tdapp:", e)
    if errors:
        print("check_tdapp: if tdapp.py changed, regenerate ports/common/python/gen with tools/micropython/gen.sh")
        return 1
    print("check_tdapp: tdapp %d.%d works: pages, page canvases, Field, Link.put with a binding, require"
          % tuple(got["version"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
