#!/usr/bin/env python3
"""Negative frontend diagnostic fixture runner."""

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from diagnostic_frontend import analyze  # noqa: E402


def main() -> int:
    fixtures = sorted((ROOT / "tests/diagnostics/frontend").glob("**/*.vit"))
    for fixture in fixtures:
        expected_path = fixture.with_suffix(".expect.json")
        expected = json.loads(expected_path.read_text(encoding="utf-8"))
        diagnostics = analyze(fixture.read_text(encoding="utf-8"), fixture.name)
        if len(diagnostics) != 1:
            raise SystemExit(f"{fixture}: expected one diagnostic, got {len(diagnostics)}")
        actual = diagnostics[0]
        start = actual["primary_span"]["start"]
        end = actual["primary_span"]["end"]
        observed = {
            "code": actual["code"],
            "line": start["line"],
            "column": start["column"],
            "end_column": end["column"],
        }
        if observed != expected:
            raise SystemExit(f"{fixture}: expected {expected}, got {observed}")
    print(f"frontend diagnostics ok: {len(fixtures)} fixture(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
