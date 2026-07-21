#!/usr/bin/env python3
"""Gate actionable diagnostic suggestions.

The compiler may offer manual help, but an automatic or ranked suggestion must
carry evidence: a precise span, replacement, reason, verified phases, risk, and
public score. This script prevents vague catch-all suggestion wording from
creeping back into diagnostics.
"""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DIAGNOSTIC = ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "diagnostic.vit"
JSON_RENDERER = ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "json.vit"
COUNTERFACTUAL = ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "counterfactual.vit"
LSP = ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "lsp.vit"
TESTS = ROOT / "src" / "vitte" / "compiler" / "tests" / "diagnostic_snapshot_tests.vit"
MAKEFILE = ROOT / "Makefile"
REPORT = ROOT / "target" / "reports" / "suggestion_quality" / "coverage.json"

VAGUE_PATTERNS = (
    re.compile(r"\btry changing this\b", re.IGNORECASE),
    re.compile(r"\bconsider fixing\b", re.IGNORECASE),
    re.compile(r"\bmaybe use\b", re.IGNORECASE),
    re.compile(r"\bfix this\b", re.IGNORECASE),
    re.compile(r"\bsomething else\b", re.IGNORECASE),
)

REQUIRED_NEEDLES: tuple[tuple[Path, str], ...] = (
    (DIAGNOSTIC, "pick DiagnosticSuggestionStrategy"),
    (DIAGNOSTIC, "strategy: DiagnosticSuggestionStrategy"),
    (DIAGNOSTIC, "proof: string"),
    (DIAGNOSTIC, "verified_phases: [DiagnosticPhase]"),
    (DIAGNOSTIC, "risk: string"),
    (DIAGNOSTIC, "public_score: u64"),
    (DIAGNOSTIC, "errors_removed: u64"),
    (DIAGNOSTIC, "new_errors: u64"),
    (DIAGNOSTIC, "error_moved: bool"),
    (DIAGNOSTIC, "behavior_changed: bool"),
    (DIAGNOSTIC, "diagnostic_suggestion_public_score"),
    (DIAGNOSTIC, "diagnostic_suggestion_with_quality"),
    (DIAGNOSTIC, "suggestion_strategy_name"),
    (DIAGNOSTIC, "diagnostic_strategy_from_suggestion"),
    (DIAGNOSTIC, "diagnostic_parser_recovery_suggestions"),
    (DIAGNOSTIC, "close the currently open block"),
    (DIAGNOSTIC, "insert the missing separator"),
    (DIAGNOSTIC, "replace this token with the token expected"),
    (DIAGNOSTIC, "move the clause after the declaration header"),
    (DIAGNOSTIC, "remove the stray token"),
    (DIAGNOSTIC, "diagnostic_sema_resolution_suggestions"),
    (DIAGNOSTIC, "import the symbol from the module"),
    (DIAGNOSTIC, "rename this use to the closest visible symbol"),
    (DIAGNOSTIC, "make the declaration public"),
    (DIAGNOSTIC, "remove the duplicate declaration"),
    (DIAGNOSTIC, "rename this binding"),
    (DIAGNOSTIC, "qualify the symbol with its module path"),
    (DIAGNOSTIC, "diagnostic_with_origin_usage_declaration_consequence"),
    (DIAGNOSTIC, "machine_applicable: suggestion.machine_applicable and suggestion.counterfactual_verified"),
    (DIAGNOSTIC, "set out.proof = \"virtual recompilation checked \""),
    (DIAGNOSTIC, "set out.risk = if new_errors > 0"),
    (DIAGNOSTIC, "set out.public_score = diagnostic_suggestion_public_score"),
    (DIAGNOSTIC, "set text = text + \" strategy=\""),
    (DIAGNOSTIC, "set text = text + \" score=\""),
    (JSON_RENDERER, "\"strategy\""),
    (JSON_RENDERER, "\"proof\""),
    (JSON_RENDERER, "\"verified_phases\""),
    (JSON_RENDERER, "\"risk\""),
    (JSON_RENDERER, "\"score\""),
    (JSON_RENDERER, "\"errors_removed\""),
    (JSON_RENDERER, "\"new_errors\""),
    (JSON_RENDERER, "\"error_moved\""),
    (JSON_RENDERER, "\"behavior_changed\""),
    (COUNTERFACTUAL, "counterfactual_hypothesis_score"),
    (LSP, "disabled: bool"),
    (LSP, "disabled_reason: string"),
    (LSP, "has_new_error_outcome"),
    (LSP, "counterfactual recompilation created a new diagnostic"),
    (LSP, "replacement has not been verified by virtual recompilation"),
    (TESTS, "test_suggestion_quality_contract"),
    (TESTS, "test_parser_and_sema_suggestion_surfaces"),
    (TESTS, "test_multispan_and_lsp_suggestion_proof_contract"),
    (TESTS, "\\\"strategy\\\": \\\""),
    (TESTS, "\\\"proof\\\": \\\""),
    (TESTS, "\\\"score\\\": "),
    (MAKEFILE, "suggestion-quality"),
)

SCAN_PATHS = (
    ROOT / "src" / "vitte" / "compiler" / "diagnostics",
    ROOT / "src" / "vitte" / "compiler" / "analysis",
    ROOT / "src" / "vitte" / "compiler" / "middle",
    ROOT / "src" / "vitte" / "compiler" / "tests" / "diagnostics",
)


def scan_vague_terms() -> list[str]:
    failures: list[str] = []
    for base in SCAN_PATHS:
        if not base.exists():
            continue
        for path in base.rglob("*"):
            if path.suffix not in {".vit", ".snap"}:
                continue
            text = path.read_text(encoding="utf-8", errors="ignore")
            for pattern in VAGUE_PATTERNS:
                if pattern.search(text):
                    failures.append(f"{path.relative_to(ROOT)} contains vague suggestion text matching {pattern.pattern!r}")
    return failures


def main() -> int:
    failures: list[str] = []
    for path, needle in REQUIRED_NEEDLES:
        text = path.read_text(encoding="utf-8")
        if needle not in text:
            failures.append(f"{path.relative_to(ROOT)} missing {needle!r}")

    failures.extend(scan_vague_terms())

    covered = len(REQUIRED_NEEDLES) - sum(1 for failure in failures if " missing " in failure)
    total = len(REQUIRED_NEEDLES)
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
            print(f"[suggestion-quality][error] {failure}", file=sys.stderr)
        print(f"[suggestion-quality] coverage={covered}/{total} ({percent}%) report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1

    print(f"[suggestion-quality] OK coverage={covered}/{total} ({percent}%) report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
