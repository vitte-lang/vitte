#!/usr/bin/env python3
"""Validate that stdlib compilation failures keep structured diagnostics."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TEST = ROOT / "src" / "vitte" / "compiler" / "tests" / "diagnostic_snapshot_tests.vit"
MAKEFILE = ROOT / "Makefile"

REQUIRED_FRAGMENTS = (
    "proc sample_stdlib_diagnostic() -> Diagnostic",
    '"STDLIB_E_CONTRACT"',
    "DiagnosticPhase.Driver",
    '"src/vitte/stdlib/alloc/vec.vitl"',
    "primary_label(",
    "diagnostic_with_explanation(",
    "diagnostic_with_probable_fix(",
    "diagnostic_with_documentation(",
    "diagnostic_with_suggestion_detail(",
    "json_report(report)",
    "diagnostic_text_line(report.diagnostics[0])",
    'find(rendered, "build failed") < 0',
    "report.error_count == 1",
    "report.warning_count == 0",
    "len(report.diagnostics[0].labels) >= 1",
    "len(report.diagnostics[0].notes) > 0",
    "len(report.diagnostics[0].helps) > 0",
    "len(report.diagnostics[0].suggestions) > 0",
)

FORBIDDEN_ONLY_SUMMARY = (
    "stdlib build failed",
    "stdlib failed",
    "build failed",
)


def main() -> int:
    text = TEST.read_text(encoding="utf-8")
    failures: list[str] = []

    for fragment in REQUIRED_FRAGMENTS:
        if fragment not in text:
            failures.append(f"missing fragment in diagnostic snapshot test: {fragment}")

    body_match = re.search(
        r"proc test_stdlib_diagnostic_contract\(\) -> bool \{(?P<body>.*?)\n\}",
        text,
        re.S,
    )
    if not body_match:
        failures.append("missing test_stdlib_diagnostic_contract body")
    else:
        body = body_match.group("body")
        for forbidden in FORBIDDEN_ONLY_SUMMARY:
            if forbidden in body and f'find(rendered, "{forbidden}") < 0' not in body:
                failures.append(f"stdlib diagnostic test allows summary-only message: {forbidden}")

    makefile = MAKEFILE.read_text(encoding="utf-8")
    if "stdlib-diagnostics:" not in makefile:
        failures.append("Makefile must expose stdlib-diagnostics")
    if "tools/check_stdlib_diagnostic_contract.py" not in makefile:
        failures.append("stdlib-diagnostics target must run this contract")

    if failures:
        for failure in failures:
            print(f"[stdlib-diagnostics][error] {failure}", file=sys.stderr)
        return 1

    print("[stdlib-diagnostics] OK structured stdlib diagnostics contract is enforced")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
