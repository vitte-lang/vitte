#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    mod = repo / "src/vitte/packages/std/mod.vit"
    internal_snap = repo / "tests/modules/contracts/std/std.exports.internal"
    errors: list[str] = []

    if not mod.exists():
        errors.append(f"missing file: {mod}")
    else:
        txt = mod.read_text(encoding="utf-8")
        if "internal/" in txt:
            errors.append(f"{mod}: public facade must not import internal modules")

    if not internal_snap.exists():
        errors.append(f"missing snapshot: {internal_snap}")
    else:
        if internal_snap.read_text(encoding="utf-8").strip():
            errors.append(f"{internal_snap}: must stay empty")

    if errors:
        for e in errors:
            print(f"[std-no-internal-exports][error] {e}")
        return 1
    print("[std-no-internal-exports] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
