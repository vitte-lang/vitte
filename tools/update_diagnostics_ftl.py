#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

DIAG_LIST_PATH = ROOT / "src" / "compiler" / "frontend" / "diagnostics_messages.hpp"


def message_key(msg: str) -> str:
    out = []
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
    entries = re.findall(r'X\((E\d+),\s*[A-Za-z0-9_]+,\s*"([^"]+)"\)', text)
    return sorted(entries, key=lambda t: t[0])


def main() -> int:
    messages = extract_messages()
    out_path = ROOT / "locales" / "en" / "diagnostics.ftl"
    out_path.parent.mkdir(parents=True, exist_ok=True)

    lines = [
        "# Vitte diagnostics (English)",
        "#",
        "# Keys are auto-generated from the original English message:",
        "# - lowercased",
        "# - non-alnum -> underscore",
        "# Example:",
        "#   \"expected identifier\" -> key \"expected_identifier\"",
        "#",
        "# Add entries as needed to override messages.",
        "",
    ]

    for code, msg in messages:
        lines.append(f"{code} = {msg}")
    for code, msg in messages:
        lines.append(f"{message_key(msg)} = {msg}")

    out_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"[update-diagnostics-ftl] wrote {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
