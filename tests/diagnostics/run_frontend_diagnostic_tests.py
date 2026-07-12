#!/usr/bin/env python3
"""Negative frontend diagnostic fixture runner."""

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

from diagnostic_frontend import analyze, levenshtein  # noqa: E402


def main() -> int:
    if levenshtein("pro", "proc") != 1 or levenshtein("prco", "proc") != 2:
        raise SystemExit("keyword Levenshtein distance is unstable")
    fixtures = sorted((ROOT / "tests/diagnostics/frontend").glob("**/*.vit"))
    for fixture in fixtures:
        expected_path = fixture.with_suffix(".expect.json")
        expected = json.loads(expected_path.read_text(encoding="utf-8"))
        diagnostics = analyze(fixture.read_text(encoding="utf-8"), fixture.name)
        if len(diagnostics) != 1:
            raise SystemExit(f"{fixture}: expected one diagnostic, got {len(diagnostics)}")
        actual = diagnostics[0]
        if actual["code"] == "E_BOOTSTRAP_NATIVE_SUBSET":
            raise SystemExit(f"{fixture}: source error was masked by bootstrap subset diagnostic")
        start = actual["primary_span"]["start"]
        end = actual["primary_span"]["end"]
        observed = {
            "code": actual["code"],
            "line": start["line"],
            "column": start["column"],
            "end_line": end["line"],
            "end_column": end["column"],
        }
        if observed != expected:
            expected_location = {key: value for key, value in expected.items() if key != "replacement"}
            if observed != expected_location:
                raise SystemExit(f"{fixture}: expected {expected_location}, got {observed}")
        if "replacement" in expected:
            replacements = [suggestion["replacement"] for suggestion in actual.get("suggestions", [])]
            if replacements != [expected["replacement"]]:
                raise SystemExit(f"{fixture}: expected replacement {expected['replacement']}, got {replacements}")
        if fixture.parent.name == "lexer":
            suggestions = actual.get("suggestions", [])
            if not suggestions or not isinstance(suggestions[0].get("replacement"), str):
                raise SystemExit(f"{fixture}: lexer diagnostic has no structured suggestion")
    print(f"frontend diagnostics ok: {len(fixtures)} fixture(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
