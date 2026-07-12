#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "typeck_surface_audit.json"


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        print(f"[typeck-surface][error] missing file: {rel}", file=sys.stderr)
        raise SystemExit(1)
    return path.read_text(encoding="utf-8", errors="replace")


def has(text: str, needle: str) -> bool:
    return needle in text


def scan_vit_files() -> list[Path]:
    return sorted((ROOT / "src" / "vitte" / "compiler").rglob("*.vit"))


def main() -> int:
    checks: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/analysis/pipeline.vit", "run_production_typeck_hir", "production analysis pipeline must use the canonical production typeck API"),
        ("src/vitte/compiler/middle/pipeline.vit", "run_production_typeck_hir", "middle pipeline must use the canonical production typeck API"),
        ("src/vitte/compiler/driver/compile.vit", "run_production_typeck_hir", "driver compile path must use the canonical production typeck API"),
        ("src/vitte/compiler/tests/typeck_tests.vit", "run_production_typeck_hir", "production typeck contract tests must stay present"),
        ("src/vitte/compiler/tests/typeck_complete_tests.vit", "TYPECK_COMPLETE_MIGRATION_ONLY", "complete tests must remain explicitly migration-only"),
        ("src/vitte/compiler/tests/typeck_complete_tests.vit", "run_complete_typeck_frontend", "migration coverage must call the isolated complete module"),
        ("src/vitte/compiler/analysis/typeck/README.md", "The canonical Vitte type checker is HIR-based", "README must name the single canonical architecture"),
        ("src/vitte/compiler/analysis/typeck/README.md", "run_production_typeck_hir", "README must document the production typeck API surface"),
        ("src/vitte/compiler/analysis/typeck/ARCHITECTURE.md", "HIR is the sole canonical type-checker architecture", "the accepted architecture decision must remain explicit"),
        ("src/vitte/compiler/analysis/typeck/api.vit", "proc run_production_typeck_hir(", "typeck API must expose the production typeck entrypoint"),
        ("src/vitte/compiler/analysis/typeck/api.vit", 'give "hir"', "typeck API must identify HIR as canonical"),
        ("src/vitte/compiler/analysis/typeck/api.vit", 'give "canonical"', "typeck API must identify the production status as canonical"),
    ]

    results: list[dict[str, str]] = []
    failures: list[str] = []

    for rel, needle, reason in checks:
        text = read(rel)
        status = "present" if has(text, needle) else "missing"
        results.append(
            {
                "file": rel,
                "needle": needle,
                "reason": reason,
                "status": status,
            }
        )
        if status != "present":
            failures.append(f"{rel}: missing `{needle}`")

    checker_text = read("src/vitte/compiler/analysis/typeck/checker.vit")
    complete_api_text = read("src/vitte/compiler/analysis/typeck/complete/api.vit")

    extra_checks = [
        ("src/vitte/compiler/analysis/typeck/checker.vit", "proc run_typeck_hir(", checker_text, "checker surface must expose run_typeck_hir"),
        ("src/vitte/compiler/analysis/typeck/complete/api.vit", "proc run_complete_typeck_frontend(", complete_api_text, "complete API must expose run_complete_typeck_frontend"),
    ]
    for rel, needle, text, reason in extra_checks:
        status = "present" if has(text, needle) else "missing"
        results.append(
            {
                "file": rel,
                "needle": needle,
                "reason": reason,
                "status": status,
            }
        )
        if status != "present":
            failures.append(f"{rel}: missing `{needle}`")

    forbidden_checks = [
        (
            "src/vitte/compiler/analysis/pipeline.vit",
            "run_complete_typeck_frontend",
            read("src/vitte/compiler/analysis/pipeline.vit"),
            "production analysis pipeline must not call the complete AST checker directly",
        ),
        (
            "src/vitte/compiler/middle/pipeline.vit",
            "run_complete_typeck_frontend",
            read("src/vitte/compiler/middle/pipeline.vit"),
            "middle pipeline must not call the complete AST checker directly",
        ),
        (
            "src/vitte/compiler/driver/compile.vit",
            "run_complete_typeck_frontend",
            read("src/vitte/compiler/driver/compile.vit"),
            "driver compile path must not call the complete AST checker directly",
        ),
        (
            "src/vitte/compiler/analysis/typeck/api.vit",
            "analysis/typeck/complete",
            read("src/vitte/compiler/analysis/typeck/api.vit"),
            "the public typeck API must expose only the HIR checker",
        ),
        (
            "src/vitte/compiler/analysis/typeck/mod.vit",
            "analysis/typeck/complete",
            read("src/vitte/compiler/analysis/typeck/mod.vit"),
            "the canonical typeck module root must not aggregate the migration checker",
        ),
        (
            "src/vitte/compiler/**/*.vit",
            "run_experimental_complete_typeck",
            "\n".join(path.read_text(encoding="utf-8", errors="replace") for path in scan_vit_files()),
            "the removed experimental public API must not be reintroduced",
        ),
    ]
    for rel, needle, text, reason in forbidden_checks:
        status = "forbidden_present" if has(text, needle) else "absent"
        results.append(
            {
                "file": rel,
                "needle": needle,
                "reason": reason,
                "status": status,
            }
        )
        if status != "absent":
            failures.append(f"{rel}: forbidden `{needle}` present")

    direct_complete_subpath_imports: list[str] = []
    direct_complete_types_test_imports: list[str] = []
    complete_root_imports: list[str] = []
    for path in scan_vit_files():
        rel = str(path.relative_to(ROOT))
        text = path.read_text(encoding="utf-8", errors="replace")
        if rel.startswith("src/vitte/compiler/analysis/typeck/complete/"):
            continue
        if "use vitte/compiler/analysis/typeck/complete/" in text:
            direct_complete_subpath_imports.append(rel)
        if rel.startswith("src/vitte/compiler/tests/") and "use vitte/compiler/analysis/typeck/complete/types" in text:
            direct_complete_types_test_imports.append(rel)
        if "use vitte/compiler/analysis/typeck/complete.{" in text:
            complete_root_imports.append(rel)

    results.append(
        {
            "file": "src/vitte/compiler/**/*.vit",
            "needle": "use vitte/compiler/analysis/typeck/complete/",
            "reason": "non-experimental code should use either the public typeck API or the complete module root, not complete subpaths",
            "status": "absent" if not direct_complete_subpath_imports else "forbidden_present",
        }
    )
    if direct_complete_subpath_imports:
        failures.append(
            "direct complete subpath imports outside analysis/typeck/complete/: "
            + ", ".join(sorted(direct_complete_subpath_imports))
        )

    results.append(
        {
            "file": "src/vitte/compiler/tests/*.vit",
            "needle": "use vitte/compiler/analysis/typeck/complete/types",
            "reason": "migration tests must consume complete types through the isolated module root",
            "status": "absent" if not direct_complete_types_test_imports else "forbidden_present",
        }
    )
    if direct_complete_types_test_imports:
        failures.append(
            "direct complete types imports in tests: "
            + ", ".join(sorted(direct_complete_types_test_imports))
        )

    allowed_complete_root_imports = {"src/vitte/compiler/tests/typeck_complete_tests.vit"}
    unexpected_complete_root_imports = sorted(set(complete_root_imports) - allowed_complete_root_imports)
    missing_complete_root_imports = sorted(allowed_complete_root_imports - set(complete_root_imports))
    complete_root_ok = not unexpected_complete_root_imports and not missing_complete_root_imports
    results.append(
        {
            "file": "src/vitte/compiler/**/*.vit",
            "needle": "use vitte/compiler/analysis/typeck/complete.{",
            "reason": "only the migration-only complete test may import the isolated checker root",
            "status": "migration_test_only" if complete_root_ok else "forbidden_present",
        }
    )
    if not complete_root_ok:
        failures.append(
            "complete root imports differ from migration allowance: "
            f"unexpected={unexpected_complete_root_imports} missing={missing_complete_root_imports}"
        )

    payload = {
        "schema": "vitte.compiler.typeck_surface_audit",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "results": results,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"[typeck-surface] status={payload['status']} report={REPORT.relative_to(ROOT)}")
    for result in results:
        print(f"[typeck-surface][check] {result['status']} {result['file']} needle={result['needle']}")
    for failure in failures:
        print(f"[typeck-surface][error] {failure}")

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
