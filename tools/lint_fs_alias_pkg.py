#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

USE_RE = re.compile(r"^\s*use\s+([^\s]+)\s+as\s+([A-Za-z_][A-Za-z0-9_]*)")


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    roots = [repo / "src/vitte/packages/fs", repo / "tests/fs", repo / "tests/modules/contracts/fs"]
    errs: list[str] = []
    for root in roots:
        if not root.exists():
            continue
        for p in root.rglob("*.vit"):
            for i, line in enumerate(p.read_text(encoding="utf-8").splitlines(), start=1):
                m = USE_RE.match(line)
                if not m:
                    continue
                alias = m.group(2)
                if not alias.endswith("_pkg"):
                    errs.append(f"{p}:{i}: alias must end with _pkg (got {alias})")

    if errs:
        for e in errs:
            print(f"[fs-alias-pkg-lint][error] {e}")
        return 1
    print("[fs-alias-pkg-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
