#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "backend_value_lowering_audit.json"


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        print(f"[backend-values][error] missing file: {rel}", file=sys.stderr)
        raise SystemExit(1)
    return path.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    required: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/middle/lower/hir_to_mir.vit", "set kind = MirRvalueKind.Array;", "arrays must survive HIR to MIR"),
        ("src/vitte/compiler/middle/lower/hir_to_mir.vit", "set kind = MirRvalueKind.Tuple;", "tuples must survive HIR to MIR"),
        ("src/vitte/compiler/backend/ir/ir.vit", "Aggregate,", "backend IR must represent aggregate construction"),
        ("src/vitte/compiler/backend/ir/ir.vit", "Discriminant,", "backend IR must represent enum tag reads"),
        ("src/vitte/compiler/backend/ir/ir.vit", "proc verify_aggregate_instruction(", "aggregate shape must be verified"),
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "proc materialized_return_instruction(", "complex return expressions must be materialized"),
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "return_instruction_from_value", "materialized values must feed ReturnValue"),
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "IrLinkage.External", "extern procedures must remain declarations"),
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "module_add_external_symbol(built, lowered_fn.name)", "extern declarations must reach symbol metadata"),
        ("src/vitte/compiler/backend/c/lowering.vit", "proc lower_struct_aggregate_text(", "C backend must construct structs and unions"),
        ("src/vitte/compiler/backend/c/lowering.vit", "proc lower_erased_aggregate_text(", "C backend must construct arrays and tuples"),
        ("src/vitte/compiler/backend/c/lowering.vit", "returned and block.terminator.kind", "C backend must suppress duplicate returns"),
        ("src/vitte/compiler/backend/c/lowering.vit", "declaration_only: declaration_only", "C lowering must classify external declarations"),
        ("src/vitte/compiler/backend/c/emitter.vit", "typedef struct VitteAggregate VitteAggregate", "C aggregate representation must be emitted"),
        ("src/vitte/compiler/backend/c/emitter.vit", "if not unit.functions[i].declaration_only", "external C procedures must not receive bodies"),
        ("src/vitte/compiler/backends/llvm_bindings/mod.vit", "%VitteAggregate = type { i64, i8* }", "LLVM aggregate representation must be emitted"),
        ("src/vitte/compiler/backends/llvm_bindings/mod.vit", "proc emit_llvm_nominal_type(", "LLVM must emit nominal types"),
        ("src/vitte/compiler/backends/llvm_bindings/mod.vit", "proc llvm_variant_ctor_lines(", "LLVM must construct enum variants"),
        ("src/vitte/compiler/backends/llvm_bindings/mod.vit", "proc llvm_return_value_lines(", "LLVM returns must be typed"),
        ("src/vitte/compiler/backends/llvm_bindings/mod.vit", 'give ["declare " + return_ty', "LLVM extern procedures must use declare"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", '".extern " + sanitize_machine_symbol(machine_call_target_text', "machine backend must preserve canonical extern symbols"),
        ("src/vitte/compiler/tests/codegen_tests.vit", "test_backend_lowers_calls_returns_and_aggregates_concretely", "cross-backend value lowering must be tested"),
        ("src/vitte/compiler/tests/c_backend_tests.vit", "test_c_backend_lowers_struct_tuple_and_array_values", "C aggregate lowering must be tested"),
        ("src/vitte/compiler/tests/c_backend_tests.vit", "test_c_backend_emits_external_calls_as_declarations_only", "C external declarations must be tested"),
    ]
    forbidden: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/backend/c/lowering.vit", 'give "{0}";', "C aggregate lowering must not use a zero stub"),
        ("src/vitte/compiler/backends/llvm_bindings/mod.vit", '" = call i64 @" + instruction.text', "LLVM calls must not hard-code i64 or raw symbols"),
        ("src/vitte/compiler/backends/llvm_bindings/mod.vit", 'give ["  ret i64 " + llvm_operand_text', "LLVM returns must not hard-code i64"),
        ("src/vitte/compiler/backends/llvm_bindings/mod.vit", 'give ["  br i1 1,', "LLVM branches must use their real condition"),
    ]

    results: list[dict[str, str]] = []
    failures: list[str] = []
    for rel, needle, reason in required:
        present = needle in read(rel)
        results.append({"file": rel, "needle": needle, "reason": reason, "status": "present" if present else "missing"})
        if not present:
            failures.append(f"{rel}: missing `{needle}`")
    for rel, needle, reason in forbidden:
        absent = needle not in read(rel)
        results.append({"file": rel, "needle": needle, "reason": reason, "status": "absent" if absent else "forbidden-present"})
        if not absent:
            failures.append(f"{rel}: forbidden `{needle}`")

    payload = {
        "schema": "vitte.compiler.backend_value_lowering_audit",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "results": results,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[backend-values] status={payload['status']} report={REPORT.relative_to(ROOT)}")
    for result in results:
        print(f"[backend-values][check] {result['status']} {result['file']} needle={result['needle']}")
    for failure in failures:
        print(f"[backend-values][error] {failure}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
