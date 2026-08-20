#!/usr/bin/env python3
"""Compile, execute, and record the maximal graph program contract."""

from __future__ import annotations

import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BOOTSTRAP = ROOT / "target" / "bootstrap-c17" / "vitte-bootstrap"
SOURCE = ROOT / "tests" / "maximal_graph" / "maximal_graph.vit"
REPORT = ROOT / "target" / "reports" / "maximal_graph_stability.json"
OUTPUT_DIR = ROOT / "target" / "reports" / "maximal_graph"


def run(command: list[str], *, env: dict[str, str] | None = None) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command,
        cwd=ROOT,
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    failures: list[str] = []
    checks: dict[str, bool] = {}
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env["VITTE_C17_GENERIC_COMPILER"] = "1"

    check = run([str(BOOTSTRAP), "check", str(SOURCE)], env=env)
    checks["source_checks"] = check.returncode == 0
    if check.returncode != 0:
        failures.append("maximal graph source check failed: " + check.stderr.strip())

    first = OUTPUT_DIR / "maximal_graph_first"
    second = OUTPUT_DIR / "maximal_graph_second"
    first_build = run([str(BOOTSTRAP), "build", str(SOURCE), "-o", str(first)], env=env)
    second_build = run([str(BOOTSTRAP), "build", str(SOURCE), "-o", str(second)], env=env)
    checks["first_native_build"] = first_build.returncode == 0 and first.is_file()
    checks["second_native_build"] = second_build.returncode == 0 and second.is_file()
    if not checks["first_native_build"]:
        failures.append("first native build failed: " + first_build.stderr.strip())
    if not checks["second_native_build"]:
        failures.append("second native build failed: " + second_build.stderr.strip())

    if first.is_file():
        execution = run([str(first)], env=env)
        checks["native_execution"] = execution.returncode == 0
        checks["expected_graph_output"] = all(
            marker in execution.stdout for marker in ("nodes", "alpha", "beta", "gamma", "edges")
        )
        if execution.returncode != 0:
            failures.append(f"maximal graph exited with {execution.returncode}")
        if not checks["expected_graph_output"]:
            failures.append("maximal graph output is missing expected graph markers")
    else:
        checks["native_execution"] = False
        checks["expected_graph_output"] = False

    first_c = first.with_suffix(first.suffix + ".c")
    second_c = second.with_suffix(second.suffix + ".c")
    checks["deterministic_lowered_source"] = first_c.is_file() and second_c.is_file() and sha256(first_c) == sha256(second_c)
    if not checks["deterministic_lowered_source"]:
        failures.append("repeated maximal graph builds produced different lowered C sources")

    payload = {
        "schema": "vitte.compiler.maximal_graph_stability",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "source": str(SOURCE.relative_to(ROOT)),
        "checks": checks,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(f"[maximal-graph] status={payload['status']} checks={sum(checks.values())}/{len(checks)}")
    for failure in failures:
        print(f"[maximal-graph][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
