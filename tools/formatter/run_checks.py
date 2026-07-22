#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
FORMATTER = ROOT / "tools" / "vitte_format.py"
MAKEFILE = ROOT / "Makefile"
CI = ROOT / ".github" / "workflows" / "ci.yml"
DOC = ROOT / "docs" / "formatter-migration-guide.md"

REQUIRED_FORMATTER_SYMBOLS = [
    "SUPPORTED_EDITIONS",
    "normalize_imports",
    "preserve_comments",
    "--edition",
    "--check",
]


def fail(message: str) -> int:
    print(f"[formatter][error] {message}", file=sys.stderr)
    return 1


def main() -> int:
    for path in [FORMATTER, MAKEFILE, CI, DOC]:
        if not path.exists():
            return fail(f"missing {path.relative_to(ROOT)}")

    formatter_src = FORMATTER.read_text(encoding="utf-8")
    for symbol in REQUIRED_FORMATTER_SYMBOLS:
        if symbol not in formatter_src:
            return fail(f"missing formatter symbol {symbol}")

    make_src = MAKEFILE.read_text(encoding="utf-8")
    ci_src = CI.read_text(encoding="utf-8")
    if "formatter-gate" not in make_src or "format-check" not in make_src:
        return fail("formatter gate is not wired in Makefile")
    if "make formatter-gate" not in ci_src:
        return fail("CI does not enforce make formatter-gate")

    print("[formatter] checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
