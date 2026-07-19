#!/usr/bin/env python3
"""Lint short compiler diagnostic messages for a consistent house style."""

from __future__ import annotations

import ast
import json
import re
import sys
from pathlib import Path
from typing import Any, Iterable

ROOT = Path(__file__).resolve().parents[1]
ALLOWED_UPPER_PREFIXES = (
    "ABI",
    "AST",
    "CLI",
    "FFI",
    "HIR",
    "IO",
    "IR",
    "LSP",
    "MIR",
    "UTF",
    "Vitte",
    "`",
)
MESSAGE_KEYS = {"message", "label", "help", "note"}
GENERIC_FIX_PATTERNS = (
    "change the expression, annotation, or call argument so both sides agree",
    "declare the symbol once, import it explicitly, or use the canonical module path",
    "use only supported const operations and guard division, overflow, or cycles",
    "shorten the borrow, reorder the use, or borrow/clone before moving",
    "balance delimiters, complete the missing token, or rewrite the local expression",
    "follow the primary help text",
)


def catalog_messages() -> Iterable[tuple[Path, int, str, str]]:
    path = ROOT / "locales/en/diagnostics.ftl"
    pattern = re.compile(r"^([A-Z][A-Z0-9_]*)\s*=\s*(.+)$")
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = pattern.match(line)
        if match:
            yield path, line_number, match.group(1), match.group(2).strip()


def canonical_fix_messages() -> Iterable[tuple[Path, int, str, str]]:
    path = ROOT / "tools/diagnostic_catalog_data.py"
    pattern = re.compile(r'fields\["fix"\]\s*=\s*(?P<literal>.+)$')
    for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        match = pattern.search(line)
        if match:
            try:
                value = ast.literal_eval(match.group("literal"))
            except (SyntaxError, ValueError):
                continue
            if isinstance(value, str):
                yield path, line_number, "fix", value


def walk_json_messages(path: Path, value: Any, key: str = "") -> Iterable[tuple[Path, int, str, str]]:
    if isinstance(value, dict):
        for child_key, child_value in value.items():
            if child_key in MESSAGE_KEYS and isinstance(child_value, str):
                yield path, 0, child_key, child_value
            elif child_key in {"helps", "notes"} and isinstance(child_value, list):
                for entry in child_value:
                    if isinstance(entry, str):
                        yield path, 0, child_key, entry
                    elif isinstance(entry, dict) and isinstance(entry.get("message"), str):
                        yield path, 0, child_key, entry["message"]
            yield from walk_json_messages(path, child_value, child_key)
    elif isinstance(value, list):
        for item in value:
            yield from walk_json_messages(path, item, key)


def schema_messages() -> Iterable[tuple[Path, int, str, str]]:
    for path in sorted((ROOT / "tests/diagnostics").rglob("*.json")):
        try:
            data = json.loads(path.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            continue
        yield from walk_json_messages(path, data)


def snapshot_messages() -> Iterable[tuple[Path, int, str, str]]:
    paths = [
        *sorted((ROOT / "tests/diagnostics/renderer").glob("*.txt")),
        *sorted((ROOT / "tests/diag_snapshots").glob("*.must")),
        *sorted((ROOT / "tests/diag_snapshots").glob("*.json.must")),
    ]
    for path in paths:
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            text = line.strip()
            if not text:
                continue
            header = re.match(r"^(error|warning|fatal)\[[^\]]+\](?::|\s+\w+:)\s*(.+)$", text)
            if header:
                yield path, line_number, "message", header.group(2)
                continue
            field = re.match(r'(?:"(?:message|helps|notes)"\s*:\s*)?(?:\[\{)?"message"\s*:\s*"([^"]+)', text)
            if field:
                yield path, line_number, "message", field.group(1)
                continue
            note_help = re.match(r'^(?:=\s*)?(note|help):\s*(.+)$', text)
            if note_help:
                yield path, line_number, note_help.group(1), note_help.group(2)
                continue
            suggestion = re.match(r'^(?:=\s*)?suggestion(?:\[[^\]]+\]|:)\s*(.+)$', text)
            if suggestion:
                yield path, line_number, "suggestion", suggestion.group(1)


def is_style_exempt(message: str) -> bool:
    return message.startswith(ALLOWED_UPPER_PREFIXES)


def lint_message(path: Path, line_number: int, kind: str, message: str) -> list[str]:
    stripped = message.strip()
    if not stripped or stripped.startswith(("{", "[")):
        return []
    errors: list[str] = []
    location = str(path.relative_to(ROOT))
    if line_number:
        location = f"{location}:{line_number}"
    prefix = f"{location}: {kind}: {stripped!r}"
    if stripped[0].isupper() and not is_style_exempt(stripped):
        errors.append(f"{prefix}: starts with an uppercase letter")
    if stripped.endswith((".", "!", ":", ";")):
        errors.append(f"{prefix}: ends with terminal punctuation")
    if re.search(r"\b[Pp]lease\b", stripped):
        errors.append(f"{prefix}: uses please")
    if re.match(r"^(error|warning|fatal|note|help)[: ]", stripped, re.IGNORECASE):
        errors.append(f"{prefix}: repeats the diagnostic role")
    lowered = stripped.lower()
    for generic in GENERIC_FIX_PATTERNS:
        if generic in lowered:
            errors.append(f"{prefix}: uses a generic catch-all diagnostic fix")
    return errors


def main() -> int:
    errors: list[str] = []
    for path, line_number, kind, message in (
        *catalog_messages(),
        *canonical_fix_messages(),
        *schema_messages(),
        *snapshot_messages(),
    ):
        errors.extend(lint_message(path, line_number, kind, message))
    if errors:
        print("[diagnostic-message-style] FAILED")
        for error in errors:
            print(f"- {error}")
        return 1
    print("[diagnostic-message-style] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
