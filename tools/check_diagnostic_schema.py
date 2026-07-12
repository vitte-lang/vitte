#!/usr/bin/env python3
"""Validate the canonical diagnostic schema and its dependency-free fixtures."""

from __future__ import annotations

import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = ROOT / "schemas/diagnostics/v1.schema.json"
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
    fixtures = sorted(FIXTURES.glob("*.json"))
    if not fixtures:
        raise ValueError(f"{FIXTURES}: no diagnostic fixtures")
    for fixture in fixtures:
        validate_fixture(load(fixture), fixture)
    print(f"diagnostic schema ok: {SCHEMA.relative_to(ROOT)} ({len(fixtures)} fixture(s))")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
