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
        "ast.exports",
        "ast.exports.public",
        "ast.exports.internal",
        "ast.exports.sha256",
    ]
    for n in required:
        p = d / n
        if not p.exists():
            errs.append(f"missing {p}")
    if errs:
        return errs

    exp = (d / "ast.exports").read_text(encoding="utf-8")
    pub = (d / "ast.exports.public").read_text(encoding="utf-8")
    if exp != pub:
        errs.append(f"{d}: public snapshot diverges from exports")

    digest = sha(d / "ast.exports")
    recorded = (d / "ast.exports.sha256").read_text(encoding="utf-8").strip()
    if digest != recorded:
        errs.append(f"{d}: sha mismatch expected={recorded} got={digest}")

    for required_symbol in (
        "stable_api_surface",
        "api_version",
        "stable_validate_tree",
        "stable_walk_preorder",
        "stable_metrics",
    ):
        if required_symbol not in exp:
            errs.append(f"{d}: {required_symbol} missing from exports")

    return errs


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    paths = [
        "src/vitte/packages/contracts_snapshots/ast",
        "tests/modules/contracts/ast",
    ]
    errors: list[str] = []
    for rel in paths:
        errors.extend(check(repo, rel))

    if errors:
        for e in errors:
            print(f"[ast-export-lint][error] {e}")
        return 1

    print("[ast-export-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
