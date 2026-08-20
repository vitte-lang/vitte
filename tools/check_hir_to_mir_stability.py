#!/usr/bin/env python3
"""Check that HIR lowering preserves semantic data in production MIR."""

from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
LOWER = ROOT / "src/vitte/compiler/middle/lower/hir_to_mir.vit"
MIR = ROOT / "src/vitte/compiler/middle/mir/mir.vit"
PIPELINE = ROOT / "src/vitte/compiler/middle/pipeline.vit"
REPORT_DIR = ROOT / "target/reports"
REPORT = REPORT_DIR / "hir_to_mir_stability.json"
MARKDOWN = REPORT_DIR / "hir_to_mir_stability.md"

REQUIRED_LOWERING = (
    "proc mir_binary_operator_from_text(operator: string) -> MirBinaryOperator",
    "proc mir_unary_operator_from_text(operator: string) -> MirUnaryOperator",
    "proc mir_place_from_name(name: string) -> MirPlace",
    "proc mir_operand_from_expr(expr: HirExpr) -> MirOperand",
    "proc mir_rvalue_from_expr(expr: HirExpr, items: [HirItem]) -> MirRvalue",
    "proc mir_statement_from_stmt(stmt: HirStmt, items: [HirItem]) -> MirStatement",
    "proc build_stmt_sequence(hir_stmts: [HirStmt], items: [HirItem], fallthrough_block: u64, next_block_id: u64) -> MirSequence",
    "proc lower_hir_to_mir_templates(hir: HirUnit) -> MirUnit",
    "proc lower_hir_to_mir(hir: HirUnit) -> MirUnit",
    "generic_arguments: if kind == MirRvalueKind.Call { expr.generic_arguments } else { [] },",
    "span: expr.span",
    "mir_terminator_branch_at",
    "mir_terminator_goto_at",
    "monomorphize_mir(lower_hir_to_mir_templates(hir))",
)

REQUIRED_MIR = (
    "form MirRvalue {",
    "binary_operator: MirBinaryOperator",
    "unary_operator: MirUnaryOperator",
    "generic_arguments: [string]",
    "form MirStatement {",
    "span: Span",
    "form MirBlock {",
    "predecessors: [u64]",
)

REQUIRED_PIPELINE = (
    "let mir0: MirUnit = lower_hir_to_mir(typed_hir);",
    "let mir1: MirUnit = validate_mir(mir0);",
)


def main() -> int:
    lowering = LOWER.read_text(encoding="utf-8")
    mir = MIR.read_text(encoding="utf-8")
    pipeline = PIPELINE.read_text(encoding="utf-8")
    failures = [f"lowering contract missing: {needle}" for needle in REQUIRED_LOWERING if needle not in lowering]
    failures.extend(f"MIR contract missing: {needle}" for needle in REQUIRED_MIR if needle not in mir)
    failures.extend(f"pipeline contract missing: {needle}" for needle in REQUIRED_PIPELINE if needle not in pipeline)
    if "MirRvalueKind.Array" not in lowering or "MirRvalueKind.Tuple" not in lowering or "MirRvalueKind.Struct" not in lowering:
        failures.append("aggregate HIR expressions are not all represented in MIR")

    payload = {
        "schema": "vitte.compiler.hir-to-mir-stability",
        "status": "error" if failures else "ok",
        "lowering_contracts": len(REQUIRED_LOWERING),
        "mir_contracts": len(REQUIRED_MIR),
        "pipeline_contracts": len(REQUIRED_PIPELINE),
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    MARKDOWN.write_text(
        "# HIR to MIR stability\n\n"
        f"- lowering contracts: {payload['lowering_contracts']}\n"
        f"- MIR contracts: {payload['mir_contracts']}\n"
        f"- pipeline contracts: {payload['pipeline_contracts']}\n"
        f"- status: {payload['status']}\n",
        encoding="utf-8",
    )
    if failures:
        for failure in failures:
            print(f"[hir-to-mir-stability][error] {failure}", file=sys.stderr)
        return 1
    print(
        "[hir-to-mir-stability] "
        f"lowering={payload['lowering_contracts']} mir={payload['mir_contracts']} "
        f"pipeline={payload['pipeline_contracts']} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
