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
CONSTANTS_SOURCE = ROOT / "tests" / "maximal_graph" / "constants_complex.vit"
STRINGS_ARRAYS_SOURCE = ROOT / "tests" / "maximal_graph" / "strings_arrays.vit"
STRUCTURES_VARIANTS_SOURCE = ROOT / "tests" / "maximal_graph" / "structures_variants.vit"
MULTIFILE_SOURCE = ROOT / "tests" / "maximal_graph" / "imports" / "app.vit"
DIAGNOSTIC_SOURCE = ROOT / "tests" / "diagnostics" / "negative" / "typeck" / "type_mismatch.vit"
LINK_ERROR_SOURCE = ROOT / "tests" / "maximal_graph" / "link_error.vit"
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

    constants_check = run([str(BOOTSTRAP), "check", str(CONSTANTS_SOURCE)], env=env)
    checks["constants_complex_check"] = constants_check.returncode == 0
    if constants_check.returncode != 0:
        failures.append("constants and complex functions check failed: " + constants_check.stderr.strip())

    constants_output = OUTPUT_DIR / "constants_complex"
    constants_build = run(
        [str(BOOTSTRAP), "build", str(CONSTANTS_SOURCE), "-o", str(constants_output)],
        env=env,
    )
    checks["constants_complex_build"] = constants_build.returncode == 0 and constants_output.is_file()
    if not checks["constants_complex_build"]:
        failures.append("constants and complex functions build failed: " + constants_build.stderr.strip())
    if constants_output.is_file():
        constants_run = run([str(constants_output)], env=env)
        checks["constants_complex_execution"] = constants_run.returncode == 0
        if constants_run.returncode != 0:
            failures.append(f"constants and complex functions exited with {constants_run.returncode}")
    else:
        checks["constants_complex_execution"] = False

    strings_check = run([str(BOOTSTRAP), "check", str(STRINGS_ARRAYS_SOURCE)], env=env)
    checks["strings_arrays_check"] = strings_check.returncode == 0
    if strings_check.returncode != 0:
        failures.append("strings and arrays check failed: " + strings_check.stderr.strip())
    strings_output = OUTPUT_DIR / "strings_arrays"
    strings_build = run(
        [str(BOOTSTRAP), "build", str(STRINGS_ARRAYS_SOURCE), "-o", str(strings_output)],
        env=env,
    )
    checks["strings_arrays_build"] = strings_build.returncode == 0 and strings_output.is_file()
    if not checks["strings_arrays_build"]:
        failures.append("strings and arrays build failed: " + strings_build.stderr.strip())
    if strings_output.is_file():
        strings_run = run([str(strings_output)], env=env)
        checks["strings_arrays_execution"] = strings_run.returncode == 0
        checks["strings_arrays_output"] = "vitte" in strings_run.stdout
        if strings_run.returncode != 0:
            failures.append(f"strings and arrays exited with {strings_run.returncode}")
        if not checks["strings_arrays_output"]:
            failures.append("strings and arrays output is missing `vitte`")
    else:
        checks["strings_arrays_execution"] = False
        checks["strings_arrays_output"] = False

    structures_check = run([str(BOOTSTRAP), "check", str(STRUCTURES_VARIANTS_SOURCE)], env=env)
    checks["structures_variants_check"] = structures_check.returncode == 0
    if structures_check.returncode != 0:
        failures.append("structures and variants check failed: " + structures_check.stderr.strip())
    structures_output = OUTPUT_DIR / "structures_variants"
    structures_build = run(
        [str(BOOTSTRAP), "build", str(STRUCTURES_VARIANTS_SOURCE), "-o", str(structures_output)],
        env=env,
    )
    checks["structures_variants_build"] = structures_build.returncode == 0 and structures_output.is_file()
    if not checks["structures_variants_build"]:
        failures.append("structures and variants build failed: " + structures_build.stderr.strip())
    if structures_output.is_file():
        structures_run = run([str(structures_output)], env=env)
        checks["structures_variants_execution"] = structures_run.returncode == 0
        if structures_run.returncode != 0:
            failures.append(f"structures and variants exited with {structures_run.returncode}")
    else:
        checks["structures_variants_execution"] = False

    imports_check = run([str(BOOTSTRAP), "check", str(MULTIFILE_SOURCE)], env=env)
    checks["module_imports_check"] = imports_check.returncode == 0
    if imports_check.returncode != 0:
        failures.append("module imports check failed: " + imports_check.stderr.strip())
    multifile_output = OUTPUT_DIR / "multifile_imports"
    imports_build = run(
        [str(BOOTSTRAP), "build", str(MULTIFILE_SOURCE), "-o", str(multifile_output)],
        env=env,
    )
    checks["module_imports_build"] = imports_build.returncode == 0 and multifile_output.is_file()
    if not checks["module_imports_build"]:
        failures.append("module imports build failed: " + imports_build.stderr.strip())
    if multifile_output.is_file():
        imports_run = run([str(multifile_output)], env=env)
        checks["module_imports_execution"] = imports_run.returncode == 0
        if imports_run.returncode != 0:
            failures.append(f"module imports exited with {imports_run.returncode}")
    else:
        checks["module_imports_execution"] = False

    diagnostic = run([str(BOOTSTRAP), "check", str(DIAGNOSTIC_SOURCE)], env=env)
    diagnostic_output = diagnostic.stdout + diagnostic.stderr
    checks["diagnostic_program_rejected"] = diagnostic.returncode != 0
    checks["diagnostic_code_present"] = "VITTE_SEMA_E_ASSIGN" in diagnostic_output
    checks["diagnostic_source_span_present"] = "type_mismatch.vit:5" in diagnostic_output
    if diagnostic.returncode == 0:
        failures.append("diagnostic program unexpectedly passed")
    if not checks["diagnostic_code_present"]:
        failures.append("diagnostic output is missing VITTE_SEMA_E_ASSIGN")
    if not checks["diagnostic_source_span_present"]:
        failures.append("diagnostic output is missing the source span")

    link_output = OUTPUT_DIR / "link_error"
    link = run(
        [str(BOOTSTRAP), "build", str(LINK_ERROR_SOURCE), "-o", str(link_output)],
        env=env,
    )
    link_text = link.stdout + link.stderr
    checks["link_error_rejected"] = link.returncode != 0 and not link_output.exists()
    checks["link_error_reported"] = "Undefined symbols" in link_text and "VITTE_DRIVER_E_LINK" in link_text
    if not checks["link_error_rejected"]:
        failures.append("link-error program unexpectedly produced an executable")
    if not checks["link_error_reported"]:
        failures.append("linker failure did not expose the native linker and Vitte link diagnostics")

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
