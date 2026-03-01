#!/usr/bin/env python3
from __future__ import annotations
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CFG = ROOT / "tools/facade_packages.json"
OUT = ROOT / "target/reports/symbol_index.json"


def parse_symbols(path: Path) -> list[str]:
    if not path.exists():
        return []
    out: list[str] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        if s.startswith("proc ") or s.startswith("form ") or s.startswith("pick "):
            out.append(s)
    return out


def main() -> int:
    cfg = json.loads(CFG.read_text(encoding="utf-8"))
    data = {"schema_version": "1.0", "packages": {}}
    for ent in cfg["packages"]:
        p = ent["name"]
        mod = ROOT / f"src/vitte/packages/{p}/mod.vit"
        data["packages"][p] = {
            "module": ent["module"],
            "diag_prefix": ent["diag_prefix"],
            "symbols": parse_symbols(mod),
        }
    OUT.parent.mkdir(parents=True, exist_ok=True)
    OUT.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[symbol-index] wrote {OUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
