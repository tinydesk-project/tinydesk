"""check_tdui_frozen.py - the tdui frozen into the firmware is tdui.py.

    python tools/check_tdui_frozen.py PATH/TO/tdsim

tdui reaches the boards as bytecode frozen into
third_party/micropython/frozen/frozen_content.c, made by
tools/micropython/gen.sh and committed. Change tdui.py without
regenerating it and every board still runs the old tdui. This starts the
simulated desktop (tools/tdsim.c), runs a script in its MicroPython that
looks up every public name tdui.py defines (tools/tdui_surface.py) in the
frozen module, and fails on any it lacks, any public module name it has
that tdui.py does not, or another tdui.API / CAPABILITIES. Run by ctest.
"""
import json
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import tdui_surface  # noqa: E402

PROBE = '''import tdui, json
want = %s
missing = []
for name in want["module"]:
    if not hasattr(tdui, name):
        missing.append(name)
for cls, members in want["classes"].items():
    c = getattr(tdui, cls, None)
    for name in members:
        if c is None or not hasattr(c, name):
            missing.append(cls + "." + name)
extra = [n for n in dir(tdui) if not n.startswith("_") and n not in want["module"] and n not in want["imports"]]
f = open("/root/tdui_frozen.json", "w")
f.write(json.dumps({"missing": missing, "extra": extra, "api": list(tdui.API), "caps": list(tdui.CAPABILITIES)}))
f.close()
'''


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    tdsim = Path(sys.argv[1]).resolve()
    code = tdui_surface.surface()
    source = tdui_surface.SOURCE.read_text(encoding="utf-8")
    # Names the module imports (const, sys, ...) are in dir() but not part of tdui.
    import ast
    imports = []
    for node in ast.parse(source).body:
        if isinstance(node, (ast.Import, ast.ImportFrom)):
            imports += [(a.asname or a.name).split(".")[0] for a in node.names]
    want = {"module": sorted(code["module"]), "imports": imports,
            "classes": {c: sorted(m) for c, m in code["classes"].items()}}
    with tempfile.TemporaryDirectory(prefix="tdui-frozen-") as work:
        root = Path(work) / "tinydesk_fs" / "root"
        root.mkdir(parents=True)
        (root / "probe.py").write_text(PROBE % json.dumps(want), encoding="utf-8")
        run = subprocess.run([str(tdsim), "wait=300", "key=Enter", "wait=1500", "type=py /root/probe.py\\r",
                              "wait=4000", "shot"], cwd=work, capture_output=True, text=True,
                             encoding="utf-8", errors="replace", timeout=120)
        result = root / "tdui_frozen.json"
        if not result.exists():
            print(run.stdout[-3000:])
            sys.exit("check_tdui_frozen: the probe did not run in the simulator (see the screen above)")
        got = json.loads(result.read_text(encoding="utf-8"))
    contract = json.loads((tdui_surface.ROOT / "ports/common/python/tdui_contract.json").read_text(encoding="utf-8"))
    errors = ["frozen tdui lacks %s" % n for n in got["missing"]]
    errors += ["frozen tdui has %s, which tdui.py does not" % n for n in got["extra"]]
    if got["api"] != contract["api"]:
        errors.append("frozen tdui.API is %s, the contract %s" % (got["api"], contract["api"]))
    for e in errors:
        print("check_tdui_frozen:", e)
    if errors:
        print("check_tdui_frozen: regenerate third_party/micropython with tools/micropython/gen.sh")
        return 1
    print("check_tdui_frozen: the frozen tdui has all %d module names and %d class members of tdui.py; "
          "api %d.%d, capabilities %s" % (len(want["module"]), sum(len(m) for m in want["classes"].values()),
                                          got["api"][0], got["api"][1], ", ".join(got["caps"])))
    return 0


if __name__ == "__main__":
    sys.exit(main())
