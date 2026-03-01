#!/usr/bin/env python3
from __future__ import annotations

import hashlib
from pathlib import Path


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def check(root: Path, rel: str) -> list[str]:
    errs: list[str] = []
    d = root / rel
    required = [
        "array.exports",
        "array.exports.public",
        "array.exports.internal",
        "array.exports.sha256",
    ]
    for n in required:
        p = d / n
        if not p.exists():
            errs.append(f"missing {p}")
    if errs:
        return errs

    exp = (d / "array.exports").read_text(encoding="utf-8")
    pub = (d / "array.exports.public").read_text(encoding="utf-8")
    if exp != pub:
        errs.append(f"{d}: public snapshot diverges from exports")

    digest = sha(d / "array.exports")
    recorded = (d / "array.exports.sha256").read_text(encoding="utf-8").strip()
    if digest != recorded:
        errs.append(f"{d}: sha mismatch expected={recorded} got={digest}")

    if "stable_api_surface" not in exp:
        errs.append(f"{d}: stable_api_surface missing from exports")
    if "api_version" not in exp:
        errs.append(f"{d}: api_version missing from exports")

    return errs


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    paths = [
        "src/vitte/packages/contracts_snapshots/array",
        "tests/modules/contracts/array",
    ]
    errors: list[str] = []
    for rel in paths:
        errors.extend(check(repo, rel))

    if errors:
        for e in errors:
            print(f"[array-export-lint][error] {e}")
        return 1

    print("[array-export-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
