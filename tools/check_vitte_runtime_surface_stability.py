#!/usr/bin/env python3
"""Check the runtime module, host ABI surface, and registry entry as one contract."""

from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RUNTIME = ROOT / "src/vitte/stdlib/runtime.vitl"
HOST_RUNTIME = ROOT / "src/vitte/stdlib/io/host_runtime.vitl"
MODULES = ROOT / "src/vitte/stdlib/modules.vitte.json"
ABI_REPORT = ROOT / "target/reports/runtime_abi_contract.json"
REAL_REPORT = ROOT / "target/runtime_stdlib_real/real_checks.json"
REPORT = ROOT / "target/reports/vitte_runtime_surface_stability.json"


RUNTIME_NEEDLES = (
    "space vitte/stdlib/runtime",
    "form RuntimeManifest",
    "form RuntimeHealth",
    "form RuntimeSummary",
    "proc runtime_version()",
    "proc runtime_ready()",
    "proc runtime_health()",
    "proc runtime_selftest()",
    "export *",
)

HOST_NEEDLES = (
    "space vitte/stdlib/io/host_runtime",
    "intrinsic vitte_host_emit_assembly_object(",
    "intrinsic vitte_host_verify_native_object(",
    "intrinsic vitte_host_link_executable(",
    "proc host_emit_assembly_object(",
    "proc host_verify_native_object(",
    "proc host_link_executable(",
    "share host_runtime_available",
)


def main() -> int:
    failures: list[str] = []
    checks: dict[str, bool] = {}
    runtime_text = RUNTIME.read_text(encoding="utf-8") if RUNTIME.is_file() else ""
    host_text = HOST_RUNTIME.read_text(encoding="utf-8") if HOST_RUNTIME.is_file() else ""
    for index, needle in enumerate(RUNTIME_NEEDLES):
        checks[f"runtime_{index}"] = needle in runtime_text
    for index, needle in enumerate(HOST_NEEDLES):
        checks[f"host_{index}"] = needle in host_text
    if not all(checks.values()):
        failures.append("runtime source surface is incomplete")

    if not MODULES.is_file():
        failures.append("stdlib module registry is missing")
    else:
        registry = json.loads(MODULES.read_text(encoding="utf-8"))
        entries = registry.get("modules", [])
        runtime_entries = [entry for entry in entries if entry.get("module") == "vitte/stdlib/runtime"]
        checks["registry_runtime_entry"] = bool(runtime_entries and runtime_entries[0].get("path") == "src/vitte/stdlib/runtime.vitl")
        checks["registry_runtime_official"] = bool(runtime_entries and runtime_entries[0].get("official") is True)
        if not checks["registry_runtime_entry"] or not checks["registry_runtime_official"]:
            failures.append("stdlib registry does not expose the official runtime.vitl module")

    for path, label in ((ABI_REPORT, "ABI"), (REAL_REPORT, "real runtime")):
        if not path.is_file():
            failures.append(f"{label} report is missing: {path.relative_to(ROOT)}")
            continue
        report = json.loads(path.read_text(encoding="utf-8"))
        key = f"{label.lower().replace(' ', '_')}_report_pass"
        checks[key] = report.get("status") in ("pass", "ok")
        if not checks[key]:
            failures.append(f"{label} report is not passing")

    payload = {
        "schema": "vitte.compiler.vitte_runtime_surface_stability",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "checks": checks,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[vitte-runtime] status={payload['status']} checks={len(checks)}")
    for failure in failures:
        print(f"[vitte-runtime][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
