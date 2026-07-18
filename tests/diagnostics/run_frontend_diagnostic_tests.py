#!/usr/bin/env python3
"""Negative frontend diagnostic fixture runner."""

from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
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
            location_keys = {"code", "line", "column", "end_line", "end_column"}
            expected_location = {key: value for key, value in expected.items() if key in location_keys}
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
    stress_fixture = ROOT / "tests/diagnostics/frontend-stress/repeated-parser.vit"
    stress_diagnostics = analyze(stress_fixture.read_text(encoding="utf-8"), stress_fixture.name)
    repeated = [value for value in stress_diagnostics if value["code"] == "PARSE_E_TOPLEVEL_DECL_EXPECTED"]
    if len(repeated) != 3:
        raise SystemExit(f"parser repetition limit is unstable: expected 3, got {len(repeated)}")
    if len(stress_diagnostics) > 20:
        raise SystemExit(f"parser total diagnostic limit exceeded: {len(stress_diagnostics)}")
    valid_multiline = """space test\nproc calculate(\n    left: f64,\n    right: f64,\n    operation: Operation\n) -> f64 {\n    give left;\n}\n"""
    invalid_codes = {"PARSE_E_TOPLEVEL_DECL_EXPECTED", "PARSE_E_PARAMETER_COLON_EXPECTED"}
    leaked = [value for value in analyze(valid_multiline, "valid-multiline.vit") if value["code"] in invalid_codes]
    if leaked:
        raise SystemExit(f"valid multi-line procedure signature rejected: {leaked}")
    driver = ROOT / "bin/vittec0"
    environment = {**os.environ, "VITTE_ROOT": str(ROOT), "NO_COLOR": "1"}
    shell_result = subprocess.run(
        [str(driver), "check", "--lang", "en", str(ROOT / "tests/diagnostics/frontend/parser/missing-parameter-colon.vit")],
        cwd=ROOT, env=environment, text=True, capture_output=True, check=False,
    )
    if shell_result.returncode == 0:
        raise SystemExit("seed driver accepted a parameter without a colon")
    for expected_text in (
        "PARSE_E_PARAMETER_COLON_EXPECTED",
        "missing colon in procedure parameter",
        "missing-parameter-colon.vit:3:5",
        "right: f64",
    ):
        if expected_text not in shell_result.stderr:
            raise SystemExit(f"seed driver diagnostic is missing {expected_text!r}: {shell_result.stderr}")
    with tempfile.NamedTemporaryFile("w", suffix=".vit", encoding="utf-8") as valid_file:
        valid_file.write(valid_multiline)
        valid_file.flush()
        valid_result = subprocess.run(
            [str(driver), "check", valid_file.name], cwd=ROOT, env=environment,
            text=True, capture_output=True, check=False,
        )
    if valid_result.returncode != 0:
        raise SystemExit(f"seed driver rejected valid multi-line signature: {valid_result.stderr}")
    print(f"frontend diagnostics ok: {len(fixtures)} fixture(s)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
