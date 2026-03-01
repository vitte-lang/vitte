#!/usr/bin/env python3
from __future__ import annotations
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "tools/facade_packages.json"


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding="utf-8"))
    errs: list[str] = []
    for ent in cfg["packages"]:
        p = ent["name"]
        mod = ROOT / f"src/vitte/packages/{p}/mod.vit"
        if not mod.exists():
            errs.append(f"{p}: missing mod.vit")
            continue
        txt = mod.read_text(encoding="utf-8")
        if "proc doctor_status()" not in txt:
            errs.append(f"{p}: missing doctor_status")
        if "proc quickfix_preview(" not in txt:
            errs.append(f"{p}: missing quickfix_preview")
        if "proc quickfix_apply(" not in txt:
            errs.append(f"{p}: missing quickfix_apply")
    if errs:
        for e in errs:
            print(f"[api-nonregression][error] {e}")
        return 1
    print("[api-nonregression] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
