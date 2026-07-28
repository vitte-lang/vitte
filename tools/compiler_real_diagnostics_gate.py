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
REQUIRED_PHASES = (
    "lexer",
    "parser",
    "resolver",
    "sema",
    "typeck",
    "borrowck",
    "mir",
    "ir",
    "backend",
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


def run_check(
    entry: str,
    locale: str,
    diagnostics_json: bool = False,
    diagnostics_lsp: bool = False,
) -> dict[str, Any]:
    args = [str(BIN), "check"]
    if diagnostics_json:
        args.append("--diagnostics-json")
    if diagnostics_lsp:
        args.append("--diagnostics-lsp")
    args.extend([entry, "--lang", locale])
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
            "check",
            *(["--diagnostics-json"] if diagnostics_json else []),
            *(["--diagnostics-lsp"] if diagnostics_lsp else []),
            entry,
            "--lang",
            locale,
        ],
        "exit_code": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "combined": combined,
        "diagnostic_count": combined.count("error[") + combined.count("warning["),
    }


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


def validate_case(case: dict[str, Any], locale: str, messages: dict[str, str]) -> dict[str, Any]:
    entry = str(case.get("entry", ""))
    expected_code = str(case.get("expected_code", ""))
    expected_category = str(case.get("expected_category") or expected_category_for_code(expected_code))
    min_count = int(case.get("min_diagnostic_count", 1))
    result = run_check(entry, locale)
    json_result = run_check(entry, locale, diagnostics_json=True)
    lsp_result = run_check(entry, locale, diagnostics_lsp=True)
    output = str(result["combined"])
    json_output = str(json_result["stdout"] or json_result["combined"])
    lsp_output = str(lsp_result["stdout"] or lsp_result["combined"])
    failures: list[str] = []
    expected_message = messages.get(expected_code, "")

    if not (ROOT / entry).is_file():
        failures.append(f"fixture missing: {entry}")
    if result["exit_code"] == 0:
        failures.append("compiler accepted invalid source")
    if json_result["exit_code"] == 0:
        failures.append("compiler accepted invalid source in diagnostics JSON mode")
    if lsp_result["exit_code"] == 0:
        failures.append("compiler accepted invalid source in diagnostics LSP mode")
    if expected_code not in output:
        failures.append(f"missing expected diagnostic code {expected_code}")
    if expected_code not in json_output:
        failures.append(f"JSON output missing expected diagnostic code {expected_code}")
    if expected_code not in lsp_output:
        failures.append(f"LSP output missing expected diagnostic code {expected_code}")
    if expected_message and expected_message not in output:
        failures.append(f"missing Fluent {locale} message for {expected_code}: {expected_message}")
    if expected_message and expected_message not in json_output:
        failures.append(f"JSON output missing Fluent {locale} message for {expected_code}: {expected_message}")
    if expected_message and expected_message not in lsp_output:
        failures.append(f"LSP output missing Fluent {locale} message for {expected_code}: {expected_message}")
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
        "category": case.get("category"),
        "entry": entry,
        "expected_code": expected_code,
        "expected_category": expected_category,
        "expected_message": expected_message,
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
        "| case | category | status | expected code |",
        "| --- | --- | --- | --- |",
    ]
    for case in report["cases"]:
        lines.append(
            f"| `{case['id']}` | {case['category']} | `{case['status']}` | `{case['expected_code']}` |"
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

    results = [validate_case(case, locale, locale_messages) for case in cases if isinstance(case, dict)]
    failures = [
        f"{case['id']}: {failure}"
        for case in results
        for failure in case["failures"]
    ]
    passed_phases = {
        str(case.get("expected_category", ""))
        for case in results
        if case["status"] == "pass"
    }
    for phase in REQUIRED_PHASES:
        if phase not in passed_phases:
            failures.append(f"missing passing real Fluent diagnostic phase: {phase}")
    passed = sum(1 for case in results if case["status"] == "pass")
    report = {
        "schema": "vitte.compiler.real_diagnostics.gate.v1",
        "status": "pass" if not failures else "fail",
        "corpus": rel(CORPUS),
        "locale": locale,
        "passed": passed,
        "total": len(results),
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
