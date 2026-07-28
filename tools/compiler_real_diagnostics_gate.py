#!/usr/bin/env python3
"""Run real compiler diagnostics over a source corpus.

This gate intentionally executes `bin/vitte check` instead of validating only
catalog metadata. It is expected to fail until every listed language error is
reported by the compiler runtime with Fluent-localized rich diagnostics.
"""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CORPUS = ROOT / "tests" / "compiler_real_diagnostics" / "corpus.vitte.json"
BIN = ROOT / "bin" / "vitte"
REPORT_DIR = ROOT / "target" / "reports" / "compiler_real_diagnostics"
REPORT_JSON = REPORT_DIR / "coverage.json"
REPORT_MD = REPORT_DIR / "coverage.md"
WRAPPER = ROOT / "tools" / "vitte_cli_locale_wrapper.c"
FORBIDDEN_SURFACE_ONLY_MARKERS = (
    "REAL_DIAGNOSTIC_CASES",
    "emit_real_diagnostic_case",
    "write_real_text_diagnostic",
    "write_real_json_diagnostic",
    "write_real_lsp_diagnostic",
)

RICH_TEXT_MARKERS = (
    "id:",
    "category:",
    "severity:",
    "fluent-key:",
    "span:",
    "label:",
    "cause:",
    "help:",
    "corrected example:",
)
FIX_MARKERS = ("fix-it:", "fix:")
LOCATION_RE = re.compile(r"[^:\s]+\.vit:\d+:\d+")
DIAGNOSTIC_ID_RE = re.compile(r"^[A-Z0-9_]+:.+\.vit:\d+:\d+$")
ERROR_CODE_RE = re.compile(r"error\[([A-Z0-9_]+)\]")
TEXT_DIAGNOSTIC_ID_RE = re.compile(r"=\s*id:\s*([A-Z0-9_]+):(.+\.vit):([1-9]\d*):([1-9]\d*)")
DIAGNOSTIC_ID_SPAN_RE = re.compile(r"^([A-Z0-9_]+):(.+\.vit):([1-9]\d*):([1-9]\d*)$")
JSON_LOCATION_RE = re.compile(r"^.+\.vit:[1-9]\d*:[1-9]\d*$")
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


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def parse_ftl(path: Path) -> dict[str, str]:
    entries: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        entries[key.strip()] = value.strip()
    return entries


def expected_category_for_code(code: str) -> str:
    if code.startswith("LEX_"):
        return "lexer"
    if code.startswith("PARSE_"):
        return "parser"
    if code.startswith("AST_"):
        return "ast"
    if code.startswith(("SEMA_", "MOD_", "CONST_EVAL_")):
        return "sema"
    if code.startswith("TYPECK_"):
        return "typeck"
    if code.startswith(("BORROWCK_", "BORROW_")):
        return "borrowck"
    if code.startswith("MIR_"):
        return "mir"
    if code.startswith("IR_"):
        return "ir"
    if code.startswith(("BACKEND_", "LINK_")):
        return "backend"
    return "diagnostic"


def run_command(
    case_id: str,
    command_name: str,
    entry: str,
    locale: str,
    diagnostics_json: bool = False,
    diagnostics_lsp: bool = False,
) -> dict[str, Any]:
    args = [str(BIN), command_name]
    if diagnostics_json:
        args.append("--diagnostics-json")
    if diagnostics_lsp:
        args.append("--diagnostics-lsp")
    args.append(entry)
    if command_name == "build":
        args.extend(["-o", f"target/compiler-real-diagnostics/{case_id}"])
    args.extend(["--lang", locale])
    proc = subprocess.run(
        args,
        cwd=ROOT,
        env={**os.environ, "VITTE_LANG": locale},
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    combined = proc.stdout + proc.stderr
    return {
        "argv": [
            "bin/vitte",
            command_name,
            *(["--diagnostics-json"] if diagnostics_json else []),
            *(["--diagnostics-lsp"] if diagnostics_lsp else []),
            entry,
            *(["-o", f"target/compiler-real-diagnostics/{case_id}"] if command_name == "build" else []),
            "--lang",
            locale,
        ],
        "exit_code": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "combined": combined,
        "diagnostic_count": combined.count("error[") + combined.count("warning["),
    }


def audit_no_surface_only_diagnostics() -> list[str]:
    if not WRAPPER.exists():
        return [f"missing wrapper source: {rel(WRAPPER)}"]
    text = WRAPPER.read_text(encoding="utf-8")
    failures: list[str] = []
    for marker in FORBIDDEN_SURFACE_ONLY_MARKERS:
        if marker in text:
            failures.append(f"surface-only diagnostic marker still present in {rel(WRAPPER)}: {marker}")
    return failures


def validate_diagnostic_contract(
    diagnostic: dict[str, Any],
    expected_code: str,
    expected_message: str,
    expected_category: str,
    prefix: str,
) -> list[str]:
    failures: list[str] = []
    if diagnostic.get("code") != expected_code:
        failures.append(f"{prefix} diagnostic code mismatch: {diagnostic.get('code')!r}")
    if diagnostic.get("severity") != "error":
        failures.append(f"{prefix} diagnostic severity mismatch: {diagnostic.get('severity')!r}")
    if diagnostic.get("category") != expected_category:
        failures.append(f"{prefix} diagnostic category mismatch: {diagnostic.get('category')!r}")
    diagnostic_id = diagnostic.get("id")
    if not isinstance(diagnostic_id, str) or not DIAGNOSTIC_ID_RE.match(diagnostic_id):
        failures.append(f"{prefix} diagnostic id is not stable/unique: {diagnostic_id!r}")
    elif expected_code not in diagnostic_id:
        failures.append(f"{prefix} diagnostic id does not include code {expected_code}")
    if diagnostic.get("fluent_key") != expected_code:
        failures.append(f"{prefix} Fluent key mismatch: {diagnostic.get('fluent_key')!r}")
    if expected_message and diagnostic.get("message") != expected_message:
        failures.append(f"{prefix} diagnostic message mismatch: {diagnostic.get('message')!r}")
    return failures


def validate_json_output(
    output: str,
    expected_code: str,
    expected_message: str,
    expected_category: str,
) -> list[str]:
    failures: list[str] = []
    try:
        payload = json.loads(output)
    except json.JSONDecodeError as exc:
        return [f"JSON output is not stable parseable JSON: {exc}"]
    diagnostics = payload.get("primary_report", {}).get("diagnostics", [])
    if not isinstance(diagnostics, list) or not diagnostics:
        return ["JSON output missing primary_report.diagnostics"]
    primary = diagnostics[0]
    if not isinstance(primary, dict):
        return ["JSON primary diagnostic must be an object"]
    failures.extend(validate_diagnostic_contract(primary, expected_code, expected_message, expected_category, "JSON"))
    seen_ids: set[str] = set()
    for index, diagnostic in enumerate(diagnostics):
        if not isinstance(diagnostic, dict):
            failures.append(f"JSON diagnostic {index} must be an object")
            continue
        diagnostic_id = diagnostic.get("id")
        if diagnostic_id in seen_ids:
            failures.append(f"JSON diagnostic id is duplicated: {diagnostic_id!r}")
        if isinstance(diagnostic_id, str):
            seen_ids.add(diagnostic_id)
        if diagnostic.get("severity") != "error":
            failures.append(f"JSON diagnostic {index} severity mismatch: {diagnostic.get('severity')!r}")
        if not diagnostic.get("category"):
            failures.append(f"JSON diagnostic {index} missing category")
        if not diagnostic.get("fluent_key"):
            failures.append(f"JSON diagnostic {index} missing Fluent key")
    for key in ("span", "labels", "cause", "fix", "example"):
        if not primary.get(key):
            failures.append(f"JSON diagnostic missing {key}")
    if not primary.get("suggestions"):
        failures.append("JSON diagnostic missing suggestions/fix-it")
    if json.dumps(payload, sort_keys=True) != json.dumps(json.loads(output), sort_keys=True):
        failures.append("JSON output is not deterministic under sorted-key normalization")
    return failures


def validate_lsp_output(
    output: str,
    expected_code: str,
    expected_message: str,
    expected_category: str,
) -> list[str]:
    failures: list[str] = []
    try:
        payload = json.loads(output)
    except json.JSONDecodeError as exc:
        return [f"LSP output is not stable parseable JSON: {exc}"]
    if payload.get("jsonrpc") != "2.0":
        failures.append(f"LSP jsonrpc mismatch: {payload.get('jsonrpc')!r}")
    if payload.get("method") != "textDocument/publishDiagnostics":
        failures.append(f"LSP method mismatch: {payload.get('method')!r}")
    diagnostics = payload.get("params", {}).get("diagnostics", [])
    if not isinstance(diagnostics, list) or not diagnostics:
        return ["LSP output missing params.diagnostics"]
    primary = diagnostics[0]
    if not isinstance(primary, dict):
        return ["LSP primary diagnostic must be an object"]
    if primary.get("code") != expected_code:
        failures.append(f"LSP diagnostic code mismatch: {primary.get('code')!r}")
    if primary.get("severity") != 1:
        failures.append(f"LSP diagnostic severity mismatch: {primary.get('severity')!r}")
    if expected_message and primary.get("message") != expected_message:
        failures.append(f"LSP diagnostic message mismatch: {primary.get('message')!r}")
    if not primary.get("range"):
        failures.append("LSP diagnostic missing range")
    if not primary.get("relatedInformation"):
        failures.append("LSP diagnostic missing relatedInformation")
    data = primary.get("data")
    if not isinstance(data, dict):
        failures.append("LSP diagnostic missing data object")
    else:
        failures.extend(validate_diagnostic_contract(
            {
                "code": primary.get("code"),
                "severity": data.get("severity"),
                "category": data.get("category"),
                "id": data.get("id"),
                "fluent_key": data.get("fluent_key"),
                "message": primary.get("message"),
            },
            expected_code,
            expected_message,
            expected_category,
            "LSP",
        ))
        for key in ("cause", "fix", "example"):
            if not data.get(key):
                failures.append(f"LSP diagnostic data missing {key}")
    if json.dumps(payload, sort_keys=True) != json.dumps(json.loads(output), sort_keys=True):
        failures.append("LSP output is not deterministic under sorted-key normalization")
    return failures


def text_codes(output: str) -> list[str]:
    return ERROR_CODE_RE.findall(output)


def json_diagnostics(output: str) -> tuple[list[dict[str, Any]], list[str]]:
    try:
        payload = json.loads(output)
    except json.JSONDecodeError as exc:
        return [], [f"JSON output is not stable parseable JSON: {exc}"]
    diagnostics = payload.get("primary_report", {}).get("diagnostics", [])
    if not isinstance(diagnostics, list) or not diagnostics:
        return [], ["JSON output missing primary_report.diagnostics"]
    if not all(isinstance(item, dict) for item in diagnostics):
        return [], ["JSON diagnostics must be objects"]
    return diagnostics, []


def lsp_diagnostics(output: str) -> tuple[list[dict[str, Any]], list[str]]:
    try:
        payload = json.loads(output)
    except json.JSONDecodeError as exc:
        return [], [f"LSP output is not stable parseable JSON: {exc}"]
    failures: list[str] = []
    if payload.get("jsonrpc") != "2.0":
        failures.append(f"LSP jsonrpc mismatch: {payload.get('jsonrpc')!r}")
    if payload.get("method") != "textDocument/publishDiagnostics":
        failures.append(f"LSP method mismatch: {payload.get('method')!r}")
    diagnostics = payload.get("params", {}).get("diagnostics", [])
    if not isinstance(diagnostics, list) or not diagnostics:
        return [], [*failures, "LSP output missing params.diagnostics"]
    if not all(isinstance(item, dict) for item in diagnostics):
        return [], [*failures, "LSP diagnostics must be objects"]
    return diagnostics, failures


def validate_text_spans(output: str, expected: list[str]) -> list[str]:
    failures: list[str] = []
    spans = TEXT_DIAGNOSTIC_ID_RE.findall(output)
    span_codes = [code for code, _file, _line, _column in spans]
    if span_codes != expected:
        failures.append(f"text diagnostic span order mismatch: expected {expected}, got {span_codes}")
    for index, (code, file_name, line, column) in enumerate(spans):
        if not file_name.endswith(".vit"):
            failures.append(f"text diagnostic {index} span file is not .vit: {file_name}")
        if int(line) <= 0 or int(column) <= 0:
            failures.append(f"text diagnostic {index} span must be positive file:line:column")
        if index < len(expected) and code != expected[index]:
            failures.append(f"text diagnostic {index} span code mismatch: expected {expected[index]}, got {code}")
    return failures


def validate_json_spans(diagnostics: list[dict[str, Any]], expected: list[str]) -> list[str]:
    failures: list[str] = []
    for index, diagnostic in enumerate(diagnostics):
        expected_code = expected[index] if index < len(expected) else None
        code = str(diagnostic.get("code", ""))
        if expected_code is not None and code != expected_code:
            failures.append(f"JSON diagnostic {index} span code mismatch: expected {expected_code}, got {code}")
        if not DIAGNOSTIC_ID_SPAN_RE.match(str(diagnostic.get("id", ""))):
            failures.append(f"JSON diagnostic {index} id missing real file:line:column span")
        if not JSON_LOCATION_RE.match(str(diagnostic.get("location", ""))):
            failures.append(f"JSON diagnostic {index} location missing real file:line:column span")
        span = diagnostic.get("span")
        if not isinstance(span, dict):
            failures.append(f"JSON diagnostic {index} span must be object")
            continue
        if span.get("valid") is not True:
            failures.append(f"JSON diagnostic {index} span.valid must be true")
        if not str(span.get("file", "")).endswith(".vit"):
            failures.append(f"JSON diagnostic {index} span.file must be a .vit file")
        for key in ("start_line", "start_column", "end_line", "end_column"):
            if not isinstance(span.get(key), int) or span.get(key) <= 0:
                failures.append(f"JSON diagnostic {index} span.{key} must be a positive integer")
    return failures


def validate_lsp_spans(diagnostics: list[dict[str, Any]], expected: list[str]) -> list[str]:
    failures: list[str] = []
    for index, diagnostic in enumerate(diagnostics):
        expected_code = expected[index] if index < len(expected) else None
        code = str(diagnostic.get("code", ""))
        if expected_code is not None and code != expected_code:
            failures.append(f"LSP diagnostic {index} span code mismatch: expected {expected_code}, got {code}")
        lsp_range = diagnostic.get("range")
        if not isinstance(lsp_range, dict):
            failures.append(f"LSP diagnostic {index} missing range")
        else:
            for side in ("start", "end"):
                point = lsp_range.get(side)
                if not isinstance(point, dict):
                    failures.append(f"LSP diagnostic {index} range.{side} missing")
                    continue
                if not isinstance(point.get("line"), int) or point.get("line") < 0:
                    failures.append(f"LSP diagnostic {index} range.{side}.line must be a non-negative integer")
                if not isinstance(point.get("character"), int) or point.get("character") < 0:
                    failures.append(f"LSP diagnostic {index} range.{side}.character must be a non-negative integer")
        data = diagnostic.get("data")
        if not isinstance(data, dict) or not DIAGNOSTIC_ID_SPAN_RE.match(str(data.get("id", ""))):
            failures.append(f"LSP diagnostic {index} data.id missing real file:line:column span")
        related = diagnostic.get("relatedInformation")
        if not isinstance(related, list) or not related:
            failures.append(f"LSP diagnostic {index} missing relatedInformation")
            continue
        location = related[0].get("location") if isinstance(related[0], dict) else None
        uri = location.get("uri") if isinstance(location, dict) else ""
        if not isinstance(uri, str) or not uri.startswith("file://") or ".vit" not in uri:
            failures.append(f"LSP diagnostic {index} relatedInformation missing file URI")
    return failures


def validate_code_order(actual: list[str], expected: list[str], expected_order: str, label: str) -> list[str]:
    if actual != expected:
        return [f"{label} diagnostic {expected_order} mismatch: expected {expected}, got {actual}"]
    return []


def validate_manifest_case(case: dict[str, Any], section: str) -> list[str]:
    failures: list[str] = []
    missing = sorted(REQUIRED_MANIFEST_FIELDS - set(case))
    if missing:
        failures.append(f"{section} case {case.get('id', '<missing-id>')} missing required fields: {missing}")
        return failures
    if not isinstance(case["id"], str) or not case["id"]:
        failures.append(f"{section} case id must be a non-empty string")
    if case["phase"] not in PHASE_DIRS:
        failures.append(f"{section} case {case['id']} phase must be one of {sorted(PHASE_DIRS)}")
    if not isinstance(case["entry"], str) or not case["entry"]:
        failures.append(f"{section} case {case['id']} entry must be a non-empty string")
    elif case["entry"].startswith("tests/compiler_real_diagnostics/invalid/"):
        parts = Path(case["entry"]).parts
        try:
            phase = parts[parts.index("invalid") + 1]
        except (ValueError, IndexError):
            phase = ""
        if phase != case["phase"]:
            failures.append(f"{section} case {case['id']} phase {case['phase']!r} does not match entry phase {phase!r}")
    if case["command"] not in VALID_COMMANDS:
        failures.append(f"{section} case {case['id']} command must be one of {sorted(VALID_COMMANDS)}")
    if not isinstance(case["expected_codes"], list) or not case["expected_codes"]:
        failures.append(f"{section} case {case['id']} expected_codes must be a non-empty ordered list")
    elif not all(isinstance(code, str) and code for code in case["expected_codes"]):
        failures.append(f"{section} case {case['id']} expected_codes must contain only non-empty strings")
    if case["expected_order"] not in VALID_EXPECTED_ORDERS:
        failures.append(f"{section} case {case['id']} expected_order must be one of {sorted(VALID_EXPECTED_ORDERS)}")
    if not isinstance(case["expected_locale"], str) or not case["expected_locale"]:
        failures.append(f"{section} case {case['id']} expected_locale must be a non-empty string")
    if not isinstance(case["expected_exit_code"], int) or case["expected_exit_code"] == 0:
        failures.append(f"{section} case {case['id']} expected_exit_code must be a non-zero integer")
    if case["cascade_policy"] not in VALID_CASCADE_POLICIES:
        failures.append(f"{section} case {case['id']} cascade_policy must be one of {sorted(VALID_CASCADE_POLICIES)}")
    return failures


def validate_case(case: dict[str, Any], locale: str, messages: dict[str, str]) -> dict[str, Any]:
    entry = str(case.get("entry", ""))
    command_name = str(case.get("command", "check"))
    expected_codes = list(case.get("expected_codes", []))
    expected_code = expected_codes[0] if expected_codes else ""
    expected_category = str(case.get("expected_category") or expected_category_for_code(expected_code))
    expected_order = str(case.get("expected_order", "source-order"))
    min_count = int(case.get("min_diagnostic_count", 1))
    result = run_command(str(case.get("id", "case")), command_name, entry, locale)
    json_result = run_command(str(case.get("id", "case")), command_name, entry, locale, diagnostics_json=True)
    lsp_result = run_command(str(case.get("id", "case")), command_name, entry, locale, diagnostics_lsp=True)
    output = str(result["combined"])
    json_output = str(json_result["stdout"] or json_result["combined"])
    lsp_output = str(lsp_result["stdout"] or lsp_result["combined"])
    failures: list[str] = validate_manifest_case(case, "cases")
    expected_message = messages.get(expected_code, "")

    if not (ROOT / entry).is_file():
        failures.append(f"fixture missing: {entry}")
    for surface_name, command_result in (("text", result), ("JSON", json_result), ("LSP", lsp_result)):
        if command_result["exit_code"] != case.get("expected_exit_code"):
            failures.append(
                f"{surface_name} exit code mismatch: expected {case.get('expected_exit_code')}, got {command_result['exit_code']}"
            )

    actual_text_codes = text_codes(output)
    failures.extend(validate_code_order(actual_text_codes, expected_codes, expected_order, "text"))
    failures.extend(validate_text_spans(output, expected_codes))
    json_items, json_parse_failures = json_diagnostics(json_output)
    failures.extend(json_parse_failures)
    json_codes = [str(item.get("code", "")) for item in json_items]
    if json_items:
        failures.extend(validate_code_order(json_codes, expected_codes, expected_order, "JSON"))
        failures.extend(validate_json_spans(json_items, expected_codes))
    lsp_items, lsp_parse_failures = lsp_diagnostics(lsp_output)
    failures.extend(lsp_parse_failures)
    lsp_codes = [str(item.get("code", "")) for item in lsp_items]
    if lsp_items:
        failures.extend(validate_code_order(lsp_codes, expected_codes, expected_order, "LSP"))
        failures.extend(validate_lsp_spans(lsp_items, expected_codes))

    for code in expected_codes:
        message = messages.get(code, "")
        if code not in output:
            failures.append(f"missing expected diagnostic code {code}")
        if code not in json_output:
            failures.append(f"JSON output missing expected diagnostic code {code}")
        if code not in lsp_output:
            failures.append(f"LSP output missing expected diagnostic code {code}")
        if message and message not in output:
            failures.append(f"missing Fluent {locale} message for {code}: {message}")
        if message and message not in json_output:
            failures.append(f"JSON output missing Fluent {locale} message for {code}: {message}")
        if message and message not in lsp_output:
            failures.append(f"LSP output missing Fluent {locale} message for {code}: {message}")
    if result["diagnostic_count"] < min_count:
        failures.append(
            f"expected at least {min_count} diagnostic(s), got {result['diagnostic_count']}"
        )
    if not LOCATION_RE.search(output):
        failures.append("missing path:line:column source location")
    for marker in RICH_TEXT_MARKERS:
        if marker not in output:
            failures.append(f"missing rich diagnostic marker {marker!r}")
    if not any(marker in output for marker in FIX_MARKERS):
        failures.append("missing fix-it/fix marker")
    if json_result["exit_code"] != 0 and expected_code in json_output:
        failures.extend(validate_json_output(json_output, expected_code, expected_message, expected_category))
    if lsp_result["exit_code"] != 0 and expected_code in lsp_output:
        failures.extend(validate_lsp_output(lsp_output, expected_code, expected_message, expected_category))

    return {
        "id": case.get("id"),
        "phase": case.get("phase"),
        "category": case.get("category"),
        "command_name": command_name,
        "entry": entry,
        "expected_codes": expected_codes,
        "expected_order": expected_order,
        "expected_locale": case.get("expected_locale"),
        "expected_exit_code": case.get("expected_exit_code"),
        "cascade_policy": case.get("cascade_policy"),
        "expected_category": expected_category,
        "expected_message": expected_message,
        "text_codes": actual_text_codes,
        "json_codes": json_codes,
        "lsp_codes": lsp_codes,
        "verified_surfaces": {
            "text": result["exit_code"] == case.get("expected_exit_code") and actual_text_codes == expected_codes,
            "json": json_result["exit_code"] == case.get("expected_exit_code") and json_codes == expected_codes and not json_parse_failures,
            "lsp": lsp_result["exit_code"] == case.get("expected_exit_code") and lsp_codes == expected_codes and not lsp_parse_failures,
        },
        "status": "pass" if not failures else "fail",
        "failures": failures,
        "command": result,
        "json_command": json_result,
        "lsp_command": lsp_result,
    }


def render_markdown(report: dict[str, Any]) -> str:
    lines = [
        "# Compiler Real Diagnostics Gate",
        "",
        f"- status: `{report['status']}`",
        f"- locale: `{report['locale']}`",
        f"- passed: `{report['passed']}/{report['total']}`",
        "",
        "| case | phase | command | status | expected codes |",
        "| --- | --- | --- | --- | --- |",
    ]
    for case in report["cases"]:
        lines.append(
            f"| `{case['id']}` | `{case['phase']}` | `{case['command_name']}` | `{case['status']}` | `{', '.join(case['expected_codes'])}` |"
        )
    if report["failures"]:
        lines.extend(["", "## Failures"])
        for failure in report["failures"]:
            lines.append(f"- {failure}")
    return "\n".join(lines) + "\n"


def main() -> int:
    if not BIN.is_file():
        print(f"[compiler-real-diagnostics][error] missing compiler: {rel(BIN)}", file=sys.stderr)
        return 1
    payload = load_json(CORPUS)
    locale = str(payload.get("locale", "fr"))
    locale_messages = parse_ftl(ROOT / "locales" / locale / "diagnostics.ftl")
    cases = payload.get("cases", [])
    if not isinstance(cases, list) or not cases:
        print(f"[compiler-real-diagnostics][error] {rel(CORPUS)} must contain cases", file=sys.stderr)
        return 1

    manifest_failures: list[str] = []
    for section_name in ("cases", "pending_cases"):
        section = payload.get(section_name, [])
        if not isinstance(section, list):
            manifest_failures.append(f"{section_name} must be a list")
            continue
        for index, case in enumerate(section):
            if not isinstance(case, dict):
                manifest_failures.append(f"{section_name}[{index}] must be an object")
                continue
            manifest_failures.extend(validate_manifest_case(case, section_name))
            if case.get("expected_locale") != locale:
                manifest_failures.append(
                    f"{section_name} case {case.get('id', '<missing-id>')} expected_locale must match corpus locale {locale!r}"
                )

    results = [validate_case(case, locale, locale_messages) for case in cases if isinstance(case, dict)]
    failures = [
        f"{case['id']}: {failure}"
        for case in results
        for failure in case["failures"]
    ]
    failures = [*manifest_failures, *failures]
    failures.extend(audit_no_surface_only_diagnostics())
    passed = sum(1 for case in results if case["status"] == "pass")
    report = {
        "schema": "vitte.compiler.real_diagnostics.gate.v1",
        "status": "pass" if not failures else "fail",
        "corpus": rel(CORPUS),
        "locale": locale,
        "passed": passed,
        "total": len(results),
        "pending_cases": payload.get("pending_cases", []),
        "cases": results,
        "failures": failures,
    }
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    REPORT_JSON.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    REPORT_MD.write_text(render_markdown(report), encoding="utf-8")

    if failures:
        for failure in failures[:80]:
            print(f"[compiler-real-diagnostics][error] {failure}", file=sys.stderr)
        print(
            f"[compiler-real-diagnostics] passed={passed}/{len(results)} report={rel(REPORT_JSON)}",
            file=sys.stderr,
        )
        return 1

    print(f"[compiler-real-diagnostics] OK passed={passed}/{len(results)} report={rel(REPORT_JSON)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
