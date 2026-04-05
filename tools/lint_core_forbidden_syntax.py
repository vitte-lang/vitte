#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path


FORBIDDEN_KEYWORDS = ("macro", "trait", "unsafe", "asm", "select")
FORBIDDEN_PATTERNS = (
    re.compile(r"\.end\b"),
)


def check_ebnf(path: Path) -> list[str]:
    txt = path.read_text(encoding="utf-8")
    errors: list[str] = []
    for kw in FORBIDDEN_KEYWORDS:
        if f"\"{kw}\"" in txt:
            errors.append(f"{path}: forbidden core keyword in EBNF: {kw}")
    for pat in FORBIDDEN_PATTERNS:
        if pat.search(txt):
            errors.append(f"{path}: forbidden core pattern found: {pat.pattern}")
    return errors


def check_core_valid_tests(root: Path) -> list[str]:
    errors: list[str] = []
    files = sorted((root / "tests/grammar/valid").glob("core-*.vit"))
    for p in files:
        txt = p.read_text(encoding="utf-8")
        for kw in FORBIDDEN_KEYWORDS:
            if re.search(rf"\b{re.escape(kw)}\b", txt):
                errors.append(f"{p}: forbidden keyword in core valid fixture: {kw}")
        for pat in FORBIDDEN_PATTERNS:
            if pat.search(txt):
                errors.append(f"{p}: forbidden pattern in core valid fixture: {pat.pattern}")
    return errors


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    errors: list[str] = []
    errors.extend(check_ebnf(repo / "src/vitte/grammar/vitte.ebnf"))
    errors.extend(check_core_valid_tests(repo))
    if errors:
        for err in errors:
            print(f"[core-forbidden-syntax][error] {err}")
        return 1
    print("[core-forbidden-syntax] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

