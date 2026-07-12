#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "c_abi_contract_audit.json"


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        print(f"[c-abi-contract][error] missing file: {rel}", file=sys.stderr)
        raise SystemExit(1)
    return path.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    required: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/backend/c/abi.vit", 'const C_ABI_NAME: string = "vitte-c-abi-v1"', "C ABI must have a stable name"),
        ("src/vitte/compiler/backend/c/abi.vit", "const C_ABI_SCHEMA_VERSION: u64 = 1", "C ABI must be versioned"),
        ("src/vitte/compiler/backend/c/abi.vit", "pick CAbiPassMode", "parameter passing modes must be explicit"),
        ("src/vitte/compiler/backend/c/abi.vit", "Indirect,", "borrowed values must have an indirect mode"),
        ("src/vitte/compiler/backend/c/abi.vit", "Variadic,", "variadic values must have an explicit mode"),
        ("src/vitte/compiler/backend/c/abi.vit", 'immutable_borrow_representation: "const-pointer"', "immutable borrow representation must be stable"),
        ("src/vitte/compiler/backend/c/abi.vit", 'mutable_borrow_representation: "pointer"', "mutable borrow representation must be stable"),
        ("src/vitte/compiler/backend/c/abi.vit", "proc c_abi_params_valid(", "signature parameter validation must exist"),
        ("src/vitte/compiler/backend/c/abi.vit", "proc c_abi_name_supported(", "calling convention compatibility must be checked"),
        ("src/vitte/compiler/backend/c/abi.vit", "#define VITTE_ABI_VITTE VITTE_ABI_C", "generated C must expose stable ABI macros"),
        ("src/vitte/compiler/backend/c/architecture.vit", 'abi_name: "vitte-c-abi-v1"', "target profile must select the canonical ABI"),
        ("src/vitte/compiler/backend/c/architecture.vit", 'calling_convention: "sysv64"', "x86_64 Linux calling convention must be explicit"),
        ("src/vitte/compiler/backend/ir/ir.vit", "abi: string", "canonical IR functions must carry ABI metadata"),
        ("src/vitte/compiler/backend/ir/ir.vit", "proc function_with_abi(", "canonical IR must expose ABI-preserving construction"),
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "ir_function_abi_from_hir", "HIR ABI metadata must reach backend IR"),
        ("src/vitte/compiler/middle/lower/mir_to_ir.vit", "abi: lowered.abi", "IR signature construction must preserve ABI metadata"),
        ("src/vitte/compiler/backend/c/lowering.vit", "let function_abi: CAbiFunction = c_abi_function(", "C lowering must resolve signatures through the ABI contract"),
        ("src/vitte/compiler/backend/c/lowering.vit", "return_type: function_abi.return_value.c_type", "C return lowering must use the ABI result"),
        ("src/vitte/compiler/backend/c/lowering.vit", 'c_error("CBACKEND_E_ABI"', "unsupported ABI must be diagnosed"),
        ("src/vitte/compiler/backend/c/emitter.vit", "function0.abi.declaration_macro", "every emitted function must carry an ABI macro"),
        ("src/vitte/compiler/backend/c/emitter.vit", "c_abi_macro_for_name(nominal.abi)", "indirect method calls must carry the same ABI convention"),
        ("src/vitte/compiler/backend/c/emitter.vit", "c_abi_header_prelude()", "generated headers must define ABI macros"),
        ("src/vitte/compiler/tests/c_backend_tests.vit", "test_c_abi_v1_contract_is_explicit_and_checked", "ABI rules must have contract tests"),
        ("src/vitte/compiler/tests/c_backend_tests.vit", "test_c_abi_is_preserved_from_hir_to_canonical_ir", "ABI propagation must have a regression test"),
        ("src/vitte/compiler/backend/c/README.md", "vitte-c-abi-v1", "ABI contract must stay documented"),
    ]

    results: list[dict[str, str]] = []
    failures: list[str] = []
    for rel, needle, reason in required:
        present = needle in read(rel)
        results.append(
            {
                "file": rel,
                "needle": needle,
                "reason": reason,
                "status": "present" if present else "missing",
            }
        )
        if not present:
            failures.append(f"{rel}: missing `{needle}`")

    lowering = read("src/vitte/compiler/backend/c/lowering.vit")
    forbidden = [
        (
            "return_type: c_type_for_vitte_type(profile, function0.return_type)",
            "function return types must not bypass ABI classification",
        ),
        (
            'give function0.return_type + " " + function0.c_name',
            "function declarations must not bypass the ABI macro",
        ),
    ]
    emitter = read("src/vitte/compiler/backend/c/emitter.vit")
    for needle, reason in forbidden:
        source = lowering if needle.startswith("return_type:") else emitter
        absent = needle not in source
        results.append(
            {
                "file": "src/vitte/compiler/backend/c",
                "needle": needle,
                "reason": reason,
                "status": "absent" if absent else "forbidden-present",
            }
        )
        if not absent:
            failures.append(f"src/vitte/compiler/backend/c: forbidden `{needle}`")

    payload = {
        "schema": "vitte.compiler.c_abi_contract_audit",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "abi_name": "vitte-c-abi-v1",
        "abi_schema_version": 1,
        "results": results,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(
        json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    print(f"[c-abi-contract] status={payload['status']} report={REPORT.relative_to(ROOT)}")
    for result in results:
        print(
            f"[c-abi-contract][check] {result['status']} "
            f"{result['file']} needle={result['needle']}"
        )
    for failure in failures:
        print(f"[c-abi-contract][error] {failure}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
