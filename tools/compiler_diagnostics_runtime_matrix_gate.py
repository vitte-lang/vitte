#!/usr/bin/env python3
from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin" / "vitte"
REPORT_DIR = ROOT / "target" / "reports" / "compiler_diagnostics_runtime_matrix"
REPORT_JSON = REPORT_DIR / "coverage.json"
REPORT_MD = REPORT_DIR / "coverage.md"
LOCALE = "fr"
FTL = ROOT / "locales" / LOCALE / "diagnostics.ftl"
ERROR_CODE_RE = re.compile(r"error\[([A-Z0-9_]+)\]")


CASES: list[dict[str, Any]] = [
    {"id": "lexer-invalid", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/lexer_invalid.vit", "codes": ["LEX_E_INVALID_CHAR"]},
    {"id": "parser-invalid", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/parser_invalid.vit", "codes": ["PARSE_E_UNCLOSED_BLOCK"]},
    {"id": "use-import-invalid", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/use_import_invalid.vit", "codes": ["MOD_E_MODULE_NOT_FOUND"]},
    {"id": "export-invalid", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/export_invalid.vit", "codes": ["SEMA_E_INVALID_EXPORT"]},
    {"id": "proc-invalid", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/proc_invalid.vit", "codes": ["PARSE_E_PARAMETER_COLON_EXPECTED"]},
    {"id": "form-invalid", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/form_invalid.vit", "codes": ["AST_E_DUPLICATE_FIELD"]},
    {"id": "pick-invalid", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/pick_invalid.vit", "codes": ["SEMA_E_DUPLICATE_PICK_BRANCH"]},
    {"id": "const-invalid", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/const_invalid.vit", "codes": ["CONST_EVAL_E_DIVISION_BY_ZERO"]},
    {"id": "symbol-unknown", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/unknown_symbol.vit", "codes": ["SEMA_E_UNKNOWN_IDENTIFIER"]},
    {"id": "type-unknown", "kind": "check", "entry": "tests/diagnostics/runtime/fixtures/cascade_controlled.vit", "codes": ["TYPECK_E_UNKNOWN_TYPE"]},
    {"id": "type-incompatible", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/types_incompatible.vit", "codes": ["TYPECK_E_ASSIGN_MISMATCH"]},
    {"id": "call-invalid", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/call_invalid.vit", "codes": ["TYPECK_E_CALL_ARITY"]},
    {"id": "return-invalid", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/return_invalid.vit", "codes": ["TYPECK_E_RETURN_MISMATCH"]},
    {"id": "borrow-move-invalid", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/borrowck_use_after_move.vit", "codes": ["BORROWCK_E_USE_AFTER_MOVE"]},
    {"id": "mir-validation-invalid", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/mir_validation_invalid.vit", "codes": ["MIR_E_VERIFICATION_FAILED"]},
    {"id": "backend-unsupported", "kind": "build", "entry": "tests/compiler_real_diagnostics/invalid/backend_unsupported_target.vit", "codes": ["BACKEND_E_UNSUPPORTED_TARGET"]},
    {"id": "multi-errors-one-file", "kind": "check", "entry": "tests/compiler_real_diagnostics/invalid/multi_errors_one_file.vit", "codes": ["TYPECK_E_ASSIGN_MISMATCH", "SEMA_E_UNKNOWN_IDENTIFIER", "TYPECK_E_CALL_ARITY"]},
]


TEXT_MARKERS = (
    "id:",
    "category:",
    "severity:",
    "fluent-key:",
    "span:",
    "label:",
    "cause:",
    "help:",
    "fix-it:",
    "corrected example:",
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def parse_ftl(path: Path) -> dict[str, str]:
    rows: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        rows[key.strip()] = value.strip()
    return rows


def command_for(case: dict[str, Any], surface: str) -> list[str]:
    command = [str(BIN), str(case["kind"])]
    if surface == "json":
        command.append("--diagnostics-json")
    if surface == "lsp":
        command.append("--diagnostics-lsp")
    command.append(str(case["entry"]))
    if case["kind"] == "build":
        command.extend(["-o", f"target/runtime-diagnostics-matrix/{case['id']}"])
    command.extend(["--lang", LOCALE])
    return command


def run(case: dict[str, Any], surface: str) -> dict[str, Any]:
    proc = subprocess.run(
        command_for(case, surface),
        cwd=ROOT,
        env={**os.environ, "VITTE_LANG": LOCALE},
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    return {
        "argv": ["bin/vitte", *command_for(case, surface)[1:]],
        "exit_code": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "combined": proc.stdout + proc.stderr,
    }


def text_codes(output: str) -> list[str]:
    return ERROR_CODE_RE.findall(output)


def json_codes(output: str) -> tuple[list[str], list[str]]:
    failures: list[str] = []
    try:
        payload = json.loads(output)
    except json.JSONDecodeError as exc:
        return [], [f"JSON is not parseable: {exc}"]
    diagnostics = payload.get("primary_report", {}).get("diagnostics", [])
    if not isinstance(diagnostics, list) or not diagnostics:
        return [], ["JSON primary_report.diagnostics missing"]
    codes: list[str] = []
    for index, diagnostic in enumerate(diagnostics):
        if not isinstance(diagnostic, dict):
            failures.append(f"JSON diagnostic {index} is not object")
            continue
        codes.append(str(diagnostic.get("code", "")))
        for key in ("id", "category", "severity", "fluent_key", "span", "labels", "cause", "fix", "example", "suggestions"):
            if not diagnostic.get(key):
                failures.append(f"JSON diagnostic {index} missing {key}")
    return codes, failures


def lsp_codes(output: str) -> tuple[list[str], list[str]]:
    failures: list[str] = []
    try:
        payload = json.loads(output)
    except json.JSONDecodeError as exc:
        return [], [f"LSP is not parseable JSON: {exc}"]
    if payload.get("jsonrpc") != "2.0":
        failures.append("LSP jsonrpc must be 2.0")
    if payload.get("method") != "textDocument/publishDiagnostics":
        failures.append("LSP method must be publishDiagnostics")
    diagnostics = payload.get("params", {}).get("diagnostics", [])
    if not isinstance(diagnostics, list) or not diagnostics:
        return [], [*failures, "LSP diagnostics missing"]
    codes: list[str] = []
    for index, diagnostic in enumerate(diagnostics):
        if not isinstance(diagnostic, dict):
            failures.append(f"LSP diagnostic {index} is not object")
            continue
        codes.append(str(diagnostic.get("code", "")))
        if diagnostic.get("severity") != 1:
            failures.append(f"LSP diagnostic {index} severity must be error")
        if not diagnostic.get("range"):
            failures.append(f"LSP diagnostic {index} missing range")
        data = diagnostic.get("data")
        if not isinstance(data, dict):
            failures.append(f"LSP diagnostic {index} missing data")
            continue
        for key in ("id", "category", "severity", "fluent_key", "cause", "fix", "example"):
            if not data.get(key):
                failures.append(f"LSP diagnostic {index} data missing {key}")
    return codes, failures


def validate_codes(actual: list[str], expected: list[str], label: str) -> list[str]:
    failures: list[str] = []
    if actual != expected:
        failures.append(f"{label} diagnostic order mismatch: expected {expected}, got {actual}")
    if len(actual) != len(set(f"{code}:{index}" for index, code in enumerate(actual))):
        failures.append(f"{label} diagnostic identity is unstable")
    parasite = [code for code in actual if code not in expected]
    if parasite:
        failures.append(f"{label} parasite diagnostics: {parasite}")
    return failures


def validate_case(case: dict[str, Any], messages: dict[str, str]) -> dict[str, Any]:
    expected = list(case["codes"])
    failures: list[str] = []
    text = run(case, "text")
    js = run(case, "json")
    lsp = run(case, "lsp")

    if not (ROOT / str(case["entry"])).is_file():
        failures.append(f"fixture missing: {case['entry']}")
    for surface_name, result in (("text", text), ("json", js), ("lsp", lsp)):
        if result["exit_code"] == 0:
            failures.append(f"{surface_name} command accepted invalid fixture")

    text_output = str(text["combined"])
    codes = text_codes(text_output)
    failures.extend(validate_codes(codes, expected, "text"))
    for code in expected:
        message = messages.get(code, "")
        if message and message not in text_output:
            failures.append(f"text missing Fluent message for {code}: {message}")
        if f"fluent-key: {code}" not in text_output:
            failures.append(f"text missing fluent-key for {code}")
    for marker in TEXT_MARKERS:
        if marker not in text_output:
            failures.append(f"text missing rich marker {marker}")
    if not re.search(r"\.vit:\d+:\d+", text_output):
        failures.append("text missing real span")

    json_actual, json_failures = json_codes(str(js["stdout"] or js["combined"]))
    failures.extend(validate_codes(json_actual, expected, "JSON"))
    failures.extend(json_failures)
    lsp_actual, lsp_failures = lsp_codes(str(lsp["stdout"] or lsp["combined"]))
    failures.extend(validate_codes(lsp_actual, expected, "LSP"))
    failures.extend(lsp_failures)

    for code in expected:
        message = messages.get(code, "")
        if message and message not in str(js["stdout"] or js["combined"]):
            failures.append(f"JSON missing Fluent message for {code}: {message}")
        if message and message not in str(lsp["stdout"] or lsp["combined"]):
            failures.append(f"LSP missing Fluent message for {code}: {message}")

    return {
        "id": case["id"],
        "kind": case["kind"],
        "entry": case["entry"],
        "expected_codes": expected,
        "text_codes": codes,
        "json_codes": json_actual,
        "lsp_codes": lsp_actual,
        "status": "pass" if not failures else "fail",
        "failures": failures,
        "commands": {"text": text["argv"], "json": js["argv"], "lsp": lsp["argv"]},
    }


def write_reports(results: list[dict[str, Any]], failures: list[str]) -> None:
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    report = {
        "schema": "vitte.compiler.diagnostics.runtime_matrix.v1",
        "status": "pass" if not failures else "fail",
        "locale": LOCALE,
        "case_count": len(results),
        "passed": sum(1 for result in results if result["status"] == "pass"),
        "cases": results,
        "failures": failures,
    }
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    lines = [
        "# Compiler Diagnostics Runtime Matrix",
        "",
        f"- status: `{report['status']}`",
        f"- locale: `{LOCALE}`",
        f"- passed: `{report['passed']}/{report['case_count']}`",
        "",
        "| case | command | status | codes |",
        "| --- | --- | --- | --- |",
    ]
    for result in results:
        lines.append(f"| `{result['id']}` | `{result['kind']}` | `{result['status']}` | `{', '.join(result['expected_codes'])}` |")
    if failures:
        lines.extend(["", "## Failures"])
        lines.extend(f"- {failure}" for failure in failures[:120])
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    if not BIN.is_file():
        print(f"[compiler-diagnostics-runtime-matrix][error] missing {rel(BIN)}", file=sys.stderr)
        return 1
    messages = parse_ftl(FTL)
    results = [validate_case(case, messages) for case in CASES]
    failures = [f"{result['id']}: {failure}" for result in results for failure in result["failures"]]
    write_reports(results, failures)
    if failures:
        for failure in failures[:80]:
            print(f"[compiler-diagnostics-runtime-matrix][error] {failure}", file=sys.stderr)
        print(f"[compiler-diagnostics-runtime-matrix] report={rel(REPORT_JSON)}", file=sys.stderr)
        return 1
    print(f"[compiler-diagnostics-runtime-matrix] OK passed={len(results)}/{len(results)} report={rel(REPORT_JSON)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
