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
CODE_PATTERN = re.compile(r"^[A-Z][A-Z0-9]*(?:_[A-Z0-9]+)*$")
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


def validate_fixture(value: object, path: Path) -> None:
    if not isinstance(value, dict):
        raise ValueError(f"{path}: diagnostic must be an object")
    missing = REQUIRED - value.keys()
    if missing:
        raise ValueError(f"{path}: missing fields: {', '.join(sorted(missing))}")
    if value["schema"] != "vitte.diagnostic" or value["schema_version"] != "1.0.0":
        raise ValueError(f"{path}: invalid schema identity")
    if not isinstance(value["code"], str) or not CODE_PATTERN.fullmatch(value["code"]):
        raise ValueError(f"{path}: diagnostic code is missing or malformed")
    if not isinstance(value["message_key"], str) or not CODE_PATTERN.fullmatch(value["message_key"]):
        raise ValueError(f"{path}: diagnostic message must reference a Fluent catalog key")
    if value["message_key"] != value["code"]:
        raise ValueError(f"{path}: message_key must equal the diagnostic code")
    validate_span(value["primary_span"], path, "primary_span")
    if not isinstance(value["labels"], list) or not value["labels"]:
        raise ValueError(f"{path}: at least one label is required")


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
    expected_phases = ["lexer", "parser", "sema", "typeck", "borrowck", "mir", "backend", "linker", "runtime"]
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
    code_names = [entry.get("code") for entry in codes["codes"] if isinstance(entry, dict)]
    duplicate_codes = sorted({code for code in code_names if code_names.count(code) > 1})
    if duplicate_codes:
        raise ValueError(f"{CODES}: duplicate diagnostic codes: {', '.join(duplicate_codes)}")
    fixtures = sorted(FIXTURES.glob("*.json"))
    if not fixtures:
        raise ValueError(f"{FIXTURES}: no diagnostic fixtures")
    for fixture in fixtures:
        validate_fixture(load(fixture), fixture)
    if not any(
        isinstance(value := load(fixture), dict)
        and isinstance(value.get("primary_span"), dict)
        and value["primary_span"].get("start", {}).get("line") != value["primary_span"].get("end", {}).get("line")
        for fixture in fixtures
    ):
        raise ValueError(f"{FIXTURES}: a multi-line span fixture is required")
    invalid_fixtures = sorted(INVALID_FIXTURES.glob("*.json"))
    for fixture in invalid_fixtures:
        try:
            validate_fixture(load(fixture), fixture)
        except ValueError:
            continue
        raise ValueError(f"{fixture}: invalid fixture unexpectedly passed")
    print(f"diagnostic schema ok: {SCHEMA.relative_to(ROOT)} ({len(fixtures)} valid, {len(invalid_fixtures)} invalid fixture(s))")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
