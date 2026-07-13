#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "backend_debug_unwind_audit.json"


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        print(f"[backend-debug-unwind][error] missing file: {rel}", file=sys.stderr)
        raise SystemExit(1)
    return path.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    required: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/backend/codegen/machine.vit", '".cfi_def_cfa_offset 16"', "the x86 frame must define its CFA offset"),
        ("src/vitte/compiler/backend/codegen/machine.vit", '".cfi_offset %rbp, -16"', "the saved frame pointer must be described"),
        ("src/vitte/compiler/backend/codegen/machine.vit", '".cfi_def_cfa %rsp, 8"', "the epilogue must restore the caller CFA"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", '"  .cfi_startproc"', "every function must begin unwind metadata"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", '"  .cfi_endproc"', "every function must end unwind metadata"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", "proc debug_location_lines(", "source locations must lower to assembler locations"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", "proc debug_info_section_lines(", "a DWARF compilation unit must be emitted"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", '".section .debug_info', "DWARF debug info must have a real section"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", '".section .debug_abbrev', "DWARF abbreviations must have a real section"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", '".section .debug_str', "DWARF strings must have a real section"),
        ("src/vitte/compiler/backend/codegen/mod.vit", "proc run_codegen_x86_64_with_profile(", "native codegen must consume the debug profile"),
        ("src/vitte/compiler/backend/pipeline.vit", "run_codegen_x86_64_with_profile(ir1, profile_name, debug_enabled)", "the backend pipeline must propagate debug mode"),
        ("src/vitte/compiler/driver/compile.vit", "run_codegen_x86_64_with_profile(ir0, config.profile_name, config.emit_debug_sections)", "multi-module codegen must propagate debug mode"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", 'argv[arg_index++] = "-g";', "debug object materialization must enable clang debug handling"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", 'strcmp(name, ".eh_frame") == 0', "object verification must require unwind data"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", 'strcmp(name, ".debug_info") == 0', "object verification must inspect debug info"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", 'strcmp(name, ".debug_line") == 0', "object verification must inspect line tables"),
        ("src/vitte/compiler/backend/native_bridge.vit", "object has no stack unwind information", "missing unwind data must be diagnosed"),
        ("src/vitte/compiler/backend/native_bridge.vit", "object has incomplete debug information", "missing debug data must be diagnosed"),
        ("src/vitte/compiler/tests/codegen_tests.vit", "test_native_debug_and_unwind_emission_follow_profile", "debug and unwind profiles must be tested"),
        ("target/reports/native_object_determinism.json", '"debug_sections_present": true', "real debug sections must pass the object gate"),
        ("target/reports/native_object_determinism.json", '"unwind_sections_present": true', "real unwind sections must pass the object gate"),
        ("target/reports/native_object_determinism.json", '"release_debug_sections_absent": true', "release objects must omit debug sections"),
        ("target/reports/native_object_determinism.json", '"release_unwind_present": true', "release objects must retain unwind data"),
        ("target/reports/native_object_determinism.json", '"dwarf_verifier_passed": true', "emitted DWARF must pass an external verifier"),
        ("docs/compiler/native_asm_backend.md", "DWARF", "debug and unwind behavior must be documented"),
    ]
    forbidden: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/backend/pipeline.vit", "run_codegen_x86_64(ir1)", "single-module codegen must not discard debug mode"),
        ("src/vitte/compiler/driver/compile.vit", "run_codegen_x86_64(ir0)", "multi-module codegen must not discard debug mode"),
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
        "schema": "vitte.compiler.backend_debug_unwind_audit",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "results": results,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[backend-debug-unwind] status={payload['status']} report={REPORT.relative_to(ROOT)}")
    for result in results:
        print(f"[backend-debug-unwind][check] {result['status']} {result['file']} needle={result['needle']}")
    for failure in failures:
        print(f"[backend-debug-unwind][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
