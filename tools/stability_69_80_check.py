#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "stability_69_80.json"


REQS = [
    (69, "profile_guided_compiler_performance", ["Makefile", "tools/startup_benchmark.py", "tools/perf_budget_check.py", "tools/perf_regression_robust.py"], ["PGO_MODE", "build-pgo-generate", "perf-budget"]),
    (70, "language_coverage_dashboard", ["tools/contracts_dashboard.py", "tools/compiler_full_coverage_check.py", "docs/reports/coverage-chart.svg"], ["coverage", "dashboard"]),
    (71, "official_grammar_stable", ["docs/spec/grammar.md", "docs/book/grammar/vitte.ebnf", "docs/book/grammar/grammar-changelog.html", "tools/grammar_alignment_checker.py"], ["freeze", "grammar", "changelog"]),
    (72, "versioned_ast_hir_mir_ir", ["tools/ast_coverage_gate.py", "tools/hir_coverage_check.py", "tools/mir_coverage_check.py", "tools/ir_coverage_check.py", "docs/release/version_contract.json"], ["AST", "HIR", "MIR", "IR"]),
    (73, "ownership_type_system_rules_stable", ["tools/borrowck_coverage_check.py", "tools/typeck_coverage_check.py", "docs/book/chapters/55-memory-safety-invariants.html", "docs/book/chapters/05-types.html"], ["ownership", "type"]),
    (74, "module_model_stable", ["tools/check_module_shape_policy.py", "tools/modules_contract_snapshots.sh", "tools/lint_critical_module_contracts.py", "docs/book/chapters/09-modules.html"], ["module", "contract"]),
    (75, "error_model_stable", ["src/vitte/compiler/diagnostics/diagnostic.vit", "tools/check_diagnostic_schema.py", "tools/check_suggestion_quality.py", "docs/book/chapters/35-error-message-anatomy.html"], ["diagnostic", "schema"]),
    (76, "abi_ffi_model_stable", ["data/ffi", "tools/validate_extern_abi.py", "tools/check_stdlib_abi_compat.py", "docs/book/chapters/65-abi-and-vitte-interop-contracts.html", "tests/truth_triangle/native_surface/ffi_contract_surface.vit"], ["ABI", "FFI"]),
    (77, "package_format_stable", ["scripts/package-matrix.sh", "tools/lint_package_layout.py", "tools/package_check_all.sh", "docs/release/version_contract.json"], ["package", "manifest_version"]),
    (78, "lockfile_format_stable", ["tools/lint_contract_lockfiles.py", "docs/release/version_contract.json"], ["lockfile", "exports_sha256"]),
    (79, "diagnostics_style_guide_stable", ["tools/check_suggestion_quality.py", "tools/check_diagnostic_catalog.py", "docs/book/chapters/35-error-message-anatomy.html", "docs/book/STYLE.html"], ["VAGUE_PATTERNS", "diagnostic"]),
    (80, "compatibility_policy_stable", ["tools/version_compatibility_gate.py", "docs/release/compatibility_matrix.md", "docs/book/chapters/40-breaking-compatibility.html"], ["compatibility", "migration"]),
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
        "schema": "vitte.stability_69_80",
        "schema_version": "1.0.0",
        "status": "pass" if not failures else "fail",
        "requirements": rows,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failures:
        for failure in failures:
            print(f"[stability-69-80][error] {failure}", file=sys.stderr)
        print(f"[stability-69-80] report={REPORT.relative_to(ROOT)}", file=sys.stderr)
        return 1
    print(f"[stability-69-80] OK requirements=69-80 report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
