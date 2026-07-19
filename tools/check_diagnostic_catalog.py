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
KINDS = {"user", "configuration", "environment", "linker", "internal_compiler"}
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
        kind = entry.get("kind", infer_kind(str(code), str(phase)))
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
        if kind not in KINDS:
            failures.append(f"{code}: kind must be one of {sorted(KINDS)}")
        if isinstance(code, str) and code.startswith("ICE") and kind != "internal_compiler":
            failures.append(f"{code}: ICE is reserved for internal compiler diagnostics")
        if kind == "internal_compiler" and isinstance(code, str) and not code.startswith("ICE"):
            failures.append(f"{code}: internal compiler diagnostics must use an ICE code")
        if phase == "ice" and kind != "internal_compiler":
            failures.append(f"{code}: ice phase must use kind=internal_compiler")
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


def infer_kind(code: str, phase: str) -> str:
    if code.startswith("ICE") or phase == "ice":
        return "internal_compiler"
    if code.startswith("LNK") or code.startswith("LINK_") or phase == "linker":
        return "linker"
    if code.startswith("GEN") or code.startswith("BACKEND_") or phase == "codegen":
        return "environment"
    if code.startswith("DRIVER_"):
        return "configuration"
    return "user"


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


def validate_canonical_diagnostic_contract() -> list[str]:
    text = (COMPILER_ROOT / "diagnostics" / "diagnostic.vit").read_text(encoding="utf-8")
    failures: list[str] = []
    required_fragments = (
        "DiagnosticKind.User",
        "DiagnosticKind.Configuration",
        "DiagnosticKind.Environment",
        "DiagnosticKind.Linker",
        "DiagnosticKind.InternalCompiler",
        "starts_with(code, \"ICE\")",
        "diagnostic_ice",
        "diagnostic_sanitize_technical_cause",
        "internal compiler error in \" + phase_name(phase)",
        "diag_kv(\"stable_id\", stable_code)",
        "diagnostic_with_internal_cause(diag4, cause_text)",
        "diagnostic_with_external_stderr",
        "if config.verbose and diagnostic.external_command != \"\"",
        "DIAGNOSTIC_USER_COLUMN_BASE",
        "DIAGNOSTIC_INTERNAL_OFFSET_UNIT",
    )
    for fragment in required_fragments:
        if fragment not in text:
            failures.append(f"src/vitte/compiler/diagnostics/diagnostic.vit: missing diagnostic contract fragment {fragment!r}")
    return failures


def validate_phase_policy_contract() -> list[str]:
    text = (COMPILER_ROOT / "diagnostics" / "phase_policy.vit").read_text(encoding="utf-8")
    failures: list[str] = []
    required_fragments = (
        "blocks: [string]",
        "root_error_codes: [string]",
        "diagnostic_phase_blocks",
        "diagnostic_phase_root_error_codes",
        "LEX_E_INVALID_CHAR",
        "TYPECK_E_ASSIGN_MISMATCH",
        "BORROWCK_E_USE_AFTER_MOVE",
        "LINK_E_SYSTEM_LINKER_FAILED",
    )
    for fragment in required_fragments:
        if fragment not in text:
            failures.append(f"src/vitte/compiler/diagnostics/phase_policy.vit: missing phase policy fragment {fragment!r}")
    return failures


def validate_backend_linker_contract() -> list[str]:
    text = (COMPILER_ROOT / "backend" / "diagnostics.vit").read_text(encoding="utf-8")
    failures: list[str] = []
    required_fragments = (
        "form LinkerFailureInfo",
        "LINK_E_UNDEFINED_SYMBOL",
        "LINK_E_LIBRARY_NOT_FOUND",
        "LINK_E_ARCHITECTURE_INCOMPATIBLE",
        "LINK_E_UNSUPPORTED_FORMAT",
        "LINK_E_DUPLICATE_SYMBOL",
        "LINK_E_ENTRYPOINT_MISSING",
        "LINK_E_PERMISSION_DENIED",
        "backend_tool_crashed",
        "vitte_demangle_symbol",
        "diagnostic_with_external_stderr",
        "diagnostic_with_kind(diag0, DiagnosticKind.Linker)",
    )
    for fragment in required_fragments:
        if fragment not in text:
            failures.append(f"src/vitte/compiler/backend/diagnostics.vit: missing backend/linker diagnostic fragment {fragment!r}")
    if "compilation failed" in text or "build failed" in text:
        failures.append("src/vitte/compiler/backend/diagnostics.vit: backend/linker diagnostics must not collapse to build failed")
    driver_text = (COMPILER_ROOT / "driver" / "compiler.vit").read_text(encoding="utf-8")
    if "build failed: linker did not materialize executable" in driver_text:
        failures.append("src/vitte/compiler/driver/compiler.vit: linker materialization diagnostics must not collapse to build failed")
    return failures


def main() -> int:
    failures = [
        *validate_catalog(),
        *validate_diagnostic_code_usage(),
        *validate_canonical_diagnostic_contract(),
        *validate_phase_policy_contract(),
        *validate_backend_linker_contract(),
    ]
    if failures:
        for failure in failures:
            print(f"[diagnostic-catalog][error] {failure}", file=sys.stderr)
        return 1
    print("[diagnostic-catalog] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
