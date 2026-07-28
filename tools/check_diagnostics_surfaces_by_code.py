#!/usr/bin/env python3
"""Verify that every public diagnostic code has text, JSON, and LSP surfaces.

The per-code part is catalog based: every public code must exist in every
Fluent text catalog and explanation catalog. JSON and LSP renderers are
generic over Diagnostic.code, so this gate also locks the renderer fields and
runs one real CLI diagnostic through text, JSON, and LSP output.
"""

from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CORE_CODES = ROOT / "tests" / "diag_snapshots" / "core_diagnostic_codes.txt"
BIN = ROOT / "bin" / "vitte"
FIXTURE = ROOT / "tests" / "negative" / "type_mismatch.vit"
REPORT_DIR = ROOT / "target" / "reports" / "diagnostics_surfaces"
JSON_REPORT = REPORT_DIR / "surfaces_by_code.json"

sys.path.insert(0, str(ROOT / "tools"))
from diagnostic_catalog_data import public_diagnostic_codes  # noqa: E402
from diagnostics_locales import supported_locale_codes  # noqa: E402


EXPECTED_CODE = "TYPECK_E_ASSIGN_MISMATCH"
EXPECTED_EN_MESSAGE = "assignment type mismatch"
EXPECTED_FR_MESSAGE = "affectation type incompatibilite"
EXPECTED_CAUSE = "The inferred type does not satisfy the type required at this location."
EXPECTED_FIX = "assign a value of the declared binding type, or change the binding annotation at its declaration"
EXPECTED_EXAMPLE = "let count: int = 1"
EXPECTED_TEXT_TERMS = (
    f"error[{EXPECTED_CODE}]",
    EXPECTED_FR_MESSAGE,
    "span: tests/negative/type_mismatch.vit:5:11-5:18",
    "label: expected declared assignment type",
    f"cause: {EXPECTED_CAUSE}",
    "help: compare the declared binding type with the assigned expression type before changing code.",
    "fix-it: replace the incompatible value with `1`, or change the binding annotation intentionally.",
    f"fix: {EXPECTED_FIX}",
    "corrected example: let count: int = 1",
)

REQUIRED_EXPLAIN_SUFFIXES = ("summary", "cause", "step1", "fix", "example")
REQUIRED_JSON_RENDERER_TERMS = (
    'json_key_value(',
    '"code"',
    '"message"',
    '"span"',
    '"labels"',
    '"notes"',
    '"helps"',
    '"suggestions"',
    '"fixes"',
    '"valid_example"',
    '"invalid_example"',
    "diagnostic_schema_version",
    "diagnostic.code",
    "diagnostic.message",
)
REQUIRED_LSP_RENDERER_TERMS = (
    "code: diagnostic.code",
    "message: diagnostic.message",
    'source: "vitte"',
    "lsp_range_from_span(diagnostic.span)",
    "diagnostic_related_information_list(diagnostic)",
    "diagnostic.suggestions",
    "lsp_code_action_from_suggestion",
    "diagnostic_code: diagnostic.code",
    "https://vitte.dev/errors/",
)


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def parse_ftl(path: Path) -> dict[str, str]:
    out: dict[str, str] = {}
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        out[key.strip()] = value.strip()
    return out


def selected_codes() -> list[str]:
    existing: list[str] = []
    if CORE_CODES.exists():
        existing = [
            line.strip()
            for line in CORE_CODES.read_text(encoding="utf-8").splitlines()
            if line.strip() and not line.strip().startswith("#")
        ]
    return public_diagnostic_codes(existing)


def require_terms(path: Path, terms: tuple[str, ...]) -> list[str]:
    text = path.read_text(encoding="utf-8")
    return [term for term in terms if term not in text]


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )


def verify_runtime_text() -> list[str]:
    proc = run([str(BIN), "check", str(FIXTURE.relative_to(ROOT)), "--lang", "fr"])
    output = proc.stdout + proc.stderr
    errors: list[str] = []
    if proc.returncode == 0:
        errors.append("text runtime fixture unexpectedly passed")
    for term in EXPECTED_TEXT_TERMS:
        if term not in output:
            errors.append(f"text runtime output missing {term!r}")
    if EXPECTED_EN_MESSAGE in output:
        errors.append("text runtime output still contains the English message under --lang fr")
    return errors


def verify_runtime_json() -> list[str]:
    proc = run([str(BIN), "check", "--diagnostics-json", str(FIXTURE.relative_to(ROOT)), "--lang", "fr"])
    errors: list[str] = []
    if proc.returncode == 0:
        errors.append("JSON runtime fixture unexpectedly passed")
    try:
        payload = json.loads(proc.stdout)
    except json.JSONDecodeError as exc:
        return [f"JSON runtime output is invalid JSON: {exc}"]
    diagnostics = payload.get("primary_report", {}).get("diagnostics", [])
    if len(diagnostics) != 1:
        errors.append("JSON runtime output must contain exactly one diagnostic")
        return errors
    primary = diagnostics[0]
    required_pairs: dict[str, Any] = {
        "code": EXPECTED_CODE,
        "message": EXPECTED_FR_MESSAGE,
        "cause": EXPECTED_CAUSE,
        "fix": EXPECTED_FIX,
        "example": EXPECTED_EXAMPLE,
    }
    for key, expected in required_pairs.items():
        if primary.get(key) != expected:
            errors.append(f"JSON runtime {key} mismatch: {primary.get(key)!r}")
    for key in ("span", "labels", "suggestions"):
        if not primary.get(key):
            errors.append(f"JSON runtime missing {key}")
    return errors


def verify_runtime_lsp() -> list[str]:
    proc = run([str(BIN), "check", "--diagnostics-lsp", str(FIXTURE.relative_to(ROOT)), "--lang", "fr"])
    errors: list[str] = []
    if proc.returncode == 0:
        errors.append("LSP runtime fixture unexpectedly passed")
    try:
        payload = json.loads(proc.stdout)
    except json.JSONDecodeError as exc:
        return [f"LSP runtime output is invalid JSON: {exc}"]
    diagnostics = payload.get("params", {}).get("diagnostics", [])
    if len(diagnostics) != 1:
        errors.append("LSP runtime output must publish exactly one diagnostic")
        return errors
    primary = diagnostics[0]
    if primary.get("code") != EXPECTED_CODE:
        errors.append(f"LSP runtime code mismatch: {primary.get('code')!r}")
    if primary.get("message") != EXPECTED_FR_MESSAGE:
        errors.append(f"LSP runtime message mismatch: {primary.get('message')!r}")
    for key in ("range", "relatedInformation", "codeDescription", "data"):
        if not primary.get(key):
            errors.append(f"LSP runtime missing {key}")
    data = primary.get("data", {})
    for key, expected in (("cause", EXPECTED_CAUSE), ("fix", EXPECTED_FIX), ("example", EXPECTED_EXAMPLE)):
        if data.get(key) != expected:
            errors.append(f"LSP runtime data.{key} mismatch: {data.get(key)!r}")
    return errors


def main() -> int:
    codes = selected_codes()
    failures: list[str] = []
    locale_report: dict[str, dict[str, Any]] = {}

    for locale in supported_locale_codes():
        locale_path = ROOT / "locales" / locale / "diagnostics.ftl"
        explain_path = ROOT / "locales" / locale / "diagnostics_explain.ftl"
        if not locale_path.exists():
            failures.append(f"{rel(locale_path)} missing")
            continue
        if not explain_path.exists():
            failures.append(f"{rel(explain_path)} missing")
            continue
        messages = parse_ftl(locale_path)
        explanations = parse_ftl(explain_path)
        missing_messages = [code for code in codes if code not in messages]
        missing_explanations = [
            f"{code}.{suffix}"
            for code in codes
            for suffix in REQUIRED_EXPLAIN_SUFFIXES
            if f"{code}.{suffix}" not in explanations
        ]
        if missing_messages:
            failures.append(f"{rel(locale_path)} missing message code(s): {', '.join(missing_messages[:20])}")
        if missing_explanations:
            failures.append(f"{rel(explain_path)} missing explain key(s): {', '.join(missing_explanations[:20])}")
        locale_report[locale] = {
            "message_codes": len(messages),
            "missing_messages": missing_messages,
            "missing_explanations": missing_explanations,
            "status": "pass" if not missing_messages and not missing_explanations else "fail",
        }

    json_missing = require_terms(ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "json.vit", REQUIRED_JSON_RENDERER_TERMS)
    if json_missing:
        failures.append("JSON renderer missing required generic terms: " + ", ".join(json_missing))
    lsp_missing = require_terms(ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "lsp.vit", REQUIRED_LSP_RENDERER_TERMS)
    if lsp_missing:
        failures.append("LSP renderer missing required generic terms: " + ", ".join(lsp_missing))

    runtime_errors = {
        "text": verify_runtime_text(),
        "json": verify_runtime_json(),
        "lsp": verify_runtime_lsp(),
    }
    for surface, errors in runtime_errors.items():
        failures.extend(f"{surface} runtime: {error}" for error in errors)

    report = {
        "status": "pass" if not failures else "fail",
        "code_count": len(codes),
        "locales": locale_report,
        "surfaces": {
            "text": {
                "mode": "per-code Fluent catalog plus runtime CLI proof",
                "runtime_code": EXPECTED_CODE,
                "runtime_status": "pass" if not runtime_errors["text"] else "fail",
            },
            "json": {
                "mode": "generic Diagnostic renderer over diagnostic.code plus runtime CLI proof",
                "runtime_code": EXPECTED_CODE,
                "runtime_status": "pass" if not runtime_errors["json"] else "fail",
            },
            "lsp": {
                "mode": "generic Diagnostic renderer over diagnostic.code plus runtime CLI proof",
                "runtime_code": EXPECTED_CODE,
                "runtime_status": "pass" if not runtime_errors["lsp"] else "fail",
            },
        },
        "failures": failures,
    }

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    JSON_REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    if failures:
        for failure in failures:
            print(f"[diagnostics-surfaces-by-code][error] {failure}", file=sys.stderr)
        print(f"[diagnostics-surfaces-by-code] report={rel(JSON_REPORT)}", file=sys.stderr)
        return 1

    print(
        "[diagnostics-surfaces-by-code] OK "
        f"codes={len(codes)} locales={len(locale_report)} surfaces=text,json,lsp "
        f"report={rel(JSON_REPORT)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
