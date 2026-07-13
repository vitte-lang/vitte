#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "backend_object_structure_audit.json"


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        print(f"[backend-object-structure][error] missing file: {rel}", file=sys.stderr)
        raise SystemExit(1)
    return path.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    required: list[tuple[str, str, str]] = [
        ("src/vitte/stdlib/io/host_runtime.vitl", "intrinsic vitte_host_verify_native_object(", "native object verification must cross the host ABI"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", "int32_t vitte_host_verify_native_object(", "the runtime must verify native objects"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", "vitte_read_u16_le(data + 16) != 1", "the verifier must require ET_REL"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", "vitte_read_u16_le(data + 18) != expected_machine", "the verifier must check the target machine"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", 'strcmp(name, ".text") == 0', "the verifier must require executable text"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", "saw_symbol_table", "the verifier must require a symbol table"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", "relocation_count += size / entry_size", "the verifier must count relocations"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", "strcmp(name, expected_c) == 0", "the verifier must resolve the expected entry symbol"),
        ("src/vitte/compiler/backend/native_bridge.vit", "host_verify_native_object(object_path, target, entry_symbol, require_relocations, debug_enabled)", "the bridge must reject unverified objects"),
        ("src/vitte/compiler/backend/native_bridge.vit", "proc native_object_verification_message(", "object verification failures must be structured"),
        ("src/vitte/compiler/driver/compile.vit", "proc ir_unit_requires_native_relocations(", "the driver must derive relocation requirements from calls"),
        ("src/vitte/compiler/driver/compile.vit", "ir_module_symbol_is_external(module0, instruction.text)", "only actual external calls must require relocations"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", '".type " + symbol + ",@function"', "ELF function symbols must have a function type"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", '".size " + symbol + ", .-" + symbol', "ELF function symbols must have a size"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", "fn0.linkage == IrLinkage.Exported", "exported functions must be globally visible"),
        ("src/vitte/compiler/tests/codegen_tests.vit", "test_native_object_verification_errors_are_structured", "verification diagnostics must be tested"),
        ("tools/native_object_determinism_test.py", '"relocation_targets_external"', "real relocation targets must be tested"),
        ("target/reports/native_object_determinism.json", '"required_sections_present": true', "the real object section gate must pass"),
        ("target/reports/native_object_determinism.json", '"defined_symbol_present": true', "the real defined symbol gate must pass"),
        ("target/reports/native_object_determinism.json", '"undefined_symbol_present": true', "the real undefined symbol gate must pass"),
        ("target/reports/native_object_determinism.json", '"relocation_targets_external": true', "the real relocation gate must pass"),
        ("docs/compiler/native_asm_backend.md", "relocations lorsqu'un appel externe", "object structure verification must be documented"),
    ]
    forbidden: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/backend/native_bridge.vit", '"deterministic native assembly object materialized", "",', "an emitted object must not be accepted before verification"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", "if fn0.is_entry {", "global visibility must not exclude exported non-entry functions"),
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
        "schema": "vitte.compiler.backend_object_structure_audit",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "results": results,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[backend-object-structure] status={payload['status']} report={REPORT.relative_to(ROOT)}")
    for result in results:
        print(f"[backend-object-structure][check] {result['status']} {result['file']} needle={result['needle']}")
    for failure in failures:
        print(f"[backend-object-structure][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
