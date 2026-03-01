#!/usr/bin/env python3
from __future__ import annotations
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "tools/facade_packages.json"


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding="utf-8"))
    errs: list[str] = []
    warns: list[str] = []
    for ent in cfg["packages"]:
        pkg = ent["name"]
        thin = ent.get("facade_thin", {})
        mode = str(thin.get("mode", "warn")).lower()
        warn_loops = int(thin.get("warn_loops", 15))
        max_loops = int(thin.get("max_loops", 40))
        max_procs = int(thin.get("max_procs", 120))
        mod = ROOT / f"src/vitte/packages/{pkg}/mod.vit"
        if not mod.exists():
            errs.append(f"{pkg}: missing mod.vit")
            continue
        txt = mod.read_text(encoding="utf-8")
        procs = re.findall(r"^\s*proc\s+", txt, flags=re.M)
        loops = re.findall(r"\bloop\s*\{", txt)
        if len(procs) > max_procs:
            msg = f"{pkg}: too many proc in facade ({len(procs)} > {max_procs})"
            if mode == "error":
                errs.append(msg)
            else:
                warns.append(msg)
        if len(loops) > max_loops:
            msg = f"{pkg}: too many loops in facade ({len(loops)} > {max_loops})"
            if mode == "error":
                errs.append(msg)
            else:
                warns.append(msg)
        elif len(loops) > warn_loops:
            warns.append(f"{pkg}: loop density high ({len(loops)} > {warn_loops})")
    for w in warns:
        print(f"[facade-thin][warn] {w}")
    if errs:
        for e in errs:
            print(f"[facade-thin][error] {e}")
        return 1
    print("[facade-thin] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
