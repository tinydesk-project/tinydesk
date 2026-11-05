"""tdui_surface.py - the public Python surface of tdui.py, read from its source.

What an app may call: the module's public functions and constants, and each
public class's public methods and properties (also those it inherits from
tdui's private base classes), with every function's parameters. Used by
check_tdui_contract.py (the surface must only grow) and
check_tdui_frozen.py (the firmware's frozen tdui must have it).
"""
import ast
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "ports/common/python/modules/tdui.py"


def _params(fn):
    """[name, ...] and how many of them are required (positional, no default)."""
    a = fn.args
    names = [p.arg for p in a.posonlyargs + a.args]
    if names and names[0] in ("self", "cls"):
        names = names[1:]
    required = len(names) - len(a.defaults)
    names += [p.arg for p in a.kwonlyargs]
    required += sum(1 for d in a.kw_defaults if d is None)
    if a.vararg:
        names.append("*" + a.vararg.arg)
    if a.kwarg:
        names.append("**" + a.kwarg.arg)
    return {"params": names, "required": required}


def _members(cls, classes):
    out = {}
    for base in cls.bases:
        if isinstance(base, ast.Name) and base.id in classes:
            out.update(_members(classes[base.id], classes))
    for node in cls.body:
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)) and not node.name.startswith("_"):
            deco = [d.id if isinstance(d, ast.Name) else getattr(d, "attr", "") for d in node.decorator_list]
            if "property" in deco or "setter" in deco:
                out[node.name] = {"property": True}
            else:
                out[node.name] = _params(node)
        elif isinstance(node, ast.Assign):
            for t in node.targets:
                if isinstance(t, ast.Name) and not t.id.startswith("_"):
                    out[t.id] = {"constant": True}
    return out


def surface(source=SOURCE):
    tree = ast.parse(Path(source).read_text(encoding="utf-8"))
    classes = {n.name: n for n in tree.body if isinstance(n, ast.ClassDef)}
    module, result = {}, {}
    for node in tree.body:
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)) and not node.name.startswith("_"):
            module[node.name] = _params(node)
        elif isinstance(node, ast.Assign):
            for t in node.targets:
                if isinstance(t, ast.Name) and not t.id.startswith("_"):
                    module[t.id] = {"constant": True}
        elif isinstance(node, ast.ClassDef) and not node.name.startswith("_"):
            module[node.name] = {"class": True}
            result[node.name] = _members(node, classes)
    return {"module": module, "classes": result}


def compatible(old, new, where, errors):
    """new keeps everything old had: names, parameter names in order, no new required ones."""
    for name, o in old.items():
        n = new.get(name)
        if n is None:
            errors.append("%s.%s was removed or renamed" % (where, name))
            continue
        if "params" in o:
            if "params" not in n:
                errors.append("%s.%s is no longer a function" % (where, name))
            elif n["params"][:len(o["params"])] != o["params"]:
                errors.append("%s.%s parameters changed: %s -> %s" % (where, name, o["params"], n["params"]))
            elif n["required"] > o["required"]:
                errors.append("%s.%s needs more arguments (%d -> %d): give new ones defaults"
                              % (where, name, o["required"], n["required"]))
    for name in new:
        if name not in old:
            errors.append("%s.%s is new: record it with tools/check_tdui_contract.py --record "
                          "(and raise the api minor number)" % (where, name))
