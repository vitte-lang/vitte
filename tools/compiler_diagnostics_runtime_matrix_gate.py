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
INVALID_ROOT = ROOT / "tests" / "compiler_real_diagnostics" / "invalid"
INVALID_ROOT_REL = Path("tests/compiler_real_diagnostics/invalid")
PHASE_DIRS = {
    "lexer",
    "parser",
    "resolver",
    "sema",
    "typeck",
    "borrowck",
    "mir",
    "ir",
    "backend",
}
REQUIRED_MANIFEST_FIELDS = {
    "id",
    "phase",
    "entry",
    "command",
    "expected_codes",
    "expected_order",
    "expected_locale",
    "expected_exit_code",
    "cascade_policy",
}
VALID_COMMANDS = {"check", "build"}
VALID_EXPECTED_ORDERS = {"source-order", "root-cause-order"}
VALID_CASCADE_POLICIES = {"none", "suppress-derived", "mark-derived"}


def manifest_case(
    case_id: str,
    phase: str,
    entry: str,
    command: str,
    expected_codes: list[str],
    *,
    cascade_policy: str = "none",
    expected_order: str = "source-order",
) -> dict[str, Any]:
    return {
        "id": case_id,
        "phase": phase,
        "entry": entry,
        "command": command,
        "expected_codes": expected_codes,
        "expected_order": expected_order,
        "expected_locale": LOCALE,
        "expected_exit_code": 1,
        "cascade_policy": cascade_policy,
    }


CASES: list[dict[str, Any]] = [
    manifest_case("lexer-invalid", "lexer", "tests/compiler_real_diagnostics/invalid/lexer/lexer_invalid.vit", "check", ["LEX_E_INVALID_CHAR"]),
    manifest_case("parser-invalid", "parser", "tests/compiler_real_diagnostics/invalid/parser/parser_invalid.vit", "check", ["PARSE_E_UNCLOSED_BLOCK"]),
    manifest_case("use-import-invalid", "resolver", "tests/compiler_real_diagnostics/invalid/resolver/use_import_invalid.vit", "check", ["MOD_E_MODULE_NOT_FOUND"]),
    manifest_case("export-invalid", "sema", "tests/compiler_real_diagnostics/invalid/sema/export_invalid.vit", "check", ["SEMA_E_INVALID_EXPORT"]),
    manifest_case("proc-invalid", "parser", "tests/compiler_real_diagnostics/invalid/parser/proc_invalid.vit", "check", ["PARSE_E_PARAMETER_COLON_EXPECTED"]),
    manifest_case("form-invalid", "sema", "tests/compiler_real_diagnostics/invalid/sema/form_invalid.vit", "check", ["AST_E_DUPLICATE_FIELD"]),
    manifest_case("pick-invalid", "sema", "tests/compiler_real_diagnostics/invalid/sema/pick_invalid.vit", "check", ["SEMA_E_DUPLICATE_PICK_BRANCH"]),
    manifest_case("const-invalid", "sema", "tests/compiler_real_diagnostics/invalid/sema/const_invalid.vit", "check", ["CONST_EVAL_E_DIVISION_BY_ZERO"]),
    manifest_case("symbol-unknown", "sema", "tests/compiler_real_diagnostics/invalid/sema/unknown_symbol.vit", "check", ["SEMA_E_UNKNOWN_IDENTIFIER"]),
    manifest_case("type-unknown", "typeck", "tests/diagnostics/runtime/fixtures/cascade_controlled.vit", "check", ["TYPECK_E_UNKNOWN_TYPE"], cascade_policy="suppress-derived", expected_order="root-cause-order"),
    manifest_case("type-incompatible", "typeck", "tests/compiler_real_diagnostics/invalid/typeck/types_incompatible.vit", "check", ["TYPECK_E_ASSIGN_MISMATCH"]),
    manifest_case("call-invalid", "typeck", "tests/compiler_real_diagnostics/invalid/typeck/call_invalid.vit", "check", ["TYPECK_E_CALL_ARITY"]),
    manifest_case("return-invalid", "typeck", "tests/compiler_real_diagnostics/invalid/typeck/return_invalid.vit", "check", ["TYPECK_E_RETURN_MISMATCH"]),
    manifest_case("borrow-move-invalid", "borrowck", "tests/compiler_real_diagnostics/invalid/borrowck/borrowck_use_after_move.vit", "check", ["BORROWCK_E_USE_AFTER_MOVE"]),
    manifest_case("mir-validation-invalid", "mir", "tests/compiler_real_diagnostics/invalid/mir/mir_validation_invalid.vit", "check", ["MIR_E_VERIFICATION_FAILED"]),
    manifest_case("ir-validation-invalid", "ir", "tests/compiler_real_diagnostics/invalid/ir/ir_validation_invalid.vit", "check", ["IR_E_VERIFY_FAILED"]),
    manifest_case("backend-unsupported", "backend", "tests/compiler_real_diagnostics/invalid/backend/backend_unsupported_target.vit", "build", ["BACKEND_E_UNSUPPORTED_TARGET"]),
    manifest_case("multi-errors-one-file", "typeck", "tests/compiler_real_diagnostics/invalid/typeck/multi_errors_one_file.vit", "check", ["TYPECK_E_ASSIGN_MISMATCH", "SEMA_E_UNKNOWN_IDENTIFIER", "TYPECK_E_CALL_ARITY"]),
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
    command = [str(BIN), str(case["command"])]
    if surface == "json":
        command.append("--diagnostics-json")
    if surface == "lsp":
        command.append("--diagnostics-lsp")
    command.append(str(case["entry"]))
    if case["command"] == "build":
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


def validate_manifest_case(case: dict[str, Any]) -> list[str]:
    failures: list[str] = []
    missing = sorted(REQUIRED_MANIFEST_FIELDS - set(case))
    if missing:
        failures.append(f"case {case.get('id', '<missing-id>')} missing required fields: {missing}")
        return failures
    if not isinstance(case["id"], str) or not case["id"]:
        failures.append("case id must be a non-empty string")
    if case["phase"] not in PHASE_DIRS:
        failures.append(f"case {case['id']} phase must be one of {sorted(PHASE_DIRS)}")
    if case["command"] not in VALID_COMMANDS:
        failures.append(f"case {case['id']} command must be one of {sorted(VALID_COMMANDS)}")
    if not isinstance(case["expected_codes"], list) or not case["expected_codes"]:
        failures.append(f"case {case['id']} expected_codes must be a non-empty ordered list")
    elif not all(isinstance(code, str) and code for code in case["expected_codes"]):
        failures.append(f"case {case['id']} expected_codes must contain only non-empty strings")
    if case["expected_order"] not in VALID_EXPECTED_ORDERS:
        failures.append(f"case {case['id']} expected_order must be one of {sorted(VALID_EXPECTED_ORDERS)}")
    if case["expected_locale"] != LOCALE:
        failures.append(f"case {case['id']} expected_locale must be {LOCALE!r}")
    if not isinstance(case["expected_exit_code"], int) or case["expected_exit_code"] == 0:
        failures.append(f"case {case['id']} expected_exit_code must be a non-zero integer")
    if case["cascade_policy"] not in VALID_CASCADE_POLICIES:
        failures.append(f"case {case['id']} cascade_policy must be one of {sorted(VALID_CASCADE_POLICIES)}")
    return failures


def invalid_fixture_phase(path: Path) -> str | None:
    try:
        relative = path.relative_to(INVALID_ROOT)
    except ValueError:
        return None
    if len(relative.parts) < 2:
        return None
    phase = relative.parts[0]
    if phase not in PHASE_DIRS:
        return None
    return phase


def validate_invalid_fixture_layout() -> list[str]:
    failures: list[str] = []
    if not INVALID_ROOT.is_dir():
        return [f"missing invalid fixture root: {rel(INVALID_ROOT)}"]

    for phase in sorted(PHASE_DIRS):
        if not (INVALID_ROOT / phase).is_dir():
            failures.append(f"missing invalid fixture phase directory: {rel(INVALID_ROOT / phase)}")

    for path in sorted(INVALID_ROOT.rglob("*.vit")):
        if invalid_fixture_phase(path) is None:
            failures.append(f"invalid fixture outside phase directory: {rel(path)}")

    covered_phases: set[str] = set()
    for case in CASES:
        entry = Path(str(case["entry"]))
        try:
            relative = entry.relative_to(INVALID_ROOT_REL)
        except ValueError:
            continue
        if len(relative.parts) < 2 or relative.parts[0] not in PHASE_DIRS:
            failures.append(f"case {case['id']} entry is not phase-scoped: {case['entry']}")
            continue
        if case.get("phase") != relative.parts[0]:
            failures.append(f"case {case['id']} phase {case.get('phase')!r} does not match entry phase {relative.parts[0]!r}")
        covered_phases.add(relative.parts[0])

    missing_coverage = sorted(PHASE_DIRS - covered_phases)
    if missing_coverage:
        failures.append(f"phase directories without runtime matrix case: {missing_coverage}")

    return failures


def validate_case(case: dict[str, Any], messages: dict[str, str]) -> dict[str, Any]:
    raw_expected = case.get("expected_codes", [])
    expected = list(raw_expected) if isinstance(raw_expected, list) else []
    failures: list[str] = []
    failures.extend(validate_manifest_case(case))
    if failures:
        return {
            "id": case.get("id"),
            "phase": case.get("phase"),
            "command": case.get("command"),
            "entry": case.get("entry"),
            "expected_codes": expected if "expected_codes" in case else [],
            "text_codes": [],
            "json_codes": [],
            "lsp_codes": [],
            "status": "fail",
            "failures": failures,
            "commands": {},
        }
    text = run(case, "text")
    js = run(case, "json")
    lsp = run(case, "lsp")

    if not (ROOT / str(case["entry"])).is_file():
        failures.append(f"fixture missing: {case['entry']}")
    for surface_name, result in (("text", text), ("json", js), ("lsp", lsp)):
        if result["exit_code"] != case["expected_exit_code"]:
            failures.append(
                f"{surface_name} exit code mismatch: expected {case['expected_exit_code']}, got {result['exit_code']}"
            )

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
        "phase": case["phase"],
        "command": case["command"],
        "entry": case["entry"],
        "expected_codes": expected,
        "expected_order": case["expected_order"],
        "expected_locale": case["expected_locale"],
        "expected_exit_code": case["expected_exit_code"],
        "cascade_policy": case["cascade_policy"],
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
        "| case | phase | command | status | codes |",
        "| --- | --- | --- | --- | --- |",
    ]
    for result in results:
        lines.append(f"| `{result['id']}` | `{result['phase']}` | `{result['command']}` | `{result['status']}` | `{', '.join(result['expected_codes'])}` |")
    if failures:
        lines.extend(["", "## Failures"])
        lines.extend(f"- {failure}" for failure in failures[:120])
    REPORT_MD.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    if not BIN.is_file():
        print(f"[compiler-diagnostics-runtime-matrix][error] missing {rel(BIN)}", file=sys.stderr)
        return 1
    messages = parse_ftl(FTL)
    layout_failures = validate_invalid_fixture_layout()
    results = [validate_case(case, messages) for case in CASES]
    failures = [*layout_failures, *[f"{result['id']}: {failure}" for result in results for failure in result["failures"]]]
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
