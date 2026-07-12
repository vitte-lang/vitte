#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "backend_native_object_audit.json"


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        print(f"[backend-native-object][error] missing file: {rel}", file=sys.stderr)
        raise SystemExit(1)
    return path.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    required: list[tuple[str, str, str]] = [
        ("src/vitte/stdlib/io/host_runtime.vitl", "intrinsic vitte_host_emit_assembly_object(", "the standard library must expose native assembly materialization"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.h", "vitte_host_emit_assembly_object", "the runtime ABI header must expose native assembly materialization"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", "int32_t vitte_host_emit_assembly_object(", "the runtime must implement native assembly materialization"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", 'argv[4] = "assembler";', "clang must be invoked in assembler mode"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", 'setenv("SOURCE_DATE_EPOCH", "0", 1);', "child tool invocations must have a reproducible epoch"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", "remove(object_c);", "stale objects must be removed before tool invocation"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", "remove(assembly_path);", "temporary assembly files must be removed"),
        ("src/vitte/compiler/backend/native_bridge.vit", "proc emit_native_object_from_assembly(", "the compiler bridge must materialize assembly objects"),
        ("src/vitte/compiler/backend/native_bridge.vit", 'give "x86_64-unknown-linux-gnu";', "the x86_64 target must be canonicalized for clang"),
        ("src/vitte/compiler/driver/compile.vit", "emit_native_object_from_assembly(backend0.codegen.assembly", "the native driver must route assembly backends to the assembler"),
        ("src/vitte/compiler/backend/codegen/mod.vit", 'object_format: "elf64-relocatable"', "codegen must advertise a real relocatable object format"),
        ("src/vitte/compiler/backend/codegen/instruction_select.vit", '.section .note.GNU-stack', "ELF assembly must mark a non-executable stack"),
        ("src/vitte/compiler/tests/codegen_tests.vit", "test_native_assembly_object_bridge_is_targeted", "the bridge command contract must be tested"),
        ("tools/native_object_determinism_test.py", '"objects_identical"', "byte-for-byte determinism must have a real toolchain test"),
        ("target/reports/native_object_determinism.json", '"status": "pass"', "the determinism gate must have a passing report"),
        ("docs/compiler/native_asm_backend.md", "native-object-determinism-gate", "native object determinism must be documented"),
    ]
    forbidden: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/backend/codegen/mod.vit", 'object_format: "elf64-relocatable-model"', "codegen must not advertise a model as the emitted object"),
        ("src/vitte/compiler/backend/native_bridge.vit", "host_system(", "native object materialization must not invoke a shell"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", 'argv[0] = "sh";', "the runtime must not assemble through a shell"),
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
        "schema": "vitte.compiler.backend_native_object_audit",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "results": results,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[backend-native-object] status={payload['status']} report={REPORT.relative_to(ROOT)}")
    for result in results:
        print(f"[backend-native-object][check] {result['status']} {result['file']} needle={result['needle']}")
    for failure in failures:
        print(f"[backend-native-object][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
