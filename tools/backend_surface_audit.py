#!/usr/bin/env python3
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "backend_surface_audit.json"


def read(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        print(f"[backend-surface][error] missing file: {rel}", file=sys.stderr)
        raise SystemExit(1)
    return path.read_text(encoding="utf-8", errors="replace")


def main() -> int:
    checks: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/main.vit", "use vitte/compiler/driver/compiler.{ run_cli_main_with_ice_boundary }", "public compiler entry must route through canonical driver runtime"),
        ("src/vitte/compiler/driver/compile.vit", "use vitte/compiler/backend/pipeline.", "driver compile path must use canonical backend pipeline"),
        ("src/vitte/compiler/driver/compile.vit", "use vitte/compiler/backend/codegen.", "driver compile path must use canonical codegen surface"),
        ("src/vitte/compiler/driver/compile.vit", "use vitte/compiler/backend/link/linker.", "driver compile path must use canonical linker surface"),
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/backend/pipeline.{ BackendPipelineResult }", "runtime driver must consume canonical backend pipeline result surface"),
        ("src/vitte/compiler/backend/mod.vit", "use vitte/compiler/backend/adapters", "canonical backend facade must own adapter reachability"),
        ("src/vitte/compiler/backend/adapters.vit", "use vitte/compiler/backends/c_emit", "canonical adapter registry must own the legacy C adapter"),
        ("src/vitte/compiler/backend/adapters.vit", "use vitte/compiler/backends/llvm_emit", "canonical adapter registry must own the legacy LLVM adapter"),
        ("src/vitte/compiler/backend/adapters.vit", "use vitte/compiler/backends/vitte_emit", "canonical adapter registry must own the specialized Vitte adapter"),
        ("src/vitte/compiler/backend/adapters.vit", "use vitte/compiler/backends/wasm", "canonical adapter registry must own the WASM adapter"),
        ("src/vitte/compiler/backends/vitte_emit/mod.vit", "use vitte/compiler/backends/vitte_emit/pipeline", "vitte_emit root must own its component graph"),
        ("src/vitte/compiler/backend/pipeline.vit", "compile_to_valid_ir_with_profile_and_packaging", "canonical backend pipeline entrypoint must exist"),
        ("src/vitte/compiler/backend/c/pipeline.vit", "run_c_backend_source", "canonical C backend surface must keep its entrypoint"),
        ("src/vitte/compiler/tests/c_backend_tests.vit", "run_c_backend_source", "canonical C backend tests must stay wired"),
        ("src/vitte/compiler/backends/vitte_emit/api_manifest.vitl", "canonical=src/vitte/compiler/backend/*", "vitte_emit adapter must declare canonical backend ownership"),
        ("src/vitte/compiler/backend/README.md", "src/vitte/compiler/backends/*", "canonical backend README must document the sibling adapter tree"),
        ("src/vitte/compiler/backends/README.md", "src/vitte/compiler/backend/*", "adapter backend README must document the canonical backend tree"),
    ]

    forbidden_checks: list[tuple[str, str, str]] = [
        ("src/vitte/compiler/main.vit", "use vitte/compiler/backends/c_emit", "public compiler entry must not depend directly on legacy c_emit adapter"),
        ("src/vitte/compiler/main.vit", "use vitte/compiler/backends/llvm_emit", "public compiler entry must not depend directly on legacy llvm_emit adapter"),
        ("src/vitte/compiler/main.vit", "use vitte/compiler/backends/vitte_emit", "public compiler entry must not depend directly on vitte_emit adapter modules"),
        ("src/vitte/compiler/main.vit", "use vitte/compiler/backends/wasm", "public compiler entry must not depend directly on wasm adapter modules"),
        ("src/vitte/compiler/driver/compile.vit", "use vitte/compiler/backends/c_emit", "driver compile path must not depend directly on legacy c_emit adapter"),
        ("src/vitte/compiler/driver/compile.vit", "use vitte/compiler/backends/llvm_emit", "driver compile path must not depend directly on legacy llvm_emit adapter"),
        ("src/vitte/compiler/driver/compile.vit", "use vitte/compiler/backends/vitte_emit", "driver compile path must not depend directly on vitte_emit adapter modules"),
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/backends/vitte_emit", "runtime driver must not depend directly on vitte_emit adapter modules"),
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/backends/c_emit", "runtime driver must not depend directly on legacy c_emit adapter"),
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/backends/llvm_emit", "runtime driver must not depend directly on legacy llvm_emit adapter"),
        ("src/vitte/compiler/driver/compiler.vit", "use vitte/compiler/backends/wasm", "runtime driver must not depend directly on wasm adapter modules"),
        ("src/vitte/compiler/backend/pipeline.vit", "use vitte/compiler/backends/c_emit", "canonical backend pipeline must not route through legacy c_emit adapter"),
        ("src/vitte/compiler/backend/pipeline.vit", "use vitte/compiler/backends/llvm_emit", "canonical backend pipeline must not route through legacy llvm_emit adapter"),
    ]

    results: list[dict[str, str]] = []
    failures: list[str] = []

    for rel, needle, reason in checks:
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
        "schema": "vitte.compiler.backend_surface_audit",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "results": results,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    print(f"[backend-surface] status={payload['status']} report={REPORT.relative_to(ROOT)}")
    for result in results:
        print(f"[backend-surface][check] {result['status']} {result['file']} needle={result['needle']}")
    for failure in failures:
        print(f"[backend-surface][error] {failure}")

    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
