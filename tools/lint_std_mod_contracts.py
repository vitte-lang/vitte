#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

DIAG_PREFIX = "VITTE-S"


def non_comment_non_empty(text: str) -> list[str]:
    out: list[str] = []
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        out.append(line)
    return out


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    mod = repo / "src/vitte/packages/std/mod.vit"
    if not mod.exists():
        print(f"[std-mod-lint][error] missing file: {mod}")
        return 1

    text = mod.read_text(encoding="utf-8")
    lines = non_comment_non_empty(text)
    errs: list[str] = []

    if "PREAMBLE (API stable facade)" not in text:
        errs.append(f"{mod}: missing PREAMBLE block")
    if "<<< ROLE-CONTRACT" not in text:
        errs.append(f"{mod}: missing ROLE-CONTRACT block")
    if any(line.startswith("entry ") for line in lines):
        errs.append(f"{mod}: import-time side effect risk: 'entry' is forbidden")

    required = (
        "versioning:",
        "api_surface_stable:",
        "boundary:",
        "input_contract:",
        "output_contract:",
        "diagnostics:",
    )
    for key in required:
        if key not in text:
            errs.append(f"{mod}: ROLE-CONTRACT missing '{key}'")

    if DIAG_PREFIX not in text:
        errs.append(f"{mod}: diagnostics namespace must reference {DIAG_PREFIX}")

    export_names: list[str] = []
    pats = (
        re.compile(r"^pick\s+([A-Za-z_][A-Za-z0-9_]*)"),
        re.compile(r"^form\s+([A-Za-z_][A-Za-z0-9_]*)"),
        re.compile(r"^proc\s+([A-Za-z_][A-Za-z0-9_]*)"),
    )
    for line in lines:
        for pat in pats:
            m = pat.match(line)
            if m:
                export_names.append(m.group(1))
                break

    if not export_names:
        errs.append(f"{mod}: no public exports found")

    snake_or_pascal = re.compile(r"^([a-z][a-z0-9_]*|[A-Z][A-Za-z0-9]*)$")
    for name in export_names:
        if not snake_or_pascal.match(name):
            errs.append(f"{mod}: exported name '{name}' violates naming convention")

    if errs:
        for e in errs:
            print(f"[std-mod-lint][error] {e}")
        return 1
    print("[std-mod-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
