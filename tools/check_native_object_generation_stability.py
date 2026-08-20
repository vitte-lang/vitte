#!/usr/bin/env python3
"""Validate the native object generation evidence produced by the runtime gate."""

from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REPORT = ROOT / "target" / "reports" / "native_object_determinism.json"
OUT_REPORT = ROOT / "target" / "reports" / "native_object_generation_stability.json"


SOURCE_CONTRACTS: dict[str, tuple[str, ...]] = {
    "src/vitte/compiler/driver/compile.vit": (
        "proc native_object_output_path(",
        "emit_native_object_from_assembly(backend0.codegen.assembly",
        "emit_native_object_from_llvm_ir(backend0.codegen.assembly",
        "native_output_overwrites_source",
    ),
    "src/vitte/compiler/backend/native_bridge.vit": (
        "proc emit_native_object_from_assembly(",
        "host_emit_assembly_object",
        "host_verify_native_object",
        "deterministic native assembly object materialized and verified",
    ),
    "src/vitte/compiler/backends/runtime_c/vitte_runtime.c": (
        "int32_t vitte_host_emit_assembly_object(",
        'argv[arg_index++] = "assembler";',
        'setenv("SOURCE_DATE_EPOCH", "0", 1);',
        "remove(object_c);",
    ),
}


REQUIRED_EVIDENCE = (
    "objects_emitted",
    "objects_identical",
    "elf_contract_valid",
    "runtime_verifier_passed",
    "runtime_verifier_rejects_invalid",
    "required_sections_present",
    "defined_symbol_present",
    "undefined_symbol_present",
    "relocation_targets_external",
    "assembly_sidecars_removed",
)


def main() -> int:
    failures: list[str] = []
    contracts: list[dict[str, object]] = []
    for relative, needles in SOURCE_CONTRACTS.items():
        path = ROOT / relative
        text = path.read_text(encoding="utf-8") if path.is_file() else ""
        missing = [needle for needle in needles if needle not in text]
        contracts.append({"file": relative, "required": len(needles), "missing": missing})
        if missing:
            failures.append(f"{relative}: missing {', '.join(missing)}")

    if not REPORT.is_file():
        failures.append(f"missing native object evidence: {REPORT.relative_to(ROOT)}")
        evidence: dict[str, object] = {}
    else:
        evidence = json.loads(REPORT.read_text(encoding="utf-8"))
        if evidence.get("status") != "pass":
            failures.append("native object determinism report is not passing")
        checks = evidence.get("checks", {})
        if not isinstance(checks, dict):
            failures.append("native object determinism report has no checks object")
        else:
            missing = [name for name in REQUIRED_EVIDENCE if checks.get(name) is not True]
            if missing:
                failures.append("native object evidence missing passing checks: " + ", ".join(missing))

    payload = {
        "schema": "vitte.compiler.native_object_generation_stability",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "contracts": contracts,
        "evidence": evidence,
        "failures": failures,
    }
    OUT_REPORT.parent.mkdir(parents=True, exist_ok=True)
    OUT_REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[native-object-generation] status={payload['status']} contracts={len(contracts)}")
    for failure in failures:
        print(f"[native-object-generation][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
