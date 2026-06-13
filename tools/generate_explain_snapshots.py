#!/usr/bin/env python3
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT_DIR = ROOT / "tests" / "explain_snapshots"
sys.path.insert(0, str(ROOT / "tools"))
from diagnostic_catalog_data import diagnostic_family, explanation_fields, readable_from_code

CASES = (
    ("E0001", "en"),
    ("TYPECK_E_ASSIGN_MISMATCH", "en"),
    ("BORROWCK_E_USE_AFTER_MOVE", "en"),
    ("E0001", "fr"),
)


def parse_ftl(path: Path) -> dict[str, str]:
    messages: dict[str, str] = {}
    if not path.exists():
        return messages
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        messages[key.strip()] = value.strip()
    return messages


def phase_from_code(code: str) -> str:
    family = diagnostic_family(code)
    if family == "syntax":
        return "parser"
    if family == "type checking":
        return "typeck"
    if family == "ownership":
        return "borrowck"
    if family == "name and module analysis":
        return "sema"
    return family.replace(" ", "-")


def render(code: str, lang: str) -> str:
    locale_messages = parse_ftl(ROOT / "locales" / lang / "diagnostics.ftl")
    en_messages = parse_ftl(ROOT / "locales" / "en" / "diagnostics.ftl")
    message = locale_messages.get(code, en_messages.get(code, readable_from_code(code)))
    fields = explanation_fields(code, message)
    return "\n".join(
        [
            f"error code: {code}",
            f"lang: {lang}",
            f"phase: {phase_from_code(code)}",
            f"message: {message}",
            f"summary: {fields['summary']}",
            f"cause: {fields['cause']}",
            f"step 1: {fields['step1']}",
            f"fix: {fields['fix']}",
            f"example: {fields['example']}",
            "",
        ]
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    stale: list[str] = []
    for code, lang in CASES:
        path = OUT_DIR / f"{code}.{lang}.txt"
        expected = render(code, lang)
        if args.check:
            current = path.read_text(encoding="utf-8") if path.exists() else ""
            if current != expected:
                stale.append(str(path.relative_to(ROOT)))
        else:
            path.write_text(expected, encoding="utf-8")
            print(f"[explain-snapshots] wrote {path.relative_to(ROOT)}")
    if stale:
        print("[explain-snapshots][error] stale snapshots")
        for path in stale:
            print(f"- {path}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
