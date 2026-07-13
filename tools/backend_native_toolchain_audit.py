#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "backend_native_toolchain_audit.json"


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        print(f"[backend-native-toolchain][error] missing file: {rel}", file=sys.stderr)
        raise SystemExit(1)
    return path.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    required: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/backend/native_toolchain.vit", "form NativeToolchain {", "native tools must share one configuration contract"),
        ("src/vitte/compiler/backend/native_toolchain.vit", "form NativeToolCommand {", "tool invocations must have a structured command plan"),
        ("src/vitte/compiler/backend/native_toolchain.vit", "proc plan_llvm_object_command(", "LLVM object compilation must be planned centrally"),
        ("src/vitte/compiler/backend/native_toolchain.vit", "proc plan_assembly_object_command(", "assembly must be planned centrally"),
        ("src/vitte/compiler/backend/native_toolchain.vit", "proc plan_link_executable_command(", "linking must be planned centrally"),
        ("src/vitte/compiler/backend/native_bridge.vit", "proc emit_native_object_from_llvm_ir_with_toolchain(", "the LLVM bridge must accept an explicit toolchain"),
        ("src/vitte/compiler/backend/native_bridge.vit", "proc emit_native_object_from_assembly_with_toolchain(", "the assembler bridge must accept an explicit toolchain"),
        ("src/vitte/compiler/backend/native_bridge.vit", "proc link_native_executable_with_toolchain(", "the linker bridge must accept an explicit toolchain"),
        ("src/vitte/compiler/backend/native_bridge.vit", "host_emit_llvm_object(ir_text, command.tool, object_path)", "the compiler path must cross the host ABI"),
        ("src/vitte/compiler/backend/native_bridge.vit", "host_emit_assembly_object(assembly_text, command.tool, target, object_path, debug_enabled)", "the assembler path must cross the host ABI"),
        ("src/vitte/compiler/backend/native_bridge.vit", "host_link_executable(command.tool, object_path, toolchain.runtime_source_path, toolchain.runtime_include_path, executable_path)", "the linker plan must cross the host ABI"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", "argv[0] = compiler_c;", "runtime execution must use the configured compiler"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", "argv[arg_index++] = assembler_c;", "runtime execution must use the configured assembler"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", "argv[0] = linker_c;", "runtime execution must use the configured linker"),
        ("src/vitte/compiler/tests/codegen_tests.vit", "test_native_toolchain_encapsulates_compiler_assembler_and_linker", "custom tool paths and plans must be tested"),
        ("docs/compiler/native_asm_backend.md", "NativeToolchain", "the native tool boundary must be documented"),
    ]
    forbidden: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", 'argv[0] = "clang";', "the runtime must not hard-code the compiler"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", 'argv[arg_index++] = "clang";', "the runtime must not hard-code the assembler"),
        ("src/vitte/compiler/backends/runtime_c/vitte_runtime.c", 'argv[0] = "cc";', "the runtime must not hard-code the linker"),
        ("src/vitte/compiler/backend/native_bridge.vit", 'give "clang -target ', "the bridge must use structured assembler plans"),
        ("src/vitte/compiler/backend/native_bridge.vit", 'give "cc " + object_path', "the bridge must use structured linker plans"),
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
        "schema": "vitte.compiler.backend_native_toolchain_audit",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "results": results,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[backend-native-toolchain] status={payload['status']} report={REPORT.relative_to(ROOT)}")
    for result in results:
        print(f"[backend-native-toolchain][check] {result['status']} {result['file']} needle={result['needle']}")
    for failure in failures:
        print(f"[backend-native-toolchain][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
