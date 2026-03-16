#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DIAG_LIST_PATH = ROOT / "src" / "compiler" / "frontend" / "diagnostics_messages.hpp"
LOCALES_DIR = ROOT / "locales"

ENTRY_RE = re.compile(r'X\((E\d+),\s*[A-Za-z0-9_]+,\s*"([^"]+)"\)')


def message_key(msg: str) -> str:
    out: list[str] = []
    prev_us = False
    for c in msg:
        if c.isalnum():
            out.append(c.lower())
            prev_us = False
        elif not prev_us:
            out.append("_")
            prev_us = True
    return "".join(out).strip("_")


def extract_messages() -> list[tuple[str, str]]:
    text = DIAG_LIST_PATH.read_text(encoding="utf-8")
    return ENTRY_RE.findall(text)


def parse_ftl(path: Path) -> dict[str, str]:
    if not path.exists():
        return {}
    data: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        data[key.strip()] = value.strip()
    return data


def file_header(locale: str) -> list[str]:
    if locale == "fr":
        return [
            "# Vitte diagnostics (French)",
            "#",
            "# Fichier synchronise depuis la table centrale.",
            "# Les traductions existantes sont conservees quand elles sont presentes.",
            "",
        ]
    return [
        "# Vitte diagnostics (English)",
        "#",
        "# File synchronized from the centralized diagnostics list.",
        "",
    ]


def render_locale(locale: str, messages: list[tuple[str, str]], existing: dict[str, str]) -> str:
    lines = file_header(locale)

    for code, msg in messages:
        value = msg if locale == "en" else existing.get(code, msg)
        lines.append(f"{code} = {value}")

    for code, msg in messages:
        key = message_key(msg)
        value = msg if locale == "en" else existing.get(key, existing.get(code, msg))
        lines.append(f"{key} = {value}")

    return "\n".join(lines) + "\n"


def sync_locale(locale: str, check: bool) -> int:
    messages = extract_messages()
    out_path = LOCALES_DIR / locale / "diagnostics.ftl"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    current = parse_ftl(out_path)
    expected = render_locale(locale, messages, current)

    if check:
        actual = out_path.read_text(encoding="utf-8") if out_path.exists() else ""
        if actual != expected:
            print(f"[update-diagnostics-ftl] out of sync: {out_path}")
            return 1
        print(f"[update-diagnostics-ftl] OK {out_path}")
        return 0

    out_path.write_text(expected, encoding="utf-8")
    print(f"[update-diagnostics-ftl] wrote {out_path}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Synchronize diagnostics.ftl files from centralized diagnostics")
    parser.add_argument("--check", action="store_true", help="fail if target files are out of sync")
    parser.add_argument("--locale", action="append", dest="locales", help="locale to sync (default: en + fr)")
    args = parser.parse_args()

    locales = args.locales or ["en", "fr"]
    rc = 0
    for locale in locales:
        rc |= sync_locale(locale, check=args.check)
    return rc


if __name__ == "__main__":
    raise SystemExit(main())
