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


def main() -> int:
    checks: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/analysis/pipeline.vit", "run_typeck_hir", "production analysis pipeline must use the HIR checker"),
        ("src/vitte/compiler/middle/pipeline.vit", "run_typeck_hir", "middle pipeline must use the HIR checker"),
        ("src/vitte/compiler/driver/compile.vit", "run_typeck_hir", "driver compile path must use the HIR checker"),
        ("src/vitte/compiler/tests/typeck_tests.vit", "run_typeck_hir", "HIR checker contract tests must stay present"),
        ("src/vitte/compiler/tests/typeck_complete_tests.vit", "run_complete_typeck_frontend", "advanced complete typeck tests must stay present"),
        ("src/vitte/compiler/analysis/typeck/README.md", "run_complete_typeck_frontend", "README must document the advanced complete surface"),
        ("src/vitte/compiler/analysis/typeck/README.md", "run_typeck_hir", "README must document the production HIR surface"),
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
