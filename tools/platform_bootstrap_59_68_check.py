#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "platform_bootstrap_59_68.json"


REQS = [
    (59, "solaris_amd64_i386", ["scripts/build-solaris-package.sh", "scripts/package-matrix.sh", "scripts/verify-installers.sh"], ["solaris", "amd64", "i386"]),
    (60, "real_platform_ci", [".github/workflows/ci.yml", ".github/workflows/bootstrap.yml", "tools/scripts_build_arch_matrix_test.sh"], ["runs-on", "installer arch matrix", "bootstrap-native-contract"]),
    (61, "progressive_self_hosting_stage0_stage1_stage2", ["Makefile", "src/vitte/compiler/tests/compiler_contract_manifest.json", "tools/bootstrap_selfhost_repro.sh", "tools/bootstrap_vitte_hard_gate.sh"], ["stage0", "stage1", "stage2"]),
    (62, "stage_bytecode_ir_comparison", ["Makefile", "tools/same_output_hash_test.sh", "tools/check_bootstrap_native_drift.sh", "tools/ir_coverage_check.py"], ["same-output-hash", "IR", "hash"]),
    (63, "reproducible_bootstrap", ["tools/bootstrap_selfhost_repro.sh", "toolchain/test_bootstrap_reproducibility.sh", "tools/bootstrap_clean_checkout.py", "tools/bootstrap_offline.py"], ["repro", "sha256", "bootstrap"]),
    (64, "complete_determinism_tests", ["tools/determinism_smoke.sh", "tools/native_object_determinism_test.py", "tools/same_output_hash_test.sh"], ["deterministic", "hash"]),
    (65, "memory_security_audit", ["tools/security_gates_report.sh", "tools/security_input_limits_smoke.sh", "tools/sanitizers_maximal.sh", "tools/memory_model/run_checks.py"], ["security", "memory"]),
    (66, "compiler_fuzzing", ["tools/parser_lexer_fuzz_smoke.py", "tools/typeck_fuzz_test.py", "tools/compiler_stress_maximal.py"], ["fuzz", "crash"]),
    (67, "crash_minimizer", ["tools/crash_report_snapshots.sh", "src/vitte/compiler/driver/compiler.vit", "tools/parser_lexer_fuzz_smoke.py"], ["crash", "reduce"]),
    (68, "automatic_case_reduction", ["src/vitte/compiler/driver/compiler.vit", "src/vitte/stdlib/tests/fuzz/minimize.vit", "tools/stdlib/run_checks.py"], ["reduce", "minimize_failure", "minimizer"]),
]


def read_evidence(paths: list[str]) -> str:
    chunks: list[str] = []
    for item in paths:
        path = ROOT / item
        if path.is_file():
            chunks.append(path.read_text(encoding="utf-8", errors="replace"))
        elif path.is_dir():
            chunks.append("\n".join(str(p.relative_to(ROOT)) for p in path.rglob("*") if p.is_file()))
    return "\n".join(chunks)


def main() -> int:
    failures: list[str] = []
    rows: list[dict[str, object]] = []
    for req_id, name, evidence, needles in REQS:
        missing = [item for item in evidence if not (ROOT / item).exists()]
        text = read_evidence(evidence)
        absent = [needle for needle in needles if needle.lower() not in text.lower()]
        covered = not missing and not absent
        if missing:
            failures.append(f"{req_id} {name}: missing evidence {', '.join(missing)}")
        if absent:
            failures.append(f"{req_id} {name}: missing proof terms {', '.join(absent)}")
        rows.append({
            "id": req_id,
            "name": name,
            "evidence": evidence,
            "required_terms": needles,
            "missing_evidence": missing,
            "missing_terms": absent,
            "covered": covered,
        })

    payload = {
        "schema": "vitte.platform_bootstrap_59_68",
        "schema_version": "1.0.0",
        "status": "pass" if not failures else "fail",
        "requirements": rows,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"[platform-bootstrap-59-68][error] {failure}", file=sys.stderr)
        print(f"[platform-bootstrap-59-68] report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[platform-bootstrap-59-68] OK requirements=59-68 report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
