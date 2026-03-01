#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path


USE_AS = re.compile(r"^\s*use\s+[^\s]+\s+as\s+([A-Za-z_][A-Za-z0-9_]*)\s*$")


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    root = repo / "tests/modules/contracts/core"
    errs: list[str] = []
    for p in root.rglob("*.vit"):
        for i, raw in enumerate(p.read_text(encoding="utf-8").splitlines(), start=1):
            m = USE_AS.match(raw.strip())
            if not m:
                continue
            alias = m.group(1)
            if not alias.endswith("_pkg"):
                errs.append(f"{p}:{i}: alias must end with _pkg (got: {alias})")
    if errs:
        for e in errs:
            print(f"[core-alias-pkg][error] {e}")
        return 1
    print("[core-alias-pkg] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
