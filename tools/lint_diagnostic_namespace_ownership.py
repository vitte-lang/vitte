#!/usr/bin/env python3
from __future__ import annotations
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "tools/facade_packages.json"
CODE_RE = re.compile(r"VITTE-([A-Z]+)[0-9]{4}")


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding="utf-8"))
    errs: list[str] = []
    for ent in cfg["packages"]:
        pkg = ent["name"]
        expected = ent["diag_prefix"].split("-")[1]
        root = ROOT / f"src/vitte/packages/{pkg}"
        if not root.exists():
            continue
        for f in root.rglob("*.vit"):
            txt = f.read_text(encoding="utf-8")
            for m in CODE_RE.finditer(txt):
                got = m.group(1)
                if got != expected:
                    errs.append(
                        f"{pkg}:{f.relative_to(ROOT)}: mixed diag namespace {m.group(0)} expected {ent['diag_prefix']}****"
                    )
    if errs:
        for e in errs:
            print(f"[diag-namespace][error] {e}")
        return 1
    print("[diag-namespace] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
