#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path


def parse_rows(path: Path) -> set[tuple[str, str, str]]:
    rows: set[tuple[str, str, str]] = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        parts = [p.strip() for p in line.split("|")]
        if len(parts) != 4:
            raise ValueError(f"invalid row in {path}: {line}")
        _version, profile, module, symbol = parts
        rows.add((profile, module, symbol))
    return rows


def main() -> int:
    p = argparse.ArgumentParser(description="Check stdlib ABI compatibility (v1 subset of v2)")
    p.add_argument("--old", default="docs/stdlib_abi_surface_v1.txt")
    p.add_argument("--new", default="docs/stdlib_abi_surface_v2.txt")
    args = p.parse_args()

    old_path = Path(args.old)
    new_path = Path(args.new)
    if not old_path.exists() or not new_path.exists():
        print("[abi-compat][error] missing ABI surface file(s)", file=sys.stderr)
        return 1

    try:
        old_rows = parse_rows(old_path)
        new_rows = parse_rows(new_path)
    except ValueError as exc:
        print(f"[abi-compat][error] {exc}", file=sys.stderr)
        return 1

    missing = sorted(old_rows - new_rows)
    if missing:
        print("[abi-compat][error] incompatible ABI change: removed rows:", file=sys.stderr)
        for row in missing:
            print(f"  - {row[0]}|{row[1]}|{row[2]}", file=sys.stderr)
        return 1

    added = sorted(new_rows - old_rows)
    print(f"[abi-compat] OK (kept={len(old_rows)}, added={len(added)})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
