#!/usr/bin/env python3
"""Check the production HIR validation boundary and its stable contracts."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HIR = ROOT / "src/vitte/compiler/middle/hir"
VALIDATE = HIR / "validate.vit"
PIPELINE = ROOT / "src/vitte/compiler/middle/pipeline.vit"
REPORT_DIR = ROOT / "target/reports"
REPORT = REPORT_DIR / "hir_validation_stability.json"
MARKDOWN = REPORT_DIR / "hir_validation_stability.md"


REQUIRED_PROCS = (
    "validate_expr",
    "validate_stmt",
    "validate_param",
    "validate_member",
    "validate_item",
    "validate_module",
    "validate_hir",
    "hir_validation_summary",
    "hir_validate_selftest",
)

REQUIRED_CODES = (
    "HIRV0001",
    "HIRV0002",
    "HIRV0003",
    "HIRV0100",
    "HIRV0150",
    "HIRV0170",
    "HIRV0200",
    "HIRV0300",
    "HIRV0400",
    "HIRV0403",
)

REQUIRED_FORMS = (
    "form HirValidationResult {",
    "diagnostics: [SemanticDiagnostic]",
    "checked_items: u64",
    "checked_stmts: u64",
    "checked_exprs: u64",
    "valid: bool",
)

REQUIRED_PIPELINE_CONTRACTS = (
    "let hir1: HirUnit = validate_hir(hir);",
    "let sema_result: SemaResult = run_sema_hir(hir1);",
    "let hir0: HirUnit = lower_ast_to_hir(frontend);",
    "let hir1: HirUnit = validate_hir(hir0);",
)


def main() -> int:
    failures: list[str] = []
    validate_text = VALIDATE.read_text(encoding="utf-8")
    pipeline_text = PIPELINE.read_text(encoding="utf-8")

    for needle in REQUIRED_PROCS:
        if not re.search(rf"\bproc\s+{re.escape(needle)}\s*\(", validate_text):
            failures.append(f"missing validation procedure: {needle}")
    for needle in REQUIRED_CODES:
        if needle not in validate_text:
            failures.append(f"missing stable validation diagnostic code: {needle}")
    for needle in REQUIRED_FORMS:
        if needle not in validate_text:
            failures.append(f"missing validation result contract: {needle}")
    for needle in REQUIRED_PIPELINE_CONTRACTS:
        if needle not in pipeline_text:
            failures.append(f"missing pipeline validation boundary: {needle}")

    if "if hir.analyzed {" not in validate_text or "give hir" not in validate_text:
        failures.append("validate_hir must be idempotent for an already analyzed unit")
    if "set diagnostics = diagnostics + validate_module(hir.module)" not in validate_text:
        failures.append("validate_hir must validate the complete module tree")
    if "hir.item_count != len(hir.module.items)" not in validate_text:
        failures.append("validate_hir must check item count consistency")
    if "hir.symbol_count != hir.module.symbol_count" not in validate_text:
        failures.append("validate_hir must check symbol count consistency")
    if "valid: result.valid" not in validate_text:
        failures.append("validated HIR must publish the validation result")

    payload = {
        "schema": "vitte.compiler.hir-validation-stability",
        "status": "fail" if failures else "pass",
        "validation_procedures": len(REQUIRED_PROCS),
        "diagnostic_codes": len(REQUIRED_CODES),
        "pipeline_contracts": len(REQUIRED_PIPELINE_CONTRACTS),
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MARKDOWN.write_text(
        "# HIR validation stability\n\n"
        f"- status: {payload['status']}\n"
        f"- validation procedures: {payload['validation_procedures']}\n"
        f"- diagnostic codes: {payload['diagnostic_codes']}\n"
        f"- pipeline contracts: {payload['pipeline_contracts']}\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[hir-validation-stability][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[hir-validation-stability] "
        f"procedures={payload['validation_procedures']} "
        f"codes={payload['diagnostic_codes']} "
        f"pipeline_contracts={payload['pipeline_contracts']} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
