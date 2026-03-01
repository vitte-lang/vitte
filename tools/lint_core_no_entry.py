#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    root = repo / "src/vitte/packages/core"
    errors: list[str] = []
    for p in root.rglob("*.vit"):
        txt = p.read_text(encoding="utf-8")
        for i, raw in enumerate(txt.splitlines(), start=1):
            line = raw.strip()
            if line.startswith("entry "):
                errors.append(f"{p}:{i}: entry forbidden in core packages")
    if errors:
        for e in errors:
            print(f"[core-no-entry][error] {e}")
        return 1
    print("[core-no-entry] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
