#!/usr/bin/env python3
"""Validate the central Vitte diagnostic catalog and its test mapping."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "schemas" / "diagnostics" / "codes.json"
TEST_ROOT = ROOT / "tests" / "diagnostics" / "catalog"
COMPILER_ROOT = ROOT / "src" / "vitte" / "compiler"

SEVERITIES = {"error", "warning", "note", "help", "fatal"}
REQUIRED_ASSERTS = {
    "code",
    "title",
    "span",
    "notes",
    "suggestions",
    "no_parasitic_diagnostics",
    "stable_order",
    "recovery",
}
FORBIDDEN_TERMS = (
    "unknown error",
    "unexpected failure",
    "something went wrong",
    "semantic problem",
    "internal issue",
)
CONDITIONALLY_FORBIDDEN = ("invalid", "failed")
PRECISE_CAUSE_WORDS = (
    "because",
    "when",
    "while",
    "without",
    "missing",
    "required",
    "unsupported",
    "target",
    "symbol",
    "token",
    "span",
    "type",
    "borrow",
    "module",
    "library",
    "object",
    "entry",
    "permission",
    "architecture",
    "delimiter",
    "literal",
    "source",
)
DIAGNOSTIC_CALL = re.compile(r"\bdiagnostic_(?:create|error|warning|fatal)\s*\((?P<arg>[^,\n]+)")
CODE_LITERAL = re.compile(r'"([A-Z][A-Z0-9_]*|[A-Z]{3}[0-9]{4}|E[0-9]{4}|P[0-9A-Z_]+)"')
ALLOWED_DYNAMIC_PRODUCERS = {
    "src/vitte/compiler/diagnostics/diagnostic.vit",
    "src/vitte/compiler/diagnostics/architecture.vit",
    "src/vitte/compiler/infrastructure/diagnostics/diagnostic.vit",
    "src/vitte/compiler/infrastructure/diagnostics/emitter.vit",
    "src/vitte/compiler/infrastructure/session/diagnostics.vit",
    "src/vitte/compiler/analysis/sema/errors.vit",
    "src/vitte/compiler/analysis/typeck/errors.vit",
    "src/vitte/compiler/analysis/borrowck/errors.vit",
    "src/vitte/compiler/analysis/borrowck/lifetimes.vit",
    "src/vitte/compiler/analysis/const_eval/errors.vit",
    "src/vitte/compiler/analysis/const_eval/interpreter.vit",
    "src/vitte/compiler/middle/typecheck/diagnostics.vit",
}


def fail(message: str) -> int:
    print(f"[diagnostic-catalog][error] {message}", file=sys.stderr)
    return 1


def load_json(path: Path) -> Any:
    return json.loads(path.read_text(encoding="utf-8"))


def precise_enough(text: str, term: str) -> bool:
    lowered = text.lower()
    if term not in lowered:
        return True
    words = re.findall(r"[a-z0-9_]+", lowered)
    if term in words:
        index = words.index(term)
        if index + 1 < len(words) and words[index + 1] not in {"program", "input", "error", "failure", "issue"}:
            return True
        if index > 0 and words[index - 1] not in {"program", "input", "error", "failure", "issue"}:
            return True
    return any(word in lowered for word in PRECISE_CAUSE_WORDS) and len(words) >= 3


def validate_text(text: str, location: str) -> list[str]:
    lowered = text.lower()
    failures: list[str] = []
    for term in FORBIDDEN_TERMS:
        if term in lowered:
            failures.append(f"{location}: contains forbidden vague term {term!r}")
    for term in CONDITIONALLY_FORBIDDEN:
        if term in lowered and not precise_enough(text, term):
            failures.append(f"{location}: uses {term!r} without a precise cause")
    return failures


def load_test_manifests() -> dict[str, set[str]]:
    manifests: dict[str, set[str]] = {}
    for path in sorted(TEST_ROOT.glob("*.catalog.json")):
        payload = load_json(path)
        cases = payload.get("cases") if isinstance(payload, dict) else None
        if not isinstance(cases, list):
            manifests[path.as_posix()] = set()
            continue
        manifests[path.relative_to(ROOT).as_posix()] = {
            str(case.get("code"))
            for case in cases
            if isinstance(case, dict) and isinstance(case.get("code"), str)
        }
    return manifests


def validate_catalog() -> list[str]:
    payload = load_json(CATALOG)
    entries = payload.get("codes") if isinstance(payload, dict) else None
    if not isinstance(entries, list) or not entries:
        return [f"{CATALOG.relative_to(ROOT)}: codes must be a non-empty array"]

    manifests = load_test_manifests()
    failures: list[str] = []
    seen_codes: set[str] = set()
    seen_aliases: dict[str, str] = {}

    for index, entry in enumerate(entries):
        if not isinstance(entry, dict):
            failures.append(f"codes[{index}]: entry must be an object")
            continue
        code = entry.get("code")
        phase = entry.get("phase")
        title = entry.get("title")
        severity = entry.get("default_severity")
        parameters = entry.get("required_parameters")
        documentation = entry.get("documentation")
        tests = entry.get("tests")

        if not isinstance(code, str) or not code:
            failures.append(f"codes[{index}]: code is required")
            continue
        if code in seen_codes:
            failures.append(f"{code}: duplicate diagnostic code")
        seen_codes.add(code)
        if not isinstance(title, str) or not title.strip():
            failures.append(f"{code}: title is required")
        else:
            failures.extend(validate_text(title, f"{code}.title"))
        if not isinstance(phase, str) or not phase:
            failures.append(f"{code}: phase is required")
        if severity not in SEVERITIES:
            failures.append(f"{code}: default_severity must be one of {sorted(SEVERITIES)}")
        if not isinstance(parameters, list) or not parameters or not all(isinstance(item, str) and item for item in parameters):
            failures.append(f"{code}: required_parameters must be a non-empty string array")
        if not isinstance(documentation, dict):
            failures.append(f"{code}: documentation is required")
        else:
            for field in ("title", "summary", "cause", "action", "example", "url"):
                value = documentation.get(field)
                if not isinstance(value, str) or not value.strip():
                    failures.append(f"{code}: documentation.{field} is required")
                else:
                    failures.extend(validate_text(value, f"{code}.documentation.{field}"))
        aliases = entry.get("aliases")
        if not isinstance(aliases, list) or not aliases:
            failures.append(f"{code}: aliases must contain the registered message key")
        else:
            for alias in aliases:
                if not isinstance(alias, str):
                    failures.append(f"{code}: aliases must be strings")
                    continue
                previous = seen_aliases.get(alias)
                if previous and previous != code:
                    failures.append(f"{alias}: alias maps to both {previous} and {code}")
                seen_aliases[alias] = code
        if not isinstance(tests, list) or not tests:
            failures.append(f"{code}: tests must contain at least one associated test")
        else:
            covered = False
            for test in tests:
                if not isinstance(test, dict):
                    failures.append(f"{code}: tests entries must be objects")
                    continue
                path = test.get("path")
                case = test.get("case")
                asserts = test.get("asserts")
                if not isinstance(path, str) or not isinstance(case, str):
                    failures.append(f"{code}: tests entries require path and case")
                    continue
                if path not in manifests:
                    failures.append(f"{code}: associated test path does not exist: {path}")
                    continue
                if case not in manifests[path]:
                    failures.append(f"{code}: associated test case {case!r} is absent from {path}")
                    continue
                covered = True
                if not isinstance(asserts, list) or not REQUIRED_ASSERTS.issubset(set(asserts)):
                    failures.append(f"{code}: tests[{path}] must assert {sorted(REQUIRED_ASSERTS)}")
            if not covered:
                failures.append(f"{code}: no associated test case covers this diagnostic")

    return failures


def validate_diagnostic_code_usage() -> list[str]:
    payload = load_json(CATALOG)
    entries = payload.get("codes", [])
    known: set[str] = set()
    if isinstance(entries, list):
        for entry in entries:
            if not isinstance(entry, dict):
                continue
            if isinstance(entry.get("code"), str):
                known.add(entry["code"])
            aliases = entry.get("aliases")
            if isinstance(aliases, list):
                known.update(alias for alias in aliases if isinstance(alias, str))

    failures: list[str] = []
    for path in sorted(COMPILER_ROOT.rglob("*.vit")):
        rel = path.relative_to(ROOT).as_posix()
        if "/tests/" in rel:
            continue
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            match = DIAGNOSTIC_CALL.search(line)
            if match:
                first_arg = match.group("arg").strip()
                if "+" in first_arg:
                    failures.append(f"{rel}:{line_number}: diagnostic code must not be constructed dynamically")
                if rel in ALLOWED_DYNAMIC_PRODUCERS:
                    continue
                literal = CODE_LITERAL.fullmatch(first_arg)
                if literal:
                    value = literal.group(1)
                    if not value.startswith("ICE") and value not in known and value != "DIAG_INTERNAL_PRIMARY":
                        failures.append(f"{rel}:{line_number}: diagnostic code {value!r} is not registered")
                elif rel not in ALLOWED_DYNAMIC_PRODUCERS:
                    failures.append(f"{rel}:{line_number}: diagnostic code must be a registered literal or an official catalog wrapper")
    return failures


def main() -> int:
    failures = [*validate_catalog(), *validate_diagnostic_code_usage()]
    if failures:
        for failure in failures:
            print(f"[diagnostic-catalog][error] {failure}", file=sys.stderr)
        return 1
    print("[diagnostic-catalog] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
