#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    mod = repo / "src/vitte/packages/db/mod.vit"
    txt = mod.read_text(encoding="utf-8") if mod.exists() else ""

    required = ["MigrationState", "migration_lock", "migration_apply", "migration_compat_check"]
    missing = [x for x in required if x not in txt]
    if missing:
        for m in missing:
            print(f"[db-migration-compat][error] missing symbol: {m}")
        return 1
    print("[db-migration-compat] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
