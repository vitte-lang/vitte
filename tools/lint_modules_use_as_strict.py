#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    roots = [
        repo / "tests/modules/contracts/core",
        repo / "tests/modules/import_matrix",
    ]
    errs: list[str] = []

    for root in roots:
        if not root.exists():
            continue
        for p in root.rglob("*.vit"):
            for i, raw in enumerate(p.read_text(encoding="utf-8").splitlines(), start=1):
                line = raw.strip()
                if not line.startswith("use "):
                    continue
                if " as " not in line:
                    errs.append(f"{p}:{i}: strict import requires alias: 'use ... as ...'")
    if errs:
        for e in errs:
            print(f"[modules-use-as-strict][error] {e}")
        return 1
    print("[modules-use-as-strict] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
