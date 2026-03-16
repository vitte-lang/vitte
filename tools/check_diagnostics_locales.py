#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "src/compiler/frontend/diagnostics_messages.hpp"
EN = ROOT / "locales/en/diagnostics.ftl"
FR = ROOT / "locales/fr/diagnostics.ftl"
CORE_CODES = ROOT / "tests/diag_snapshots/core_diagnostic_codes.txt"


ENTRY_RE = re.compile(r'X\((E\d+),\s*\w+,\s*"([^"]+)"\)')


def parse_header() -> list[tuple[str, str]]:
    text = HEADER.read_text(encoding="utf-8")
    return ENTRY_RE.findall(text)


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
    header_entries = parse_header()
    expected_texts = {code: text for code, text in header_entries}

    selected_codes: list[str] = []
    for raw in CORE_CODES.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        selected_codes.append(line)

    errors: list[str] = []

    en = parse_ftl(EN)
    fr = parse_ftl(FR)

    for locale_path, data in ((EN, en), (FR, fr)):
        for code in selected_codes:
            if code not in data:
                errors.append(f"{locale_path}: missing code key {code}")

    for code in selected_codes:
        text = expected_texts[code]
        if en.get(code) != text:
            errors.append(f"{EN}: code {code} != header text '{text}'")

    if errors:
        print("[diagnostics-locales] FAILED")
        for err in errors:
            print(f"- {err}")
        return 1

    print(f"[diagnostics-locales] OK codes={len(selected_codes)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
