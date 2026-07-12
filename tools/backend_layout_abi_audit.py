#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "backend_layout_abi_audit.json"


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        print(f"[backend-layout-abi][error] missing file: {rel}", file=sys.stderr)
        raise SystemExit(1)
    return path.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    required: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/backend/target/layout.vit", "form TypeLayout", "type layouts must be explicit"),
        ("src/vitte/compiler/backend/target/layout.vit", "field_offsets: [u64]", "aggregate layouts must expose field offsets"),
        ("src/vitte/compiler/backend/target/layout.vit", "proc type_layout_for_name(", "primitive and pointer layouts must share one classifier"),
        ("src/vitte/compiler/backend/target/layout.vit", "char_size: 4", "Vitte char must match its Unicode scalar representation"),
        ("src/vitte/compiler/backend/target/layout.vit", "-i128:128-n32:64-S128", "AArch64 LLVM layout must be explicit"),
        ("src/vitte/compiler/backend/target/calling_convention.vit", "proc calling_convention_x86_64_sysv(", "SysV register convention must be modeled"),
        ("src/vitte/compiler/backend/target/calling_convention.vit", "proc calling_convention_x86_64_windows(", "Win64 register convention must be modeled"),
        ("src/vitte/compiler/backend/target/calling_convention.vit", "proc calling_convention_aapcs64(", "AAPCS64 register convention must be modeled"),
        ("src/vitte/compiler/backend/target/calling_convention.vit", "proc abi_argument_locations_for_return(", "hidden aggregate returns must shift x86 arguments"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", "proc frame_layout_for_function(", "machine frames must derive from typed slots"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", "machine_prologue_x86_64_with_stack(frame.size)", "x86 frames must reserve aligned stack storage"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", "emit_call_argument_lines(frame, instruction, convention, locations)", "x86 calls must materialize ABI argument locations"),
        ("src/vitte/compiler/backends/llvm_bindings/mod.vit", "proc llvm_alignment_for_type(", "LLVM allocas must consume target alignment"),
        ("src/vitte/compiler/backend/c/architecture.vit", "calling_convention_for_triple(triple)", "C ABI metadata must consume the target convention"),
        ("src/vitte/compiler/backend/c/architecture.vit", "pointer_width: layout.pointer_size * 8", "C ABI metadata must consume the target layout"),
        ("src/vitte/compiler/tests/codegen_tests.vit", "test_target_layout_has_canonical_field_offsets", "field offsets must be tested"),
        ("src/vitte/compiler/tests/codegen_tests.vit", "test_register_conventions_classify_arguments_and_returns", "register and stack classification must be tested"),
        ("src/vitte/compiler/tests/codegen_tests.vit", "test_x86_64_codegen_uses_aligned_frames_and_abi_registers", "machine ABI emission must be tested"),
        ("src/vitte/compiler/backend/target/README.md", "calling_convention.vit", "the canonical layout and ABI ownership must be documented"),
    ]
    forbidden: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/backends/llvm_bindings/mod.vit", "align 8", "LLVM alignment must not be hard-coded"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", "machine_prologue_x86_64()", "x86 codegen must not rely on the red zone for its frame"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", 'let lines: [string] = metadata + ["  call " + callee]', "calls must not skip ABI argument lowering"),
        ("src/vitte/compiler/backend/c/architecture.vit", 'calling_convention: "sysv64"', "C profiles must not duplicate the register convention"),
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
        "schema": "vitte.compiler.backend_layout_abi_audit",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "results": results,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[backend-layout-abi] status={payload['status']} report={REPORT.relative_to(ROOT)}")
    for result in results:
        print(f"[backend-layout-abi][check] {result['status']} {result['file']} needle={result['needle']}")
    for failure in failures:
        print(f"[backend-layout-abi][error] {failure}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
