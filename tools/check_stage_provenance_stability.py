#!/usr/bin/env python3
"""Audit stage input provenance and reject binary-copy stage builders."""

from __future__ import annotations

import json
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
REAL = ROOT / "tools" / "bootstrap_real" / "bootstrap_real.py"
CHAIN = ROOT / "tools" / "bootstrap_real" / "bootstrap_chain.py"
REPORT = ROOT / "target" / "reports" / "stage_provenance_stability.json"


def function_body(source: str, name: str) -> str:
    marker = f"def {name}("
    start = source.find(marker)
    if start < 0:
        return ""
    body_start = source.find("\n", start) + 1
    next_def = source.find("\ndef ", body_start)
    return source[body_start:] if next_def < 0 else source[body_start:next_def]


def main() -> int:
    failures: list[str] = []
    checks: dict[str, bool] = {}
    real = REAL.read_text(encoding="utf-8")
    chain = CHAIN.read_text(encoding="utf-8")

    builders = {
        "stage1": ("stage1_build_command", "bootstrap_compiler"),
        "stage2": ("stage2_build_command", "stage1"),
        "release": ("release_build_command", "stage2"),
    }
    provenance: dict[str, dict[str, object]] = {}
    for stage, (function, predecessor) in builders.items():
        body = function_body(real, function)
        has_build = '"build"' in body and '"-o"' in body
        uses_predecessor = predecessor in body
        copies = "copy" in body or "shutil.copy" in body
        checks[f"{stage}_uses_{predecessor}"] = has_build and uses_predecessor
        checks[f"{stage}_does_not_copy"] = not copies
        provenance[stage] = {
            "builder": function,
            "predecessor": predecessor,
            "has_build_command": has_build,
            "uses_predecessor": uses_predecessor,
            "copies_binary": copies,
        }
        if not checks[f"{stage}_uses_{predecessor}"]:
            failures.append(f"{function} does not compile from {predecessor}")
        if copies:
            failures.append(f"{function} contains a binary copy path")

    checks["chain_builds_source_bootstrap"] = "source_bootstrap_build_command()" in chain
    checks["chain_orders_stages"] = all(
        marker in chain
        for marker in ('"--stage1"', '"--stage2"', '"--release"', '"--verify-chain"')
    )
    checks["chain_does_not_execute_signed_stage0"] = '[sys.executable, str(BOOTSTRAP_REAL), "--stage0"' not in chain
    if not checks["chain_builds_source_bootstrap"]:
        failures.append("bootstrap chain does not build its bootstrap compiler from source")
    if not checks["chain_orders_stages"]:
        failures.append("bootstrap chain does not declare all ordered stage phases")
    if not checks["chain_does_not_execute_signed_stage0"]:
        failures.append("bootstrap chain executes signed stage0 as a compiler")

    payload = {
        "schema": "vitte.toolchain.stage_provenance_stability",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "checks": checks,
        "provenance": provenance,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[stage-provenance] status={payload['status']} checks={sum(checks.values())}/{len(checks)}")
    for failure in failures:
        print(f"[stage-provenance][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
