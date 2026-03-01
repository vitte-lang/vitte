#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path


DIAG_PREFIX = "VITTE-C"


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
    core_mod = repo / "src/vitte/packages/core/mod.vit"
    if not core_mod.exists():
        print(f"[core-mod-lint][error] missing file: {core_mod}")
        return 1

    text = core_mod.read_text(encoding="utf-8")
    lines = non_comment_non_empty(text)
    errs: list[str] = []

    if "PREAMBLE (API stable facade)" not in text:
        errs.append(f"{core_mod}: missing PREAMBLE block")
    if "<<< ROLE-CONTRACT" not in text:
        errs.append(f"{core_mod}: missing ROLE-CONTRACT block")

    if any(line.startswith("entry ") for line in lines):
        errs.append(f"{core_mod}: import-time side effect risk: 'entry' is forbidden in core facade")

    role_required = (
        "versioning:",
        "api_surface_stable:",
        "boundary:",
        "input_contract:",
        "output_contract:",
    )
    for key in role_required:
        if key not in text:
            errs.append(f"{core_mod}: ROLE-CONTRACT missing '{key}'")

    export_names: list[str] = []
    export_patterns = (
        re.compile(r"^pick\s+([A-Za-z_][A-Za-z0-9_]*)"),
        re.compile(r"^form\s+([A-Za-z_][A-Za-z0-9_]*)"),
        re.compile(r"^proc\s+([A-Za-z_][A-Za-z0-9_]*)"),
    )
    for line in lines:
        for pat in export_patterns:
            m = pat.match(line)
            if m:
                export_names.append(m.group(1))
                break

    if not export_names:
        errs.append(f"{core_mod}: no public exports found")

    snake_or_pascal = re.compile(r"^([a-z][a-z0-9_]*|[A-Z][A-Za-z0-9]*)$")
    for name in export_names:
        if not snake_or_pascal.match(name):
            errs.append(f"{core_mod}: exported name '{name}' violates naming convention")

    # Reserve coded diagnostics namespace for core checks.
    if DIAG_PREFIX not in text and "role:" in text:
        # This warning is advisory to keep the check non-blocking for now.
        print(f"[core-mod-lint][warn] {core_mod}: no {DIAG_PREFIX} reference yet in facade metadata")

    if errs:
        for e in errs:
            print(f"[core-mod-lint][error] {e}")
        print(f"[core-mod-lint] FAILED: {len(errs)} error(s)")
        return 1

    print("[core-mod-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
