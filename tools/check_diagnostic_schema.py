#!/usr/bin/env python3
"""Validate the canonical diagnostic schema and its dependency-free fixtures."""

from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT / "schemas/diagnostics/v1.schema.json"
SEVERITIES = ROOT / "schemas/diagnostics/severities.json"
PHASES = ROOT / "schemas/diagnostics/phases.json"
CODES = ROOT / "schemas/diagnostics/codes.json"
FIXTURES = ROOT / "tests/diagnostics/schema"
INVALID_FIXTURES = ROOT / "tests/diagnostics/schema-invalid"
CANONICAL_CODE_PATTERN = re.compile(r"^(LEX|PAR|RES|SEM|TYP|BOR|MIR|IR|GEN|LNK|ICE)[0-9]{4}$")
MESSAGE_KEY_PATTERN = re.compile(r"^[A-Z][A-Z0-9]*(?:_[A-Z0-9]+)*$")
PHASE_PREFIX = {
    "lexer": "LEX",
    "parser": "PAR",
    "resolver": "RES",
    "sema": "SEM",
    "typeck": "TYP",
    "borrowck": "BOR",
    "mir": "MIR",
    "ir": "IR",
    "codegen": "GEN",
    "linker": "LNK",
    "ice": "ICE",
}
VAGUE_CAUSE_PATTERNS = (
    "compiler contract",
    "diagnostic rule",
    "error occurred",
    "failure occurred",
    "invalid input",
    "invalid program",
    "something went wrong",
)
VAGUE_ACTION_PATTERNS = (
    "fix the error",
    "fix the issue",
    "follow the primary help text",
    "inspect diagnostics",
    "repair the highlighted compiler contract",
    "try again",
)
REQUIRED = {
    "schema", "schema_version", "code", "severity", "phase", "message_key", "message",
    "primary_span", "labels", "notes", "helps", "suggestions",
}


def load(path: Path) -> object:
    with path.open(encoding="utf-8") as stream:
        return json.load(stream)


def validate_span(value: object, path: Path, field: str) -> None:
    if not isinstance(value, dict) or not isinstance(value.get("file"), str) or not value["file"]:
        raise ValueError(f"{path}: {field} must contain a non-empty file")
    positions: list[tuple[int, int, int]] = []
    for name in ("start", "end"):
        position = value.get(name)
        if not isinstance(position, dict):
            raise ValueError(f"{path}: {field}.{name} must be an object")
        line, column, byte = position.get("line"), position.get("column"), position.get("byte")
        if not isinstance(line, int) or line < 1 or not isinstance(column, int) or column < 1:
            raise ValueError(f"{path}: {field}.{name} line and column are one-based")
        if not isinstance(byte, int) or byte < 0:
            raise ValueError(f"{path}: {field}.{name}.byte must be non-negative")
        positions.append((line, column, byte))
    if positions[1] < positions[0] or positions[1][2] < positions[0][2]:
        raise ValueError(f"{path}: {field} end precedes start")
    if positions[1] == positions[0]:
        raise ValueError(f"{path}: {field} must identify a non-empty real source range")


def reject_vague_text(text: str, path: Path, field: str, patterns: tuple[str, ...]) -> None:
    lowered = text.lower()
    for pattern in patterns:
        if pattern in lowered:
            raise ValueError(f"{path}: {field} is too vague: {text!r}")


def validate_fixture(value: object, path: Path, registry: dict[str, dict[str, object]]) -> None:
    if not isinstance(value, dict):
        raise ValueError(f"{path}: diagnostic must be an object")
    missing = REQUIRED - value.keys()
    if missing:
        raise ValueError(f"{path}: missing fields: {', '.join(sorted(missing))}")
    if value["schema"] != "vitte.diagnostic" or value["schema_version"] != "1.0.0":
        raise ValueError(f"{path}: invalid schema identity")
    if not isinstance(value["code"], str) or not CANONICAL_CODE_PATTERN.fullmatch(value["code"]):
        raise ValueError(f"{path}: diagnostic code is missing or malformed")
    entry = registry.get(value["code"])
    if entry is None:
        raise ValueError(f"{path}: diagnostic code is not registered: {value['code']}")
    if entry.get("phase") != value["phase"]:
        raise ValueError(f"{path}: diagnostic code {value['code']} belongs to phase {entry.get('phase')}, not {value['phase']}")
    if not isinstance(value["message_key"], str) or not MESSAGE_KEY_PATTERN.fullmatch(value["message_key"]):
        raise ValueError(f"{path}: diagnostic message must reference a Fluent catalog key")
    aliases = entry.get("aliases")
    if value["message_key"] != entry.get("message_key") and (
        not isinstance(aliases, list) or value["message_key"] not in aliases
    ):
        raise ValueError(f"{path}: message_key must be the registered Fluent key or alias for {value['code']}")
    validate_span(value["primary_span"], path, "primary_span")
    if not isinstance(value["labels"], list) or not value["labels"]:
        raise ValueError(f"{path}: at least one label is required")
    primary_labels = 0
    for index, label in enumerate(value["labels"]):
        if not isinstance(label, dict) or label.get("kind") not in {"primary", "secondary"}:
            raise ValueError(f"{path}: labels[{index}] has an invalid kind")
        if not isinstance(label.get("message"), str) or not label["message"]:
            raise ValueError(f"{path}: labels[{index}] requires a message")
        validate_span(label.get("span"), path, f"labels[{index}].span")
        primary_labels += label["kind"] == "primary"
    if primary_labels != 1:
        raise ValueError(f"{path}: exactly one primary label is required")
    primary_label = next(label for label in value["labels"] if label["kind"] == "primary")
    if primary_label.get("span") != value["primary_span"]:
        raise ValueError(f"{path}: primary label span must match primary_span")
    if not isinstance(value["notes"], list) or not value["notes"]:
        raise ValueError(f"{path}: at least one note is required to describe the real cause")
    for index, note in enumerate(value["notes"]):
        if not isinstance(note, str) or not note:
            raise ValueError(f"{path}: notes[{index}] requires a message")
        reject_vague_text(note, path, f"notes[{index}]", VAGUE_CAUSE_PATTERNS)
    if not isinstance(value["helps"], list):
        raise ValueError(f"{path}: helps must be an array")
    for index, help_text in enumerate(value["helps"]):
        if not isinstance(help_text, str) or not help_text:
            raise ValueError(f"{path}: helps[{index}] requires a message")
        reject_vague_text(help_text, path, f"helps[{index}]", VAGUE_ACTION_PATTERNS)
    if not isinstance(value["suggestions"], list):
        raise ValueError(f"{path}: suggestions must be an array")
    for index, suggestion in enumerate(value["suggestions"]):
        if not isinstance(suggestion, dict):
            raise ValueError(f"{path}: suggestions[{index}] must be an object")
        if suggestion.get("applicability") not in {"machine-applicable", "maybe-incorrect", "has-placeholders", "unspecified"}:
            raise ValueError(f"{path}: suggestions[{index}] has invalid applicability")
        if not isinstance(suggestion.get("message"), str) or not isinstance(suggestion.get("replacement"), str):
            raise ValueError(f"{path}: suggestions[{index}] requires message and replacement")
        validate_span(suggestion.get("span"), path, f"suggestions[{index}].span")
        reject_vague_text(suggestion["message"], path, f"suggestions[{index}].message", VAGUE_ACTION_PATTERNS)
    if not value["helps"] and not value["suggestions"]:
        raise ValueError(f"{path}: at least one help or suggestion is required to describe a corrective action")


def main() -> int:
    schema = load(SCHEMA)
    if not isinstance(schema, dict) or schema.get("$schema") != "https://json-schema.org/draft/2020-12/schema":
        raise ValueError(f"{SCHEMA}: expected JSON Schema draft 2020-12")
    if set(schema.get("required", [])) != REQUIRED:
        raise ValueError(f"{SCHEMA}: canonical required fields drifted")
    severities = load(SEVERITIES)
    if not isinstance(severities, dict) or not isinstance(severities.get("severities"), list):
        raise ValueError(f"{SEVERITIES}: invalid severity registry")
    names = [entry.get("name") for entry in severities["severities"] if isinstance(entry, dict)]
    expected = ["error", "warning", "note", "help"]
    if names != expected:
        raise ValueError(f"{SEVERITIES}: severity order must be {expected}")
    schema_names = schema.get("properties", {}).get("severity", {}).get("enum", [])
    if schema_names != expected:
        raise ValueError(f"{SCHEMA}: severity enum differs from registry")
    ranks = [entry.get("rank") for entry in severities["severities"] if isinstance(entry, dict)]
    if ranks != sorted(ranks, reverse=True) or len(set(ranks)) != len(ranks):
        raise ValueError(f"{SEVERITIES}: severity ranks must be unique and descending")
    phases = load(PHASES)
    if not isinstance(phases, dict) or not isinstance(phases.get("phases"), list):
        raise ValueError(f"{PHASES}: invalid phase registry")
    phase_names = [entry.get("name") for entry in phases["phases"] if isinstance(entry, dict)]
    expected_phases = ["lexer", "parser", "resolver", "sema", "typeck", "borrowck", "mir", "ir", "codegen", "linker", "ice"]
    if phase_names != expected_phases:
        raise ValueError(f"{PHASES}: phase order must be {expected_phases}")
    schema_phases = schema.get("properties", {}).get("phase", {}).get("enum", [])
    if schema_phases != expected_phases:
        raise ValueError(f"{SCHEMA}: phase enum differs from registry")
    phase_order = [entry.get("order") for entry in phases["phases"] if isinstance(entry, dict)]
    if phase_order != sorted(phase_order) or len(set(phase_order)) != len(phase_order):
        raise ValueError(f"{PHASES}: phase order values must be unique and ascending")
    codes = load(CODES)
    if not isinstance(codes, dict) or not isinstance(codes.get("codes"), list) or not codes["codes"]:
        raise ValueError(f"{CODES}: invalid or empty diagnostic code registry")
    if codes.get("schema_version") != "2.0.0":
        raise ValueError(f"{CODES}: diagnostic code registry must use schema_version 2.0.0")
    if codes.get("phase_prefixes") != PHASE_PREFIX:
        raise ValueError(f"{CODES}: phase_prefixes must match the canonical compiler phase convention")
    code_names = [entry.get("code") for entry in codes["codes"] if isinstance(entry, dict)]
    duplicate_codes = sorted({code for code in code_names if code_names.count(code) > 1})
    if duplicate_codes:
        raise ValueError(f"{CODES}: duplicate diagnostic codes: {', '.join(duplicate_codes)}")
    registry: dict[str, dict[str, object]] = {}
    aliases: dict[str, str] = {}
    previous_sort_key: tuple[int, str] | None = None
    phase_order = {phase: index for index, phase in enumerate(PHASE_PREFIX)}
    for index, entry in enumerate(codes["codes"]):
        if not isinstance(entry, dict):
            raise ValueError(f"{CODES}: codes[{index}] must be an object")
        code = entry.get("code")
        phase = entry.get("phase")
        message_key = entry.get("message_key")
        entry_aliases = entry.get("aliases")
        if not isinstance(code, str) or not CANONICAL_CODE_PATTERN.fullmatch(code):
            raise ValueError(f"{CODES}: codes[{index}].code is not canonical: {code!r}")
        if phase not in PHASE_PREFIX:
            raise ValueError(f"{CODES}: {code} has an unknown phase: {phase!r}")
        if not code.startswith(PHASE_PREFIX[phase]):
            raise ValueError(f"{CODES}: {code} does not match phase {phase}")
        if not isinstance(message_key, str) or not MESSAGE_KEY_PATTERN.fullmatch(message_key):
            raise ValueError(f"{CODES}: {code} has an invalid message_key")
        if not isinstance(entry_aliases, list) or entry_aliases != [message_key]:
            raise ValueError(f"{CODES}: {code} must have exactly one legacy alias matching message_key")
        if entry.get("stable") is not True:
            raise ValueError(f"{CODES}: {code} must be marked stable=true")
        if entry.get("deprecated") not in {True, False}:
            raise ValueError(f"{CODES}: {code} must declare deprecated=true or deprecated=false")
        if message_key in aliases:
            raise ValueError(f"{CODES}: legacy diagnostic key {message_key} maps to both {aliases[message_key]} and {code}")
        sort_key = (phase_order[phase], code)
        if previous_sort_key is not None and sort_key <= previous_sort_key:
            raise ValueError(f"{CODES}: codes must be sorted by phase and stable public code")
        previous_sort_key = sort_key
        aliases[message_key] = code
        registry[code] = entry
    fixtures = sorted(FIXTURES.glob("*.json"))
    if not fixtures:
        raise ValueError(f"{FIXTURES}: no diagnostic fixtures")
    for fixture in fixtures:
        validate_fixture(load(fixture), fixture, registry)
    if not any(
        isinstance(value := load(fixture), dict)
        and isinstance(value.get("primary_span"), dict)
        and value["primary_span"].get("start", {}).get("line") != value["primary_span"].get("end", {}).get("line")
        for fixture in fixtures
    ):
        raise ValueError(f"{FIXTURES}: a multi-line span fixture is required")
    if not any(isinstance(value := load(fixture), dict) and len(value.get("labels", [])) > 1 for fixture in fixtures):
        raise ValueError(f"{FIXTURES}: a multiple-label fixture is required")
    if not any(isinstance(value := load(fixture), dict) and value.get("suggestions") for fixture in fixtures):
        raise ValueError(f"{FIXTURES}: a replacement suggestion fixture is required")
    invalid_fixtures = sorted(INVALID_FIXTURES.glob("*.json"))
    for fixture in invalid_fixtures:
        try:
            validate_fixture(load(fixture), fixture, registry)
        except ValueError:
            continue
        raise ValueError(f"{fixture}: invalid fixture unexpectedly passed")
    print(f"diagnostic schema ok: {SCHEMA.relative_to(ROOT)} ({len(fixtures)} valid, {len(invalid_fixtures)} invalid fixture(s))")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
