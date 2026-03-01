#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    mod = repo / "src/vitte/packages/db/mod.vit"
    errs: list[str] = []
    txt = mod.read_text(encoding="utf-8") if mod.exists() else ""

    # Keep this rule pragmatic: enforce prepared path exists and unsafe concat guard exists.
    if "proc prepare(" not in txt:
        errs.append("missing prepare() API")
    if "is_safe_sql" not in txt:
        errs.append("missing SQL safety guard")
    if "execute_prepared" not in txt:
        errs.append("missing execute_prepared path")

    if errs:
        for e in errs:
            print(f"[db-sql-injection-lint][error] {e}")
        return 1
    print("[db-sql-injection-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
