#!/usr/bin/env python3
"""Generate or verify stable snapshots for every parser diagnostic fixture."""

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from diagnostic_frontend import analyze  # noqa: E402

FIXTURES = ROOT / "tests/diagnostics/frontend/parser"
SNAPSHOT = ROOT / "tests/diagnostics/parser-snapshots.json"


def normalize(value: dict[str, Any]) -> dict[str, Any]:
    return {
        "code": value["code"],
        "severity": value["severity"],
        "phase": value["phase"],
        "message_key": value["message_key"],
        "primary_span": value["primary_span"],
        "labels": value.get("labels", []),
        "notes": value.get("notes", []),
        "helps": value.get("helps", []),
        "suggestions": value.get("suggestions", []),
    }


def build_snapshots() -> dict[str, list[dict[str, Any]]]:
    snapshots: dict[str, list[dict[str, Any]]] = {}
    for fixture in sorted(FIXTURES.glob("*.vit")):
        diagnostics = analyze(fixture.read_text(encoding="utf-8"), fixture.name)
        snapshots[fixture.name] = [normalize(value) for value in diagnostics]
    return snapshots


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--update", action="store_true")
    args = parser.parse_args()
    rendered = json.dumps(build_snapshots(), ensure_ascii=False, indent=2, sort_keys=True) + "\n"
    if args.update:
        SNAPSHOT.write_text(rendered, encoding="utf-8")
        print(f"updated {SNAPSHOT.relative_to(ROOT)}")
        return 0
    if not SNAPSHOT.exists() or SNAPSHOT.read_text(encoding="utf-8") != rendered:
        print("parser diagnostic snapshots are stale; run with --update", file=sys.stderr)
        return 1
    print(f"parser diagnostic snapshots ok: {len(build_snapshots())} fixture(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
