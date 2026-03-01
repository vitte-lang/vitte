#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    internal_snap = repo / "tests/modules/contracts/log/log.exports.internal"
    errs: list[str] = []

    if not internal_snap.exists():
        errs.append(f"missing snapshot: {internal_snap}")
    elif internal_snap.read_text(encoding="utf-8").strip():
        errs.append(f"{internal_snap}: must stay empty")

    if errs:
        for e in errs:
            print(f"[log-no-internal-exports][error] {e}")
        return 1
    print("[log-no-internal-exports] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
