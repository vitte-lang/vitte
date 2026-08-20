#!/usr/bin/env python3
"""Verify that the retained C17 bootstrap and Vitte backend stay separate."""

from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "c17_separation_stability.json"


REQUIRED: dict[str, tuple[str, ...]] = {
    "bootstrap/src/driver/driver.c": (
        "-std=c17 -Wall -Wextra -pedantic",
        "VITTE_DRIVER_STAGE_CODEGEN_C",
        "vitte_driver_compile_c",
    ),
    "bootstrap/src/codegen/codegen.h": (
        "VITTE_CODEGEN_BACKEND_C17",
        "VITTE_CODEGEN_INPUT_IR",
    ),
    "bootstrap/src/codegen/codegen.c": (
        "vitte_codegen_validate_ir",
        "vitte_c17_backend_emit_ir_to_file",
        "vitte_c17_backend_emit_ir_to_buffer",
    ),
    "bootstrap/src/backend/c17/backend.h": (
        "vitte_c17_backend_emit_ir_to_file",
        "vitte_c17_backend_emit_ir_to_buffer",
    ),
    "bootstrap/src/backend/c17/module.c": (
        "vitte_c17_module_emit_ir",
        "vitte_c17_emit_ir_function_prototype",
        "vitte_c17_emit_ir_function_body",
    ),
    "src/vitte/compiler/backend/codegen/mod.vit": (
        "run_codegen_x86_64_with_profile",
        'object_format: "elf64-relocatable"',
    ),
    "src/vitte/compiler/backend/native_bridge.vit": (
        "host_emit_assembly_object",
        "host_verify_native_object",
    ),
}


FORBIDDEN_IN_VITTE: tuple[str, ...] = (
    "VITTE_CODEGEN_BACKEND_C17",
    "VITTE_C17_GENERIC_COMPILER",
    "bootstrap/src/backend/c17",
    "-std=c17",
    "vitte_c17_backend_emit_ir",
)


def read(relative: str) -> str:
    path = ROOT / relative
    return path.read_text(encoding="utf-8") if path.is_file() else ""


def main() -> int:
    failures: list[str] = []
    checks: list[dict[str, object]] = []

    for relative, needles in REQUIRED.items():
        text = read(relative)
        missing = [needle for needle in needles if needle not in text]
        checks.append({"file": relative, "required": len(needles), "missing": missing})
        if missing:
            failures.append(f"{relative}: missing {', '.join(missing)}")

    vitte_files = sorted((ROOT / "src/vitte/compiler").rglob("*.vit"))
    for path in vitte_files:
        text = path.read_text(encoding="utf-8")
        present = [marker for marker in FORBIDDEN_IN_VITTE if marker in text]
        if present:
            failures.append(f"{path.relative_to(ROOT)}: C17 marker leaked into Vitte source: {', '.join(present)}")
    checks.append({"vitte_source_files": len(vitte_files), "forbidden_markers": list(FORBIDDEN_IN_VITTE), "status": "clean" if not any("leaked" in failure for failure in failures) else "leak"})

    payload = {
        "schema": "vitte.compiler.c17_separation_stability",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "checks": checks,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[c17-separation] status={payload['status']} checks={len(checks)} vitte_sources={len(vitte_files)}")
    for failure in failures:
        print(f"[c17-separation][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
