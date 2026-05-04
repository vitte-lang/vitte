#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EN = ROOT / "locales/en/diagnostics.ftl"
FR = ROOT / "locales/fr/diagnostics.ftl"
CORE_CODES = ROOT / "tests/diag_snapshots/core_diagnostic_codes.txt"


def parse_ftl(path: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        out[key.strip()] = value.strip()
    return out


def main() -> int:
    selected_codes = [
        line.strip()
        for line in CORE_CODES.read_text(encoding="utf-8").splitlines()
        if line.strip() and not line.strip().startswith("#")
    ]
    errors: list[str] = []
    locales = ((EN, parse_ftl(EN)), (FR, parse_ftl(FR)))
    for locale_path, data in locales:
        for code in selected_codes:
            if code not in data:
                errors.append(f"{locale_path}: missing code key {code}")

    if errors:
        print("[diagnostics-locales] FAILED")
        for err in errors:
            print(f"- {err}")
        return 1

    print(f"[diagnostics-locales] OK codes={len(selected_codes)}")
    return 0
