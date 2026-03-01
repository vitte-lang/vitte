#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

USE_RE = re.compile(r"^\s*use\s+([^\s]+)\s+as\s+([A-Za-z_][A-Za-z0-9_]*)")
SENSITIVE = {"vitte/ffi", "vitte/process", "vitte/net"}


def read_allowlist(path: Path) -> set[str]:
    if not path.exists():
        return set()
    out: set[str] = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if line and not line.startswith("#"):
            out.add(line)
    return out


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    root = repo / "src/vitte/packages/db"
    allow = read_allowlist(repo / "tests/modules/contracts/db/db_sensitive_imports.allow")
    errs: list[str] = []

    for p in root.rglob("*.vit"):
        rel = p.relative_to(repo).as_posix()
        for i, line in enumerate(p.read_text(encoding="utf-8").splitlines(), start=1):
            m = USE_RE.match(line)
            if not m:
                continue
            target = m.group(1)
            if target in SENSITIVE and f"{rel}:{target}" not in allow:
                errs.append(f"{p}:{i}: sensitive import denied: {target}")

    if errs:
        for e in errs:
            print(f"[db-sensitive-imports][error] {e}")
        return 1
    print("[db-sensitive-imports] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
