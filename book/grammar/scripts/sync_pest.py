#!/usr/bin/env python3
"""Synchronize src/vitte/grammar/vitte.pest against current EBNF revision."""

from __future__ import annotations

import argparse
import hashlib
from pathlib import Path


SOURCE_REL = "src/vitte/grammar/vitte.ebnf"
TARGET_REL = "src/vitte/grammar/vitte.pest"


def _strip_generated_header(text: str) -> str:
    if not text.startswith("// GENERATED FILE - DO NOT EDIT"):
        return text
    lines = text.splitlines()
    i = 0
    while i < len(lines) and lines[i].startswith("//"):
        i += 1
    while i < len(lines) and not lines[i].strip():
        i += 1
    return ("\n".join(lines[i:]) + ("\n" if lines[i:] else ""))


def _render(source_text: str, pest_body: str) -> str:
    source_hash = hashlib.sha256(source_text.encode("utf-8")).hexdigest()
    header = [
        "// GENERATED FILE - DO NOT EDIT",
        f"// source: {SOURCE_REL}",
        f"// source_sha256: {source_hash}",
        "// tool: book/grammar/scripts/sync_pest.py v1",
        "",
    ]
    return "\n".join(header) + pest_body


def main() -> int:
    parser = argparse.ArgumentParser(description="Sync vitte.pest metadata against vitte.ebnf")
    parser.add_argument("--check", action="store_true", help="fail if vitte.pest is out of sync")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[3]
    source = repo / SOURCE_REL
    target = repo / TARGET_REL

    if not source.exists():
        print(f"[pest-sync] missing source: {source}")
        return 1
    if not target.exists():
        print(f"[pest-sync] missing target: {target}")
        return 1

    source_text = source.read_text(encoding="utf-8")
    current = target.read_text(encoding="utf-8")
    pest_body = _strip_generated_header(current)
    expected = _render(source_text, pest_body)

    if args.check:
        if current != expected:
            print("[pest-sync] FAILED")
            print(f"- out of sync: {target}")
            return 1
        print("[pest-sync] OK")
        return 0

    target.write_text(expected, encoding="utf-8")
    print(f"[pest-sync] wrote {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

