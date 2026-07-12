#!/usr/bin/env python3
"""Exhaustive negative and false-positive matrix for canonical lexer diagnostics."""

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from diagnostic_frontend import analyze_lexer  # noqa: E402

EXPECTED = {
    "LEX_E_INVALID_CHAR",
    "LEX_E_UNTERMINATED_STRING",
    "LEX_E_INVALID_CHAR_LITERAL",
    "LEX_E_INVALID_ESCAPE",
    "LEX_E_INVALID_UNICODE",
    "LEX_E_INVALID_NUMBER",
    "LEX_E_TOKEN_TOO_LARGE",
    "LEX_E_UNTERMINATED_COMMENT",
}


def main() -> int:
    fixture_dir = ROOT / "tests/diagnostics/frontend/lexer"
    observed: set[str] = set()
    for source_path in sorted(fixture_dir.glob("*.vit")):
        expected = json.loads(source_path.with_suffix(".expect.json").read_text(encoding="utf-8"))
        diagnostics = analyze_lexer(source_path.read_text(encoding="utf-8"), source_path.name)
        if [value["code"] for value in diagnostics] != [expected["code"]]:
            raise SystemExit(f"{source_path}: unexpected diagnostics {[value['code'] for value in diagnostics]}")
        diagnostic = diagnostics[0]
        if diagnostic["schema_version"] != "1.0.0" or diagnostic["message_key"] != diagnostic["code"]:
            raise SystemExit(f"{source_path}: diagnostic schema/catalog contract failed")
        observed.add(diagnostic["code"])
    if observed != EXPECTED:
        raise SystemExit(f"lexer diagnostic coverage mismatch: missing={sorted(EXPECTED - observed)} extra={sorted(observed - EXPECTED)}")
    valid = ROOT / "tests/diagnostics/frontend-valid/lexer.vit"
    false_positives = analyze_lexer(valid.read_text(encoding="utf-8"), valid.name)
    if false_positives:
        raise SystemExit(f"{valid}: false positives {[value['code'] for value in false_positives]}")
    print(f"lexer diagnostics exhaustive: {len(observed)} codes, valid control clean")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
