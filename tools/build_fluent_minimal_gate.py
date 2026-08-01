#!/usr/bin/env python3
from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin/vitte"
FIXTURE = "tests/pipeline/hello_world.vit"
REPORT = ROOT / "target/reports/build_fluent_diagnostics/minimal.json"
EXPECTED_CODE = "E_CLI_MISSING_ARG"
RAW_PREFIX = "[vitte][error]"
RAW_IO_TEXT = "E_CLI_IO: cannot read"


def run(surface: str) -> dict[str, Any]:
    argv = [str(BIN), "build"]
    if surface == "json":
        argv.append("--diagnostics-json")
    elif surface == "lsp":
        argv.append("--diagnostics-lsp")
    argv.append(FIXTURE)
    proc = subprocess.run(
        argv,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "argv": ["bin/vitte", *argv[1:]],
        "exit_code": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
    }


def validate(surface: str, result: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    combined = str(result["stdout"]) + str(result["stderr"])
    if result["exit_code"] == 0:
        failures.append("build without -o unexpectedly succeeded")
    if RAW_PREFIX in combined:
        failures.append("raw diagnostic prefix is present")
    if RAW_IO_TEXT in combined:
        failures.append("raw E_CLI_IO diagnostic text is present")
    if surface == "text":
        if f"error[{EXPECTED_CODE}]" not in combined:
            failures.append(f"missing error[{EXPECTED_CODE}]")
        for marker in ("= id:", "= category:", "= severity:", "= fluent-key:", "= span:", "= label:", "= cause:", "= help:", "= fix-it:", "= corrected example:"):
            if marker not in combined:
                failures.append(f"missing rich text marker {marker}")
        return failures
    if result["stderr"]:
        failures.append(f"{surface} surface wrote to stderr")
    try:
        payload = json.loads(str(result["stdout"]))
    except json.JSONDecodeError as exc:
        failures.append(f"{surface} output is not parseable JSON: {exc}")
        return failures
    if surface == "json":
        diagnostics = payload.get("primary_report", {}).get("diagnostics", [])
        if not diagnostics or diagnostics[0].get("code") != EXPECTED_CODE:
            failures.append("JSON primary diagnostic code mismatch")
        elif any(diagnostics[0].get(key) in (None, "", []) for key in ("id", "category", "severity", "fluent_key", "message", "span", "labels", "cause", "help", "fix", "example", "suggestions")):
            failures.append("JSON primary diagnostic is missing rich fields")
    else:
        if payload.get("jsonrpc") != "2.0":
            failures.append("LSP jsonrpc must be 2.0")
        if payload.get("method") != "textDocument/publishDiagnostics":
            failures.append("LSP method must be textDocument/publishDiagnostics")
        diagnostics = payload.get("params", {}).get("diagnostics", [])
        if not diagnostics or diagnostics[0].get("code") != EXPECTED_CODE:
            failures.append("LSP primary diagnostic code mismatch")
        elif any(diagnostics[0].get(key) in (None, "", []) for key in ("range", "severity", "source", "message", "relatedInformation", "data")):
            failures.append("LSP primary diagnostic is missing rich fields")
    return failures


def main() -> int:
    rows: dict[str, Any] = {}
    runtime_state = "binary-missing"
    failure_count = 0
    if not BIN.is_file():
        rows["runtime"] = {"failures": ["bin/vitte is missing"], "passed": False}
        failure_count = 1
    else:
        binary = BIN.read_bytes()
        runtime_state = "runtime-obsolete" if b"[vitte][error]" in binary or b"E_CLI_IO: cannot read" in binary or (b"_command_build" in binary and b"run_cli_main" not in binary) else "runtime-current"
        if runtime_state == "runtime-obsolete":
            failure_count += 1
        for surface in ("text", "json", "lsp"):
            result = run(surface)
            failures = validate(surface, result)
            rows[surface] = {**result, "failures": failures, "passed": not failures}
            failure_count += len(failures)
    report = {
        "schema": "vitte.build-fluent-diagnostics.minimal.v1",
        "status": "ok" if failure_count == 0 else "failed",
        "fixture": FIXTURE,
        "expected_code": EXPECTED_CODE,
        "runtime_state": runtime_state,
        "surfaces": rows,
        "failure_count": failure_count,
    }
    if runtime_state == "runtime-obsolete":
        report["blocker"] = "runtime compiler artifact does not execute the current Vitte driver"
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failure_count:
        print(
            f"[build-fluent-minimal-gate][error] failures={failure_count} report={REPORT.relative_to(ROOT)}",
            file=sys.stderr,
        )
        return 1
    print(f"[build-fluent-minimal-gate] status=ok report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
