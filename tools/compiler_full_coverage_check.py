#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "compiler_full_coverage.json"
TARGET_ABI = ROOT / "data" / "ffi" / "abi" / "target_abi_v1.json"


REQUIRED_TARGETS = {
    "x86_64-unknown-linux-gnu": {"architecture": "x86_64", "pointer_width": 64, "object_format": "elf64"},
    "aarch64-unknown-linux-gnu": {"architecture": "aarch64", "pointer_width": 64, "object_format": "elf64"},
    "riscv64-unknown-linux-gnu": {"architecture": "riscv64", "pointer_width": 64, "object_format": "elf64"},
    "i386-unknown-linux-gnu": {"architecture": "i386", "pointer_width": 32, "object_format": "elf32"},
}


REQUIREMENTS = [
    {
        "id": 11,
        "name": "lexer_parser_recovery",
        "families": ["lexer", "parser", "recovery"],
        "evidence": [
            "tools/check_lexer_diagnostics.py",
            "tools/parser_sync_coverage_report.py",
            "tools/parser_precedence_property_test.py",
            "tools/parser_bootstrap_surface_test.py",
            "tests/frontend/frontend_error.vit",
            "tests/golden/frontend/CONFIG.json",
        ],
    },
    {
        "id": 12,
        "name": "hir_variants_real_tests",
        "families": ["hir", "variants", "real-tests"],
        "evidence": [
            "tools/hir_coverage_check.py",
            "tools/hir_fixture_snapshots.py",
            "src/vitte/compiler/tests/hir_tests.vit",
            "src/vitte/compiler/tests/hir_coverage_manifest.json",
        ],
    },
    {
        "id": 13,
        "name": "sema_import_visibility_export_shadow_collision",
        "families": ["imports", "visibility", "exports", "shadowing", "collisions"],
        "evidence": [
            "tools/sema_coverage_check.py",
            "tools/sema_fixture_snapshots.py",
            "src/vitte/compiler/tests/sema_tests.vit",
            "tests/pkg/ok_cross_package_imports.vit",
            "tests/pkg/bad_symbol_not_exported.vit",
            "tests/pkg/bad_import_cycle.vit",
        ],
    },
    {
        "id": 14,
        "name": "typeck_inference_generics_traits_coercions_casts",
        "families": ["inference", "generics", "traits", "coercions", "casts"],
        "evidence": [
            "tools/typeck_coverage_check.py",
            "tools/typeck_fixture_check.py",
            "tools/typeck_differential_test.py",
            "tools/typeck_fuzz_test.py",
            "src/vitte/compiler/tests/typeck_tests.vit",
            "tests/type_system/inference_positive.vit",
            "tests/type_system/generics_positive.vit",
            "tests/type_system/traits_positive.vit",
            "tests/type_system/call_result_cast_positive.vit",
        ],
    },
    {
        "id": 15,
        "name": "borrowck_move_loan_lifetime_partial_closure_async",
        "families": ["move", "loan", "lifetime", "partial-move", "closures", "async"],
        "evidence": [
            "tools/borrowck_coverage_check.py",
            "tools/borrowck_coverage_check.py",
            "src/vitte/compiler/tests/borrowck_tests.vit",
            "src/vitte/compiler/analysis/borrowck/mod.vit",
            "src/vitte/compiler/middle/borrow/checks.vit",
        ],
    },
    {
        "id": 16,
        "name": "mir_cfg_temporaries_validation_passes_monomorphization",
        "families": ["cfg", "temporaries", "validation", "passes", "monomorphization"],
        "evidence": [
            "tools/mir_coverage_check.py",
            "tools/mir_opt/run_checks.py",
            "src/vitte/compiler/tests/mir_tests.vit",
            "src/vitte/compiler/tests/mir_coverage_manifest.json",
            "tests/mir/valid/control_flow.vit",
            "tests/mir/invalid/bad_cfg.vit",
            "src/vitte/compiler/middle/mir/monomorphize.vit",
        ],
    },
    {
        "id": 17,
        "name": "ir_backend_contract_lowering_snapshots",
        "families": ["backend-contract", "lowering", "snapshots"],
        "evidence": [
            "tools/ir_coverage_check.py",
            "tools/backend_ir_contract_audit.py",
            "tools/backend_value_lowering_audit.py",
            "src/vitte/compiler/tests/ir_coverage_manifest.json",
            "tests/diag_snapshots/core_ir_golden_manifest.txt",
        ],
    },
    {
        "id": 18,
        "name": "native_backend_architecture_matrix",
        "families": ["x86_64", "aarch64", "riscv64", "i386"],
        "evidence": [
            "tools/backend_architecture_matrix_test.py",
            "tools/backend_native_object_audit.py",
            "tools/backend_cross_sysroot_test.py",
            "tools/backend_native_toolchain_audit.py",
            "data/ffi/abi/target_abi_v1.json",
        ],
    },
    {
        "id": 19,
        "name": "stable_target_abi",
        "families": ["target-abi", "stable", "layout"],
        "evidence": [
            "tools/runtime_abi_contract.py",
            "tools/c_abi_contract_audit.py",
            "tools/backend_layout_abi_audit.py",
            "toolchain/scripts/interop/vitte_c_abi_v1.json",
            "data/ffi/abi/target_abi_v1.json",
        ],
    },
]


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def validate_target_abi(failures: list[str]) -> dict[str, object]:
    if not TARGET_ABI.is_file():
        failures.append(f"missing target ABI manifest: {rel(TARGET_ABI)}")
        return {}
    payload = json.loads(TARGET_ABI.read_text(encoding="utf-8"))
    targets = {str(entry.get("triple", "")): entry for entry in payload.get("targets", [])}
    for triple, expected in REQUIRED_TARGETS.items():
        entry = targets.get(triple)
        if not entry:
            failures.append(f"missing stable ABI target: {triple}")
            continue
        if not entry.get("stable"):
            failures.append(f"target ABI is not stable: {triple}")
        for key, value in expected.items():
            if entry.get(key) != value:
                failures.append(f"{triple}: {key}={entry.get(key)!r}, expected {value!r}")
        if entry.get("calling_convention") != "vitte_c_abi_v1":
            failures.append(f"{triple}: calling convention must be vitte_c_abi_v1")
    return payload


def main() -> int:
    failures: list[str] = []
    rows: list[dict[str, object]] = []
    for req in REQUIREMENTS:
        missing = []
        for item in req["evidence"]:
            path = ROOT / item
            if not path.exists():
                missing.append(item)
        if missing:
            failures.append(f"requirement {req['id']} missing evidence: {', '.join(missing)}")
        rows.append({**req, "missing_evidence": missing, "covered": not missing})

    target_abi = validate_target_abi(failures)
    payload = {
        "schema": "vitte.compiler.full_coverage",
        "schema_version": "1.0.0",
        "status": "pass" if not failures else "fail",
        "requirements": rows,
        "target_abi_schema": target_abi.get("schema", ""),
        "required_targets": sorted(REQUIRED_TARGETS),
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"[compiler-full-coverage][error] {failure}", file=sys.stderr)
        print(f"[compiler-full-coverage] report={rel(REPORT)}", file=sys.stderr)
        return 1
    print(f"[compiler-full-coverage] OK requirements=11-19 report={rel(REPORT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
