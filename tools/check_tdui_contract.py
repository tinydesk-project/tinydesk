"""check_tdui_contract.py - the tdui contract file against the code.

    python tools/check_tdui_contract.py            check
    python tools/check_tdui_contract.py --record   also write tdui.py's new
                                                   public names into the contract

ports/common/python/tdui_contract.json is what app makers (TinyDesk App
Builder) build against. This checks that its numbers are the ones of
apps/td_apps.h and ports/common/python/modules/tdui.py: requests, widgets,
events, limits, the api number (also apps/store.c's) and the capabilities.
Its "python" part is tdui's public Python surface (tools/tdui_surface.py):
the code may only add to it. Removing or renaming a function, method,
property or constant, changing a function's parameters or giving it a new
required one fails, because apps already made call them. New names fail
too until --record writes them into the contract (raise the api minor
number with them). Run by ctest.
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import tdui_surface  # noqa: E402


def c_enum(text, first):
    """The TD_PYUI_ names of the C enum that starts with `first`, numbered."""
    start = text.index(first)
    body = re.sub(r"/\*.*?\*/", "", text[start:text.index("}", start)], flags=re.S)
    names, n = {}, 0
    for m in re.finditer(r"TD_PYUI_([A-Z0-9_]+)(?:\s*=\s*(\d+))?\s*,?", body):
        if m.group(2):
            n = int(m.group(2))
        names[m.group(1)] = n
        n += 1
    return names


# tdui.py's shorter names for some of them
ALIAS = {"LCLEAR": "LIST_CLEAR", "LADD": "LIST_ADD", "W_CHECK": "W_CHECKBOX", "W_TEXT": "W_TEXTBOX"}


def subset(what, contract, code, errors):
    """Every name the code has is in the contract with the same number."""
    for k, v in code.items():
        if contract.get(k) != v:
            errors.append("%s: %s is %s in the code, %s in the contract" % (what, k, v, contract.get(k)))


def py_consts(text, prefix):
    return {m.group(1): int(m.group(2))
            for m in re.finditer(r"^%s([A-Z0-9_]+) = const\((\d+)\)" % re.escape(prefix), text, re.M)}


def write_python(path, code):
    """Replace (or add) the contract's "python" part, one line per name, and
    leave the rest of the file as it is."""
    text = path.read_text(encoding="utf-8")
    cut = text.find(',\n  "python":')
    text = (text[:cut] if cut >= 0 else text.rstrip().rstrip("}").rstrip()) + ",\n"

    def block(members, indent):
        rows = ['%s"%s": %s' % (indent, k, json.dumps(v)) for k, v in sorted(members.items())]
        return "{\n" + ",\n".join(rows) + "\n" + indent[:-2] + "}"

    classes = ",\n".join('      "%s": %s' % (c, block(m, "        ")) for c, m in sorted(code["classes"].items()))
    text += ('  "python": {\n    "module": %s,\n    "classes": {\n%s\n    }\n  }\n}\n'
             % (block(code["module"], "      "), classes))
    path.write_text(text, encoding="utf-8", newline="\n")


def main():
    contract = json.loads((ROOT / "ports/common/python/tdui_contract.json").read_text(encoding="utf-8"))
    header = (ROOT / "apps/td_apps.h").read_text(encoding="utf-8")
    tdui = (ROOT / "ports/common/python/modules/tdui.py").read_text(encoding="utf-8")
    errors = []

    def same(what, want, have):
        if want != have:
            errors.append("%s: contract %s, code %s" % (what, want, have))

    requests = c_enum(header, "TD_PYUI_OPEN = 1")
    widgets = c_enum(header, "TD_PYUI_W_LABEL = 1")
    events = c_enum(header, "TD_PYUI_EV_CLICK = 1")
    same("C requests", contract["requests"], requests)
    same("C widgets", contract["widgets"], {k[2:]: v for k, v in widgets.items()})
    same("C events", contract["events"], {k[3:]: v for k, v in events.items()})

    py = {ALIAS.get(k, k): v for k, v in py_consts(tdui, "_").items()}
    subset("tdui.py requests", contract["requests"],
           {k: v for k, v in py.items() if not k.startswith(("W_", "EV_"))}, errors)
    same("tdui.py widgets", contract["widgets"], {k[2:]: v for k, v in py.items() if k.startswith("W_")})
    same("tdui.py events", contract["events"], {k[3:]: v for k, v in py.items() if k.startswith("EV_")})

    limits = contract["limits"]
    pub = py_consts(tdui, "")
    for name in ("MAX_WINDOWS", "MAX_WIDGETS", "MAX_PAGES"):
        same("tdui.py " + name, limits[name], pub.get(name))
    for name in ("MAX_CELLS", "MAX_PIXELS"):
        m = re.search(r"#define TD_PYUI_%s (\d+)" % name, header)
        same("td_apps.h " + name, limits[name]["default"], int(m.group(1)) if m else None)

    m = re.search(r"^API = \((\d+), (\d+)\)", tdui, re.M)
    same("tdui.API", contract["api"], [int(m.group(1)), int(m.group(2))] if m else None)
    # The Store (apps/store.c) decides by the same number what a board runs.
    store = (ROOT / "apps/store.c").read_text(encoding="utf-8")
    major = re.search(r"#define TDUI_MAJOR\s+(\d+)", store)
    minor = re.search(r"#define TDUI_MINOR\s+(\d+)", store)
    same("store.c TDUI_MAJOR/MINOR", contract["api"],
         [int(major.group(1)), int(minor.group(1))] if major and minor else None)
    caps = set(re.findall(r'"([a-z_]+)"', re.search(r"^CAPABILITIES = (.*)$", tdui, re.M).group(1)))
    same("tdui.CAPABILITIES", set(contract["capabilities"]), caps)

    # The Python surface only grows.
    code = tdui_surface.surface()
    path = ROOT / "ports/common/python/tdui_contract.json"
    if "--record" in sys.argv:
        recorded = contract.setdefault("python", {"module": {}, "classes": {}})
        problems = []
        tdui_surface.compatible(recorded["module"], code["module"], "tdui", problems)
        for cls, members in recorded["classes"].items():
            tdui_surface.compatible(members, code["classes"].get(cls, {}), "tdui." + cls, problems)
        breaking = [p for p in problems if " is new:" not in p]
        if breaking:
            errors += breaking   # never record a break
        else:
            write_python(path, code)
            print("check_tdui_contract: recorded the Python surface")
    elif "python" not in contract:
        errors.append("the contract has no python surface: run with --record")
    else:
        recorded = contract["python"]
        tdui_surface.compatible(recorded["module"], code["module"], "tdui", errors)
        for cls, members in recorded["classes"].items():
            tdui_surface.compatible(members, code["classes"].get(cls, {}), "tdui." + cls, errors)

    for e in errors:
        print("check_tdui_contract:", e)
    if not errors:
        print("check_tdui_contract: %d requests, %d widgets, %d events, api %d.%d agree"
              % (len(requests), len(widgets), len(events), *contract["api"]))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
