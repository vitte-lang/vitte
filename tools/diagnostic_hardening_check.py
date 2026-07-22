#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "diagnostic_hardening.json"


REQUIREMENTS = [
    {
        "id": 20,
        "name": "real_cross_compilation_tests",
        "evidence": [
            "tools/backend_cross_sysroot_test.py",
            "tools/backend_architecture_matrix_test.py",
            "tools/backend_native_toolchain_audit.py",
            "src/vitte/compiler/tests/codegen_tests.vit",
            "data/ffi/abi/target_abi_v1.json",
        ],
        "needles": {
            "tools/backend_cross_sysroot_test.py": ["cross_outputs_created", "compiler_received_sysroot", "linker_received_target"],
            "tools/backend_architecture_matrix_test.py": ["x86_64-unknown-linux-gnu", "aarch64-unknown-linux-gnu"],
            "data/ffi/abi/target_abi_v1.json": ["riscv64-unknown-linux-gnu", "i386-unknown-linux-gnu"],
        },
    },
    {
        "id": 21,
        "name": "check_build_diagnostic_parity",
        "evidence": [
            "src/vitte/compiler/tests/compiler_contract_manifest.json",
            "tools/compiler_contract_check.py",
            "tools/compile_all_compiler_files.sh",
            "src/vitte/compiler/driver/compile.vit",
        ],
        "needles": {
            "src/vitte/compiler/tests/compiler_contract_manifest.json": ["cli.check_build_diagnostic_parity"],
            "tools/compile_all_compiler_files.sh": ["check.*.out", "build.*.out"],
        },
    },
    {
        "id": 22,
        "name": "text_json_lsp_snapshot_per_code",
        "evidence": [
            "tools/check_diagnostic_catalog.py",
            "tools/generate_diagnostic_catalog_snapshots.py",
            "tools/lsp/run_checks.py",
            "src/vitte/compiler/tests/diagnostic_snapshot_tests.vit",
            "tests/diagnostics/catalog/parser.catalog.json",
            "tests/diagnostics/catalog/typeck.catalog.json",
        ],
        "needles": {
            "src/vitte/compiler/tests/diagnostic_snapshot_tests.vit": ["json_diagnostic", "lsp_diagnostic", "sarif_report"],
            "tools/lsp/run_checks.py": ["diagnostics", "code_actions"],
        },
    },
    {
        "id": 23,
        "name": "apply_every_fixit_and_recompile",
        "evidence": [
            "tools/apply_fixits_recompile_test.py",
            "tests/diagnostics/frontend/parser/fixit-keyword-recompile.vit",
            "src/vitte/compiler/tests/diagnostic_snapshot_tests.vit",
        ],
        "needles": {
            "tools/apply_fixits_recompile_test.py": ["machine-applicable", "fixed source did not recompile", "overlapping machine-applicable fix-it spans"],
        },
    },
    {
        "id": 24,
        "name": "stress_corpus_multi_unicode_crlf_tabs_long_lines",
        "evidence": [
            "tests/diag_snapshots/multiple_diagnostics_one_file.vit",
            "tests/diag_snapshots/multifile_main.vit",
            "tests/diag_snapshots/multifile_helper.vit",
            "src/vitte/compiler/tests/compiler_contract_manifest.json",
            "src/vitte/stdlib/tests/modules/std_text_inputs_test.vit",
            "src/vitte/compiler/tests/diagnostic_snapshot_tests.vit",
        ],
        "needles": {
            "src/vitte/compiler/tests/compiler_contract_manifest.json": ["tests.unicode_crlf_tabs_long_lines", "tests.fixtures_valid_invalid_multifile"],
            "src/vitte/stdlib/tests/modules/std_text_inputs_test.vit": ["café déjà vu", "alpha\\r\\nbeta", "\\tcol1\\tcol2"],
            "src/vitte/compiler/tests/diagnostic_snapshot_tests.vit": ["test_unicode_and_tab_diagnostic_rendering", "test_multiple_independent_errors_in_one_file", "test_multifile_diagnostic_spans"],
        },
    },
    {
        "id": 25,
        "name": "no_parasitic_diagnostics",
        "evidence": [
            "tools/generate_diagnostic_catalog_snapshots.py",
            "src/vitte/compiler/tests/sema_tests.vit",
            "src/vitte/compiler/tests/typeck_tests.vit",
            "src/vitte/compiler/tests/borrowck_tests.vit",
        ],
        "needles": {
            "tools/generate_diagnostic_catalog_snapshots.py": ["no_parasitic_diagnostics"],
            "src/vitte/compiler/tests/sema_tests.vit": ["test_sema_no_parasitic_diagnostics_and_error_recovery"],
            "src/vitte/compiler/tests/typeck_tests.vit": ["no_parasites"],
            "src/vitte/compiler/tests/borrowck_tests.vit": ["no_parasite"],
        },
    },
    {
        "id": 26,
        "name": "stable_diagnostic_order",
        "evidence": [
            "src/vitte/compiler/tests/diagnostic_snapshot_tests.vit",
            "src/vitte/compiler/tests/sema_tests.vit",
            "tools/typeck_snapshot_check.py",
        ],
        "needles": {
            "src/vitte/compiler/tests/diagnostic_snapshot_tests.vit": ["test_diagnostic_order_is_stable_across_runs", "test_report_orders_diagnostics_by_file_and_position"],
            "src/vitte/compiler/tests/sema_tests.vit": ["stable", "order"],
        },
    },
    {
        "id": 27,
        "name": "diagnostic_deduplication",
        "evidence": [
            "src/vitte/compiler/tests/diagnostic_snapshot_tests.vit",
            "src/vitte/compiler/tests/sema_tests.vit",
            "src/vitte/compiler/tests/typeck_coverage_manifest.json",
            "src/vitte/compiler/tests/borrowck_tests.vit",
        ],
        "needles": {
            "src/vitte/compiler/tests/diagnostic_snapshot_tests.vit": ["test_duplicate_diagnostics_are_suppressed"],
            "src/vitte/compiler/tests/sema_tests.vit": ["deduplicated"],
            "src/vitte/compiler/tests/typeck_coverage_manifest.json": ["TypeckSnapshot.deduplication"],
            "src/vitte/compiler/tests/borrowck_tests.vit": ["test_borrowck_diagnostic_deduplication_keeps_one_duplicate"],
        },
    },
]


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def main() -> int:
    failures: list[str] = []
    rows: list[dict[str, object]] = []
    for req in REQUIREMENTS:
        missing: list[str] = []
        needle_failures: list[str] = []
        for item in req["evidence"]:
            path = ROOT / item
            if not path.exists():
                missing.append(item)
        for item, needles in req.get("needles", {}).items():
            path = ROOT / item
            if not path.exists():
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            for needle in needles:
                if needle not in text:
                    needle_failures.append(f"{item} missing {needle!r}")
        if missing:
            failures.append(f"requirement {req['id']} missing evidence: {', '.join(missing)}")
        failures.extend(f"requirement {req['id']} {failure}" for failure in needle_failures)
        rows.append({
            "id": req["id"],
            "name": req["name"],
            "evidence": req["evidence"],
            "missing_evidence": missing,
            "needle_failures": needle_failures,
            "covered": not missing and not needle_failures,
        })

    payload = {
        "schema": "vitte.compiler.diagnostic_hardening",
        "schema_version": "1.0.0",
        "status": "pass" if not failures else "fail",
        "requirements": rows,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"[diagnostic-hardening][error] {failure}", file=sys.stderr)
        print(f"[diagnostic-hardening] report={rel(REPORT)}", file=sys.stderr)
        return 1
    print(f"[diagnostic-hardening] OK requirements=20-27 report={rel(REPORT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
