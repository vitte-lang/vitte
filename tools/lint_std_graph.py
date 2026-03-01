#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

USE_RE = re.compile(r"^\s*use\s+([^\s]+)\s+as\s+([A-Za-z_][A-Za-z0-9_]*)")
DENY_PREFIXES = ("vitte/app", "vitte/apps", "vitte/features", "vitte/ide")


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    root = repo / "src/vitte/packages/std"
    errs: list[str] = []

    for p in root.rglob("*.vit"):
        text = p.read_text(encoding="utf-8")
        for i, line in enumerate(text.splitlines(), start=1):
            m = USE_RE.match(line)
            if not m:
                continue
            target = m.group(1)
            if target.startswith(DENY_PREFIXES):
                errs.append(f"{p}:{i}: std must not depend on app/features: {target}")

    if errs:
        for e in errs:
            print(f"[std-graph-lint][error] {e}")
        return 1
    print("[std-graph-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
