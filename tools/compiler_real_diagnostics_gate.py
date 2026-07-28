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

RICH_TEXT_MARKERS = (
    "span:",
    "label:",
    "cause:",
    "help:",
    "corrected example:",
)
FIX_MARKERS = ("fix-it:", "fix:")
LOCATION_RE = re.compile(r"[^:\s]+\.vit:\d+:\d+")


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


def run_check(entry: str, locale: str, diagnostics_json: bool = False) -> dict[str, Any]:
    args = [str(BIN), "check"]
    if diagnostics_json:
        args.append("--diagnostics-json")
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
        "argv": ["bin/vitte", "check", *(["--diagnostics-json"] if diagnostics_json else []), entry, "--lang", locale],
        "exit_code": proc.returncode,
        "stdout": proc.stdout,
        "stderr": proc.stderr,
        "combined": combined,
        "diagnostic_count": combined.count("error[") + combined.count("warning["),
    }


def validate_json_output(output: str, expected_code: str, expected_message: str) -> list[str]:
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
    if primary.get("code") != expected_code:
        failures.append(f"JSON diagnostic code mismatch: {primary.get('code')!r}")
    if expected_message and primary.get("message") != expected_message:
        failures.append(f"JSON diagnostic message mismatch: {primary.get('message')!r}")
    for key in ("span", "labels", "cause", "fix", "example"):
        if not primary.get(key):
            failures.append(f"JSON diagnostic missing {key}")
    if not primary.get("suggestions"):
        failures.append("JSON diagnostic missing suggestions/fix-it")
    if json.dumps(payload, sort_keys=True) != json.dumps(json.loads(output), sort_keys=True):
        failures.append("JSON output is not deterministic under sorted-key normalization")
    return failures


def validate_case(case: dict[str, Any], locale: str, messages: dict[str, str]) -> dict[str, Any]:
    entry = str(case.get("entry", ""))
    expected_code = str(case.get("expected_code", ""))
    min_count = int(case.get("min_diagnostic_count", 1))
    result = run_check(entry, locale)
    json_result = run_check(entry, locale, diagnostics_json=True)
    output = str(result["combined"])
    json_output = str(json_result["stdout"] or json_result["combined"])
    failures: list[str] = []
    expected_message = messages.get(expected_code, "")

    if not (ROOT / entry).is_file():
        failures.append(f"fixture missing: {entry}")
    if result["exit_code"] == 0:
        failures.append("compiler accepted invalid source")
    if json_result["exit_code"] == 0:
        failures.append("compiler accepted invalid source in diagnostics JSON mode")
    if expected_code not in output:
        failures.append(f"missing expected diagnostic code {expected_code}")
    if expected_code not in json_output:
        failures.append(f"JSON output missing expected diagnostic code {expected_code}")
    if expected_message and expected_message not in output:
        failures.append(f"missing Fluent {locale} message for {expected_code}: {expected_message}")
    if expected_message and expected_message not in json_output:
        failures.append(f"JSON output missing Fluent {locale} message for {expected_code}: {expected_message}")
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
        failures.extend(validate_json_output(json_output, expected_code, expected_message))

    return {
        "id": case.get("id"),
        "category": case.get("category"),
        "entry": entry,
        "expected_code": expected_code,
        "expected_message": expected_message,
        "status": "pass" if not failures else "fail",
        "failures": failures,
        "command": result,
        "json_command": json_result,
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
