#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin" / "vitte"
REPORT = ROOT / "target" / "reports" / "build_fluent_diagnostics" / "coverage.json"
OUTPUT_ROOT = ROOT / "target" / "build_fluent_diagnostics"
RAW_ERROR_MARKER = "[vitte][error]"
TEXT_CODE_RE = re.compile(r"(?:error|fatal)\[([A-Z0-9_]+)\]")


@dataclass(frozen=True)
class Case:
    case_id: str
    argv: tuple[str, ...]
    expected_codes: tuple[str, ...]


def fixture(path: str) -> str:
    return path


CASES = (
    Case("build-output-required", ("build", fixture("tests/golden/frontend/fixtures/hello_min.vit")), ("E_CLI_MISSING_ARG",)),
    Case("file-missing", ("build", "target/fixtures/does-not-exist.vit", "-o", "target/build_fluent_diagnostics/missing"), ("E_CLI_IO",)),
    Case("output-value-missing", ("build", fixture("tests/golden/frontend/fixtures/hello_min.vit"), "-o"), ("E_CLI_MISSING_ARG",)),
    Case("output-directory-missing", ("build", fixture("tests/golden/frontend/fixtures/hello_min.vit"), "-o", "target/build_fluent_diagnostics/not-a-directory/out"), ("E_IO_DIRECTORY_NOT_FOUND",)),
    Case("output-overwrites-source", ("build", fixture("tests/golden/frontend/fixtures/hello_min.vit"), "-o", fixture("tests/golden/frontend/fixtures/hello_min.vit")), ("E_IO_OVERWRITE_FORBIDDEN",)),
    Case("unknown-option", ("build", fixture("tests/golden/frontend/fixtures/hello_min.vit"), "--not-a-vitte-option"), ("E_CLI_UNKNOWN_OPTION",)),
    Case("lexer-invalid", ("build", fixture("tests/compiler_real_diagnostics/invalid/lexer/lexer_invalid.vit"), "-o", "target/build_fluent_diagnostics/lexer"), ("LEX_E_INVALID_CHAR",)),
    Case("parser-invalid", ("build", fixture("tests/compiler_real_diagnostics/invalid/parser/parser_invalid.vit"), "-o", "target/build_fluent_diagnostics/parser"), ("PARSE_E_UNCLOSED_BLOCK",)),
    Case("import-missing", ("build", fixture("tests/compiler_real_diagnostics/invalid/resolver/use_import_invalid.vit"), "-o", "target/build_fluent_diagnostics/import"), ("MOD_E_MODULE_NOT_FOUND",)),
    Case("symbol-unknown", ("build", fixture("tests/compiler_real_diagnostics/invalid/sema/unknown_symbol.vit"), "-o", "target/build_fluent_diagnostics/symbol"), ("SEMA_E_UNKNOWN_IDENTIFIER",)),
    Case("type-mismatch", ("build", fixture("tests/compiler_real_diagnostics/invalid/typeck/types_incompatible.vit"), "-o", "target/build_fluent_diagnostics/type"), ("TYPECK_E_ASSIGN_MISMATCH",)),
    Case("return-mismatch", ("build", fixture("tests/compiler_real_diagnostics/invalid/typeck/return_invalid.vit"), "-o", "target/build_fluent_diagnostics/return"), ("TYPECK_E_RETURN_MISMATCH",)),
    Case("borrow-invalid", ("build", fixture("tests/compiler_real_diagnostics/invalid/borrowck/borrowck_use_after_move.vit"), "-o", "target/build_fluent_diagnostics/borrow"), ("BORROWCK_E_USE_AFTER_MOVE",)),
    Case("mir-invalid", ("build", fixture("tests/compiler_real_diagnostics/invalid/mir/mir_validation_invalid.vit"), "-o", "target/build_fluent_diagnostics/mir"), ("MIR_E_VERIFICATION_FAILED",)),
    Case("ir-invalid", ("build", fixture("tests/compiler_real_diagnostics/invalid/ir/ir_validation_invalid.vit"), "-o", "target/build_fluent_diagnostics/ir"), ("IR_E_VERIFY_FAILED",)),
    Case("backend-unsupported", ("build", fixture("tests/golden/frontend/fixtures/hello_min.vit"), "-o", "target/build_fluent_diagnostics/backend", "--target", "unsupported-target"), ("BACKEND_E_UNSUPPORTED_TARGET",)),
    Case("linker-output-not-materialized", ("build", fixture("tests/golden/frontend/fixtures/hello_min.vit"), "-o", "target/build_fluent_diagnostics/linker-missing", "--no-native"), ("LINK_E_OUTPUT_NOT_MATERIALIZED",)),
)


def command(case: Case, surface: str) -> list[str]:
    argv = [str(BIN), case.argv[0], "--lang", "fr", *case.argv[1:]]
    if surface == "json":
        argv.insert(2, "--diagnostics-json")
    elif surface == "lsp":
        argv.insert(2, "--diagnostics-lsp")
    return argv


def run(case: Case, surface: str) -> dict[str, Any]:
    proc = subprocess.run(
        command(case, surface),
        cwd=ROOT,
        env={**os.environ, "VITTE_LANG": "fr"},
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "argv": ["bin/vitte", *command(case, surface)[1:]],
        "exit_code": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
    }


def required_diagnostic_fields(value: dict[str, Any]) -> list[str]:
    missing: list[str] = []
    for key in ("code", "id", "category", "severity", "fluent_key", "message", "span", "labels", "cause", "help", "fix", "example", "suggestions"):
        if value.get(key) in (None, "", []):
            missing.append(key)
    return missing


def validate_text(case: Case, result: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    output = result["stdout"] + result["stderr"]
    if result["exit_code"] == 0:
        failures.append("invalid build exited with zero")
    if RAW_ERROR_MARKER in output:
        failures.append("raw [vitte][error] output is forbidden")
    codes = tuple(TEXT_CODE_RE.findall(output))
    if codes != case.expected_codes:
        failures.append(f"diagnostic order mismatch: expected {case.expected_codes}, got {codes}")
    for marker in ("= id:", "= category:", "= severity:", "= fluent-key:", "= span:", "= label:", "= cause:", "= help:", "= fix-it:", "= corrected example:"):
        if marker not in output:
            failures.append(f"missing rich text marker {marker}")
    return failures


def validate_json(case: Case, result: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    if result["exit_code"] == 0:
        failures.append("invalid JSON build exited with zero")
    if result["stderr"]:
        failures.append("JSON diagnostic wrote non-JSON stderr")
    try:
        payload = json.loads(result["stdout"])
    except json.JSONDecodeError as exc:
        return [f"JSON output is not parseable: {exc}"]
    diagnostics = payload.get("primary_report", {}).get("diagnostics", [])
    codes = tuple(item.get("code") for item in diagnostics if isinstance(item, dict))
    if codes != case.expected_codes:
        failures.append(f"JSON diagnostic order mismatch: expected {case.expected_codes}, got {codes}")
    for index, value in enumerate(diagnostics):
        if not isinstance(value, dict):
            failures.append(f"JSON diagnostic {index} is not an object")
            continue
        missing = required_diagnostic_fields(value)
        if missing:
            failures.append(f"JSON diagnostic {index} missing fields: {', '.join(missing)}")
    return failures


def validate_lsp(case: Case, result: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    if result["exit_code"] == 0:
        failures.append("invalid LSP build exited with zero")
    if result["stderr"]:
        failures.append("LSP diagnostic wrote non-JSON stderr")
    try:
        payload = json.loads(result["stdout"])
    except json.JSONDecodeError as exc:
        return [f"LSP output is not parseable JSON: {exc}"]
    if payload.get("jsonrpc") != "2.0" or payload.get("method") != "textDocument/publishDiagnostics":
        failures.append("LSP output is not a publishDiagnostics JSON-RPC notification")
    diagnostics = payload.get("params", {}).get("diagnostics", [])
    codes = tuple(item.get("code") for item in diagnostics if isinstance(item, dict))
    if codes != case.expected_codes:
        failures.append(f"LSP diagnostic order mismatch: expected {case.expected_codes}, got {codes}")
    for index, value in enumerate(diagnostics):
        if not isinstance(value, dict):
            failures.append(f"LSP diagnostic {index} is not an object")
            continue
        for key in ("range", "severity", "code", "source", "message", "relatedInformation", "data"):
            if value.get(key) in (None, "", []):
                failures.append(f"LSP diagnostic {index} missing {key}")
        data = value.get("data", {})
        if not isinstance(data, dict):
            failures.append(f"LSP diagnostic {index} data is not an object")
            continue
        for key in ("code", "id", "category", "severity", "fluent_key", "cause", "help", "fix", "example", "code_action_title"):
            if data.get(key) in (None, ""):
                failures.append(f"LSP diagnostic {index} data missing {key}")
    return failures


def main() -> int:
    OUTPUT_ROOT.mkdir(parents=True, exist_ok=True)
    if not BIN.exists():
        report = {
            "schema": "vitte.build-fluent-diagnostics.coverage.v1",
            "binary": "bin/vitte",
            "locale": "fr",
            "case_count": len(CASES),
            "passed_count": 0,
            "failure_count": 1,
            "cases": [],
            "status": "failed",
            "blocker": "runtime compiler artifact is missing",
        }
        REPORT.parent.mkdir(parents=True, exist_ok=True)
        REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print(
            f"[build-fluent-diagnostics-gate][error] missing runtime compiler artifact: {BIN.relative_to(ROOT)}; report={REPORT.relative_to(ROOT)}",
            file=sys.stderr,
        )
        return 1
    output_blocker = OUTPUT_ROOT / "not-a-directory"
    if output_blocker.is_dir():
        print(f"[build-fluent-diagnostics-gate][error] expected a file at {output_blocker.relative_to(ROOT)}, found a directory", file=sys.stderr)
        return 1
    output_blocker.write_text("blocks output directory creation\n", encoding="utf-8")
    linker_output = OUTPUT_ROOT / "linker-missing"
    linker_output.unlink(missing_ok=True)
    rows: list[dict[str, Any]] = []
    failure_count = 0
    validators = {"text": validate_text, "json": validate_json, "lsp": validate_lsp}
    for case in CASES:
        surfaces: dict[str, Any] = {}
        for surface, validator in validators.items():
            result = run(case, surface)
            failures = validator(case, result)
            failure_count += len(failures)
            surfaces[surface] = {**result, "failures": failures, "passed": not failures}
        rows.append({
            "id": case.case_id,
            "expected_codes": list(case.expected_codes),
            "surfaces": surfaces,
            "passed": all(value["passed"] for value in surfaces.values()),
        })

    report = {
        "schema": "vitte.build-fluent-diagnostics.coverage.v1",
        "binary": "bin/vitte",
        "locale": "fr",
        "case_count": len(CASES),
        "passed_count": sum(1 for row in rows if row["passed"]),
        "failure_count": failure_count,
        "cases": rows,
        "status": "ok" if failure_count == 0 else "failed",
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    if failure_count:
        print(
            f"[build-fluent-diagnostics-gate][error] passed={report['passed_count']}/{len(CASES)} failures={failure_count}; report={REPORT.relative_to(ROOT)}",
            file=sys.stderr,
        )
        return 1
    print(f"[build-fluent-diagnostics-gate] passed={len(CASES)}/{len(CASES)} report={REPORT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
