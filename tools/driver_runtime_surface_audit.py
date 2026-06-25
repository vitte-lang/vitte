#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "driver_runtime_surface_audit.json"


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        print(f"[driver-runtime][error] missing file: {rel}", file=sys.stderr)
        raise SystemExit(1)
    return path.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    required_checks: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/main.vit", "run_cli_main_with_ice_boundary", "public compiler entry must route through driver runtime core"),
        ("src/vitte/compiler/driver/compiler.vit", "proc run_cli_main_with_ice_boundary(", "driver runtime core must expose panic-boundary CLI entry"),
        ("src/vitte/compiler/driver/compiler.vit", "proc parse_cli_request(", "driver runtime core must keep CLI request normalization"),
        ("src/vitte/compiler/driver/compiler.vit", "proc compile_source(", "driver runtime core must keep source compilation entry"),
        ("src/vitte/compiler/driver/compiler.vit", "proc compile_file(", "driver runtime core must keep file compilation entry"),
        ("src/vitte/compiler/driver/README.md", "driver/compiler.vit", "driver README must document the runtime core"),
    ]

    forbidden_checks: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/backends/c_emit", "runtime driver must not import legacy c_emit adapter directly"),
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/backends/llvm_emit", "runtime driver must not import legacy llvm_emit adapter directly"),
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/backends/wasm/mod", "runtime driver must not import wasm adapter mod directly"),
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/backends/backend_infrastructure", "runtime driver must not import backend infrastructure adapter directly"),
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/analysis/const_eval/mod", "runtime driver must not keep dead const_eval umbrella imports"),
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/analysis/static/mod", "runtime driver must not keep dead static-analysis umbrella imports"),
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/infrastructure/distributed/mod", "runtime driver must not keep dead distributed umbrella imports"),
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/ir/hir_to_mir_lowering", "runtime driver must not keep dead direct IR-lowering imports"),
    ]

    results: list[dict[str, str]] = []
    failures: list[str] = []

    for rel, needle, reason in required_checks:
        text = read(rel)
        ok = needle in text
        results.append({"file": rel, "needle": needle, "reason": reason, "status": "present" if ok else "missing"})
        if not ok:
            failures.append(f"{rel}: missing `{needle}`")

    for rel, needle, reason in forbidden_checks:
        text = read(rel)
        forbidden = needle in text
        results.append({"file": rel, "needle": needle, "reason": reason, "status": "forbidden-present" if forbidden else "absent"})
        if forbidden:
            failures.append(f"{rel}: forbidden `{needle}`")

    payload = {
        "schema": "vitte.compiler.driver_runtime_surface_audit",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "results": results,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"[driver-runtime] status={payload['status']} report={REPORT.relative_to(ROOT)}")
    for result in results:
        print(f"[driver-runtime][check] {result['status']} {result['file']} needle={result['needle']}")
    for failure in failures:
        print(f"[driver-runtime][error] {failure}")

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
