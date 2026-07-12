#!/usr/bin/env python3
"""Validate the canonical diagnostic schema and its dependency-free fixtures."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT / "schemas/diagnostics/v1.schema.json"
SEVERITIES = ROOT / "schemas/diagnostics/severities.json"
PHASES = ROOT / "schemas/diagnostics/phases.json"
CODES = ROOT / "schemas/diagnostics/codes.json"
FIXTURES = ROOT / "tests/diagnostics/schema"
REQUIRED = {
    "schema", "schema_version", "code", "severity", "phase", "message",
    "primary_span", "labels", "notes", "helps", "suggestions",
}


def load(path: Path) -> object:
    with path.open(encoding="utf-8") as stream:
        return json.load(stream)


def validate_fixture(value: object, path: Path) -> None:
    if not isinstance(value, dict):
        raise ValueError(f"{path}: diagnostic must be an object")
    missing = REQUIRED - value.keys()
    if missing:
        raise ValueError(f"{path}: missing fields: {', '.join(sorted(missing))}")
    if value["schema"] != "vitte.diagnostic" or value["schema_version"] != "1.0.0":
        raise ValueError(f"{path}: invalid schema identity")
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
    print(f"diagnostic schema ok: {SCHEMA.relative_to(ROOT)} ({len(fixtures)} fixture(s))")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
