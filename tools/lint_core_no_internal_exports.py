#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    core_mod = repo / "src/vitte/packages/core/mod.vit"
    internal_snap = repo / "tests/modules/contracts/core/core.exports.internal"
    errors: list[str] = []

    if not core_mod.exists():
        errors.append(f"missing file: {core_mod}")
    else:
        txt = core_mod.read_text(encoding="utf-8")
        if "internal/" in txt:
            errors.append(f"{core_mod}: public facade must not import internal modules")

    if not internal_snap.exists():
        errors.append(f"missing snapshot: {internal_snap}")
    else:
        if internal_snap.read_text(encoding="utf-8").strip():
            errors.append(f"{internal_snap}: must stay empty (no internal exports)")

    if errors:
        for e in errors:
            print(f"[core-no-internal-exports][error] {e}")
        return 1
    print("[core-no-internal-exports] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
