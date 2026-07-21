#!/usr/bin/env python3
"""Gate the verified counterfactual diagnostic surface."""

from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CANONICAL = ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "diagnostic.vit"
COUNTERFACTUAL = ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "counterfactual.vit"
JSON_RENDERER = ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "json.vit"
TEXT_RENDERER = ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "render.vit"
TESTS = ROOT / "src" / "vitte" / "compiler" / "tests" / "diagnostic_snapshot_tests.vit"
MAKEFILE = ROOT / "Makefile"
REPORT = ROOT / "target" / "reports" / "counterfactual_diagnostics" / "coverage.json"


CHECKS: tuple[tuple[Path, str], ...] = (
    (CANONICAL, "pick DiagnosticCorrectionOutcome"),
    (CANONICAL, "counterfactual_verified: bool"),
    (CANONICAL, "counterfactual_rank: u64"),
    (CANONICAL, "counterfactual_errors_removed: u64"),
    (CANONICAL, "counterfactual_total_errors: u64"),
    (CANONICAL, "counterfactual_outcomes: [DiagnosticCorrectionOutcome]"),
    (CANONICAL, "counterfactual_report: string"),
    (CANONICAL, "DiagnosticCorrectionOutcome.NotRecompiled"),
    (CANONICAL, "diagnostic_suggestion_with_counterfactual_result"),
    (CANONICAL, "diagnostic_verified_counterfactual_solution"),
    (CANONICAL, "has_new_error_outcome"),
    (CANONICAL, "diagnostic_verified_suggestions"),
    (CANONICAL, "diagnostic_counterfactual_tested_count"),
    (CANONICAL, "plans[i].counterfactual_verified"),
    (CANONICAL, "render_counterfactual_solution"),
    (CANONICAL, "compiler tested "),
    (COUNTERFACTUAL, "form CounterfactualHypothesis"),
    (COUNTERFACTUAL, "form CounterfactualVirtualProgram"),
    (COUNTERFACTUAL, "form CounterfactualPipelineCheck"),
    (COUNTERFACTUAL, "form CounterfactualVirtualResult"),
    (COUNTERFACTUAL, "pick CounterfactualPipelinePhase"),
    (COUNTERFACTUAL, "counterfactual_standard_pipeline_checks"),
    (COUNTERFACTUAL, "counterfactual_virtual_result_is_verified"),
    (COUNTERFACTUAL, "counterfactual_suggestion_from_virtual_result"),
    (COUNTERFACTUAL, "counterfactual_typeck_string_to_int_hypotheses"),
    (COUNTERFACTUAL, "counterfactual_attach_typeck_string_to_int"),
    (COUNTERFACTUAL, "parse_int("),
    (COUNTERFACTUAL, "creates 4 type incompatibilities in the body of `"),
    (COUNTERFACTUAL, "lexer, parser, resolver, typeck, and borrowck"),
    (COUNTERFACTUAL, "DiagnosticCorrectionOutcome.ErrorMoved"),
    (COUNTERFACTUAL, "DiagnosticCorrectionOutcome.BehaviorMayChange"),
    (JSON_RENDERER, "json_counterfactual_outcomes"),
    (JSON_RENDERER, "\"counterfactual_verified\""),
    (JSON_RENDERER, "\"counterfactual_outcomes\""),
    (JSON_RENDERER, "\"counterfactual_report\""),
    (TEXT_RENDERER, "render_counterfactual_line"),
    (TEXT_RENDERER, "verified correction rejected"),
    (TEXT_RENDERER, "diagnostic_counterfactual_tested_count"),
    (TESTS, "test_unverified_suggestion_is_not_certain"),
    (TESTS, "test_counterfactual_rejects_new_error_solution"),
    (TESTS, "test_typeck_counterfactual_argument_example_contract"),
    (TESTS, "test_counterfactual_virtual_pipeline_contract"),
    (TESTS, "counterfactual_virtual_result_is_verified(success)"),
    (TESTS, "!counterfactual_virtual_result_is_verified(failure)"),
    (TESTS, "sample_counterfactual_typeck_argument_diagnostic"),
    (TESTS, "error[TYP0042]"),
    (TESTS, "compiler tested 3 correction(s) in a virtual copy"),
    (TESTS, "creates 4 type incompatibilities in the body of `repeat`"),
    (TESTS, "diagnostic_verified_counterfactual_solution"),
    (TESTS, "counterfactual_verified"),
    (MAKEFILE, "src/vitte/compiler/diagnostics/counterfactual.vit"),
    (MAKEFILE, "counterfactual-diagnostics"),
)


def main() -> int:
    failures: list[str] = []
    for path, needle in CHECKS:
        text = path.read_text(encoding="utf-8")
        if needle not in text:
            failures.append(f"{path.relative_to(ROOT)} missing {needle!r}")

    covered = len(CHECKS) - len(failures)
    total = len(CHECKS)
    percent = int((covered * 100) / total)
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(
        json.dumps(
            {
                "status": "pass" if not failures else "fail",
                "covered": covered,
                "total": total,
                "percent": percent,
                "failures": failures,
            },
            indent=2,
            sort_keys=True,
        )
        + "\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[counterfactual-diagnostics][error] {failure}", file=sys.stderr)
        print(f"[counterfactual-diagnostics] coverage={covered}/{total} ({percent}%) report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[counterfactual-diagnostics] OK coverage={covered}/{total} ({percent}%) report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
