#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


def read_set(path: Path) -> set[str]:
    if not path.exists():
        return set()
    return {line.strip() for line in path.read_text(encoding="utf-8").splitlines() if line.strip()}


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    exports = repo / "tests/modules/contracts/core/core.exports"
    facade = repo / "tests/modules/contracts/core/core.facade.api"
    public = repo / "tests/modules/contracts/core/core.exports.public"

    current = read_set(exports)
    expected = read_set(facade)
    public_set = read_set(public)

    errs: list[str] = []
    removed = sorted(expected - current)
    if removed:
        errs.append(f"breaking removals detected: {', '.join(removed)}")

    if current != public_set:
        errs.append("core.exports and core.exports.public diverge")

    if errs:
        for e in errs:
            print(f"[core-compat-contracts][error] {e}")
        return 1

    print("[core-compat-contracts] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
