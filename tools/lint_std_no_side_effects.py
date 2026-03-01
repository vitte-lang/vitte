#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    root = repo / "src/vitte/packages/std"
    errs: list[str] = []
    for p in root.rglob("*.vit"):
        for i, raw in enumerate(p.read_text(encoding="utf-8").splitlines(), start=1):
            line = raw.strip()
            if line.startswith("#"):
                continue
            if line.startswith("entry "):
                errs.append(f"{p}:{i}: entry is forbidden (no side effects at import time)")
    if errs:
        for e in errs:
            print(f"[std-no-side-effects][error] {e}")
        return 1
    print("[std-no-side-effects] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
