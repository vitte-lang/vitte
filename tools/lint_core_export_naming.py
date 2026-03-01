#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path


SNAKE_OR_PASCAL = re.compile(r"^([a-z][a-z0-9_]*|[A-Z][A-Za-z0-9]*)$")


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    exports = repo / "tests/modules/contracts/core/core.exports"
    if not exports.exists():
        print(f"[core-export-naming][error] missing {exports}")
        return 1

    errs: list[str] = []
    for i, raw in enumerate(exports.read_text(encoding="utf-8").splitlines(), start=1):
        name = raw.strip()
        if not name:
            continue
        if not SNAKE_OR_PASCAL.match(name):
            errs.append(f"{exports}:{i}: invalid export name '{name}'")

    if errs:
        for e in errs:
            print(f"[core-export-naming][error] {e}")
        return 1
    print("[core-export-naming] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
