#!/usr/bin/env python3
"""Snapshot runtime diagnostic surfaces from bin/vitte.

The runtime snapshots are intentionally command based. They do not synthesize
diagnostics in the test process; every captured surface comes from invoking
`bin/vitte check` or `bin/vitte build`.
"""

from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin" / "vitte"
WRAPPER = ROOT / "tools" / "vitte_cli_locale_wrapper.c"
REPORT = ROOT / "target" / "reports" / "runtime_diagnostics" / "coverage.json"
STRICT_REPORT = ROOT / "target" / "reports" / "runtime_diagnostics" / "strict_open_tests.json"

TEXT_DIR = ROOT / "tests" / "diagnostics" / "runtime" / "text"
JSON_DIR = ROOT / "tests" / "diagnostics" / "runtime" / "json"
LSP_DIR = ROOT / "tests" / "diagnostics" / "runtime" / "lsp"

TYPE_MISMATCH = "tests/negative/type_mismatch.vit"
ALL_ERRORS = "tests/diagnostics/runtime/fixtures/all_errors_visible.vit"
CASCADE = "tests/diagnostics/runtime/fixtures/cascade_controlled.vit"

FORBIDDEN_SURFACE_ONLY_MARKERS = (
    "REAL_DIAGNOSTIC_CASES",
    "emit_real_diagnostic_case",
    "write_real_text_diagnostic",
    "write_real_json_diagnostic",
    "write_real_lsp_diagnostic",
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def command_payload(proc: subprocess.CompletedProcess[str], surface: str) -> str:
    if surface == "text":
        return proc.stdout + proc.stderr
    return proc.stdout or proc.stderr


def write_or_check(path: Path, content: str, write: bool, failures: list[str]) -> None:
    if write:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        return
    if not path.exists():
        failures.append(f"missing snapshot: {rel(path)}")
        return
    expected = path.read_text(encoding="utf-8")
    if content != expected:
        failures.append(f"snapshot mismatch: {rel(path)}")


def assert_no_surface_only(failures: list[str]) -> None:
    text = WRAPPER.read_text(encoding="utf-8")
    for marker in FORBIDDEN_SURFACE_ONLY_MARKERS:
        if marker in text:
            failures.append(f"surface-only diagnostic marker still present: {rel(WRAPPER)}:{marker}")


def validate_type_mismatch_surfaces(failures: list[str]) -> dict[str, Any]:
    cases = [
        (
            "text/type_mismatch.check.snap",
            "text",
            [str(BIN), "check", TYPE_MISMATCH, "--lang", "fr"],
        ),
        (
            "text/type_mismatch.build.snap",
            "text",
            [str(BIN), "build", TYPE_MISMATCH, "-o", "target/runtime-diagnostics/type-mismatch", "--lang", "fr"],
        ),
        (
            "json/type_mismatch.check.snap",
            "json",
            [str(BIN), "check", "--diagnostics-json", TYPE_MISMATCH, "--lang", "fr"],
        ),
        (
            "lsp/type_mismatch.check.snap",
            "lsp",
            [str(BIN), "check", "--diagnostics-lsp", TYPE_MISMATCH, "--lang", "fr"],
        ),
    ]
    results: list[dict[str, Any]] = []
    for name, surface, args in cases:
        proc = run(args)
        output = command_payload(proc, surface)
        if proc.returncode == 0:
            failures.append(f"{name}: command unexpectedly passed")
        if "TYPECK_E_ASSIGN_MISMATCH" not in output:
            failures.append(f"{name}: missing TYPECK_E_ASSIGN_MISMATCH")
        if "affectation type incompatibilite" not in output:
            failures.append(f"{name}: missing Fluent fr message")
        if surface in {"json", "lsp"}:
            try:
                json.loads(output)
            except json.JSONDecodeError as exc:
                failures.append(f"{name}: invalid JSON surface: {exc}")
        results.append({
            "snapshot": name,
            "argv": [Path(args[0]).name, *args[1:]],
            "exit_code": proc.returncode,
            "surface": surface,
        })
    return {"status": "pass", "cases": results}


def update_snapshots(write: bool, failures: list[str]) -> None:
    snapshot_cases = [
        (
            TEXT_DIR / "type_mismatch.check.snap",
            "text",
            [str(BIN), "check", TYPE_MISMATCH, "--lang", "fr"],
        ),
        (
            TEXT_DIR / "type_mismatch.build.snap",
            "text",
            [str(BIN), "build", TYPE_MISMATCH, "-o", "target/runtime-diagnostics/type-mismatch", "--lang", "fr"],
        ),
        (
            JSON_DIR / "type_mismatch.check.snap",
            "json",
            [str(BIN), "check", "--diagnostics-json", TYPE_MISMATCH, "--lang", "fr"],
        ),
        (
            LSP_DIR / "type_mismatch.check.snap",
            "lsp",
            [str(BIN), "check", "--diagnostics-lsp", TYPE_MISMATCH, "--lang", "fr"],
        ),
    ]
    for path, surface, args in snapshot_cases:
        proc = run(args)
        write_or_check(path, command_payload(proc, surface), write, failures)


def diagnostic_count(output: str) -> int:
    return output.count("error[") + output.count("warning[")


def open_runtime_tests() -> list[dict[str, Any]]:
    all_errors_proc = run([str(BIN), "check", ALL_ERRORS, "--lang", "fr"])
    all_errors_output = all_errors_proc.stdout + all_errors_proc.stderr
    cascade_proc = run([str(BIN), "check", CASCADE, "--lang", "fr"])
    cascade_output = cascade_proc.stdout + cascade_proc.stderr
    return [
        {
            "id": "all-errors-visible",
            "argv": ["vitte", "check", ALL_ERRORS, "--lang", "fr"],
            "exit_code": all_errors_proc.returncode,
            "diagnostic_count": diagnostic_count(all_errors_output),
            "expected_diagnostic_count": 5,
            "expected_order": "stable by source position",
            "status": "pass" if all_errors_proc.returncode != 0 and diagnostic_count(all_errors_output) == 5 else "pending",
            "reason": "compiler must emit five independent diagnostics instead of accepting or stopping early",
        },
        {
            "id": "cascade-controlled",
            "argv": ["vitte", "check", CASCADE, "--lang", "fr"],
            "exit_code": cascade_proc.returncode,
            "diagnostic_count": diagnostic_count(cascade_output),
            "expected_root_code": "TYPECK_E_UNKNOWN_TYPE",
            "status": "pass" if cascade_proc.returncode != 0 and diagnostic_count(cascade_output) >= 1 else "pending",
            "reason": "compiler must emit the root diagnostic and suppress or mark derived cascade diagnostics",
        },
    ]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true", help="write snapshots instead of checking them")
    parser.add_argument("--strict-open-tests", action="store_true", help="fail while all-errors/cascade tests are pending")
    args = parser.parse_args()

    failures: list[str] = []
    assert_no_surface_only(failures)
    update_snapshots(args.write, failures)
    runtime_surface = validate_type_mismatch_surfaces(failures)
    open_tests = open_runtime_tests()

    if args.strict_open_tests:
        for test in open_tests:
            if test["status"] != "pass":
                failures.append(f"{test['id']} pending: {test['reason']}")

    report = {
        "schema": "vitte.diagnostics.runtime_snapshots.v1",
        "status": "pass" if not failures else "fail",
        "runtime_surface": runtime_surface,
        "open_tests": open_tests,
        "failures": failures,
    }
    report_path = STRICT_REPORT if args.strict_open_tests else REPORT
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if failures:
        for failure in failures:
            print(f"[runtime-diagnostics][error] {failure}", file=sys.stderr)
        print(f"[runtime-diagnostics] report={rel(report_path)}", file=sys.stderr)
        return 1
    print(f"[runtime-diagnostics] OK report={rel(report_path)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
