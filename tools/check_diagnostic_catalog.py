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
CENTRAL_CATALOG = ROOT / "schemas" / "diagnostics" / "catalog.json"
TEST_ROOT = ROOT / "tests" / "diagnostics" / "catalog"
COMPILER_ROOT = ROOT / "src" / "vitte" / "compiler"
SOURCE_SHAPE_FIXTURES = (
    "source-ascii.json",
    "source-accents.json",
    "source-unicode.json",
    "source-tabs.json",
    "source-long-line.json",
    "source-empty-file.json",
    "source-lf.json",
    "source-crlf.json",
)

SEVERITIES = {"error", "warning", "note", "help", "fatal"}
CENTRAL_PHASES = {
    "lexer",
    "parser",
    "bootstrap",
    "module_resolution",
    "symbol_resolution",
    "sema",
    "typeck",
    "borrowck",
    "mir",
    "ir",
    "codegen",
    "linker",
    "runtime",
    "driver",
    "ice",
}
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
CENTRAL_SNAPSHOT_ASSERTS = {
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
    "invalid",
    "failed",
    "unknown error",
    "unexpected failure",
    "something went wrong",
    "semantic problem",
    "internal issue",
)
LEGACY_CONDITIONALLY_FORBIDDEN = ("invalid", "failed")
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
CODE_LITERAL = re.compile(r'"([A-Z][A-Z0-9_]*|[A-Z]{3}[0-9]{4}|E[0-9]{4}|P[0-9A-Z_]+)"')
ALLOWED_DYNAMIC_PRODUCERS = {
    "src/vitte/compiler/diagnostics/diagnostic.vit",
    "src/vitte/compiler/diagnostics/architecture.vit",
    "src/vitte/compiler/infrastructure/diagnostics/diagnostic.vit",
    "src/vitte/compiler/infrastructure/diagnostics/emitter.vit",
    "src/vitte/compiler/infrastructure/session/diagnostics.vit",
    "src/vitte/compiler/analysis/sema/errors.vit",
    "src/vitte/compiler/analysis/typeck/errors.vit",
    "src/vitte/compiler/analysis/borrowck/mod.vit",
    "src/vitte/compiler/analysis/borrowck/errors.vit",
    "src/vitte/compiler/analysis/borrowck/lifetimes.vit",
    "src/vitte/compiler/analysis/const_eval/errors.vit",
    "src/vitte/compiler/analysis/const_eval/evaluator.vit",
    "src/vitte/compiler/analysis/const_eval/interpreter.vit",
    "src/vitte/compiler/analysis/report.vit",
    "src/vitte/compiler/middle/typecheck/diagnostics.vit",
    "src/vitte/compiler/middle/borrow/checks.vit",
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


def followed_by_precise_cause(text: str, term: str) -> bool:
    lowered = text.lower()
    start = lowered.find(term)
    if start < 0:
        return True
    tail = lowered[start + len(term):]
    if not tail.strip():
        return False
    words = re.findall(r"[a-z0-9_]+", tail)
    if len(words) < 2:
        return False
    return any(word in words for word in PRECISE_CAUSE_WORDS)


def validate_text(text: str, location: str, strict: bool = True) -> list[str]:
    lowered = text.lower()
    failures: list[str] = []
    for term in FORBIDDEN_TERMS:
        if not strict and term in LEGACY_CONDITIONALLY_FORBIDDEN:
            continue
        if term in lowered and not followed_by_precise_cause(text, term):
            failures.append(f"{location}: contains forbidden vague term {term!r}")
    if not strict:
        for term in LEGACY_CONDITIONALLY_FORBIDDEN:
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
    if not CATALOG.exists():
        return [f"{CATALOG.relative_to(ROOT)}: central diagnostic catalog is required"]
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
            failures.extend(validate_text(title, f"{code}.title", strict=False))
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
                    failures.extend(validate_text(value, f"{code}.documentation.{field}", strict=False))
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


def validate_central_catalog() -> list[str]:
    if not CENTRAL_CATALOG.exists():
        return [f"{CENTRAL_CATALOG.relative_to(ROOT)}: central diagnostic catalog is required"]
    payload = load_json(CENTRAL_CATALOG)
    entries = payload.get("entries") if isinstance(payload, dict) else None
    if not isinstance(entries, list) or not entries:
        return [f"{CENTRAL_CATALOG.relative_to(ROOT)}: entries must be a non-empty array"]

    failures: list[str] = []
    seen: set[str] = set()
    required_doc_fields = ("url", "summary", "cause", "correction")
    for index, entry in enumerate(entries):
        if not isinstance(entry, dict):
            failures.append(f"central.entries[{index}]: entry must be an object")
            continue
        code = entry.get("code")
        title = entry.get("title")
        phase = entry.get("phase")
        severity = entry.get("default_severity")
        parameters = entry.get("required_parameters")
        documentation = entry.get("documentation")
        tests = entry.get("tests")

        if not isinstance(code, str) or not code:
            failures.append(f"central.entries[{index}]: code is required")
            continue
        if code in seen:
            failures.append(f"{code}: duplicate central catalog entry")
        seen.add(code)
        if not isinstance(title, str) or not title.strip():
            failures.append(f"{code}: title is required")
        else:
            failures.extend(validate_text(title, f"{code}.title"))
        if phase not in CENTRAL_PHASES:
            failures.append(f"{code}: phase must be one of {sorted(CENTRAL_PHASES)}")
        if severity not in SEVERITIES:
            failures.append(f"{code}: default_severity must be one of {sorted(SEVERITIES)}")
        if not isinstance(parameters, list) or not parameters:
            failures.append(f"{code}: required_parameters must be a non-empty array")
        elif not all(isinstance(value, str) and value.strip() for value in parameters):
            failures.append(f"{code}: required_parameters must contain only non-empty strings")
        if not isinstance(documentation, dict):
            failures.append(f"{code}: documentation is required")
        else:
            for field in required_doc_fields:
                value = documentation.get(field)
                if not isinstance(value, str) or not value.strip():
                    failures.append(f"{code}: documentation.{field} is required")
                else:
                    failures.extend(validate_text(value, f"{code}.documentation.{field}"))
        if not isinstance(tests, list) or not tests:
            failures.append(f"{code}: tests must contain at least one associated test path")
        else:
            has_snapshot = False
            for test_path in tests:
                if not isinstance(test_path, str) or not test_path:
                    failures.append(f"{code}: tests entries must be non-empty strings")
                    continue
                if test_path.endswith((".snap", ".must", ".json.must", ".txt.snap", ".json.snap")):
                    has_snapshot = True
                    failures.extend(validate_central_snapshot_assertions(code, test_path))
                if not (ROOT / test_path).exists():
                    failures.append(f"{code}: associated test path does not exist: {test_path}")
            if not has_snapshot:
                failures.append(f"{code}: tests must include at least one snapshot expectation")
    return failures


def validate_central_snapshot_assertions(code: str, test_path: str) -> list[str]:
    path = ROOT / test_path
    if not path.exists() or not test_path.startswith("tests/diagnostics/catalog/snapshots/central/") or not test_path.endswith(".snap"):
        return []
    text = path.read_text(encoding="utf-8")
    missing = sorted(assertion for assertion in CENTRAL_SNAPSHOT_ASSERTS if assertion not in text)
    if missing:
        return [f"{code}: snapshot {test_path} must assert {missing}"]
    return []


def central_catalog_entry_errors(entries: list[Any]) -> list[str]:
    failures: list[str] = []
    seen: set[str] = set()
    for entry in entries:
        if not isinstance(entry, dict):
            continue
        code = entry.get("code")
        tests = entry.get("tests")
        if not isinstance(code, str) or not code:
            continue
        if code in seen:
            failures.append(f"{code}: duplicate central catalog entry")
        seen.add(code)
        if not isinstance(tests, list) or not tests:
            failures.append(f"{code}: tests must contain at least one associated test path")
        elif not any(isinstance(path, str) and path.endswith((".snap", ".must", ".json.must", ".txt.snap", ".json.snap")) for path in tests):
            failures.append(f"{code}: tests must include at least one snapshot expectation")
    return failures


def validate_catalog_ci_invariant_contract() -> list[str]:
    duplicate_errors = central_catalog_entry_errors([
        {"code": "TEST_E_DUPLICATE", "tests": ["tests/diagnostics/input_matrix_test.py"]},
        {"code": "TEST_E_DUPLICATE", "tests": ["tests/diagnostics/input_matrix_test.py"]},
    ])
    if not any("duplicate central catalog entry" in error for error in duplicate_errors):
        return ["catalog CI invariant failed to reject duplicate diagnostic codes"]

    missing_test_errors = central_catalog_entry_errors([
        {"code": "TEST_E_NO_TESTS", "tests": []},
    ])
    if not any("tests must contain at least one associated test path" in error for error in missing_test_errors):
        return ["catalog CI invariant failed to reject diagnostics without tests"]

    missing_snapshot_errors = central_catalog_entry_errors([
        {"code": "TEST_E_NO_SNAPSHOT", "tests": ["tests/diagnostics/input_matrix_test.py"]},
    ])
    if not any("tests must include at least one snapshot expectation" in error for error in missing_snapshot_errors):
        return ["catalog CI invariant failed to reject diagnostics without snapshots"]

    return []


def validate_message_style_ci_invariant_contract() -> list[str]:
    forbidden = validate_text("something went wrong", "TEST.message")
    if not any("forbidden vague term" in error for error in forbidden):
        return ["catalog CI invariant failed to reject forbidden diagnostic message terms"]

    vague = validate_text("invalid program", "TEST.message")
    if not any("forbidden vague term" in error for error in vague):
        return ["catalog CI invariant failed to reject vague diagnostic message terms"]

    failed = validate_text("failed", "TEST.message")
    if not any("forbidden vague term" in error for error in failed):
        return ["catalog CI invariant failed to reject failed diagnostic message terms"]

    caused = validate_text("something went wrong because target object is missing", "TEST.message")
    if caused:
        return ["catalog CI invariant rejected a forbidden term followed by a precise cause"]

    precise = validate_text("binary literal uses a forbidden prefix", "TEST.message")
    if precise:
        return ["catalog CI invariant rejected a precise diagnostic message"]

    return []


def line_for_offset(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def split_call_arguments(text: str, start: int) -> tuple[list[str], int]:
    args: list[str] = []
    depth = 0
    in_string = False
    escaped = False
    arg_start = start
    index = start
    while index < len(text):
        char = text[index]
        if in_string:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
        else:
            if char == '"':
                in_string = True
            elif char == "(":
                depth += 1
            elif char == ")":
                if depth == 0:
                    tail = text[arg_start:index].strip()
                    if tail:
                        args.append(tail)
                    return args, index
                depth -= 1
            elif char == "," and depth == 0:
                args.append(text[arg_start:index].strip())
                arg_start = index + 1
        index += 1
    tail = text[arg_start:index].strip()
    if tail:
        args.append(tail)
    return args, index


def diagnostic_call_args(text: str) -> list[tuple[int, str, list[str]]]:
    calls: list[tuple[int, str, list[str]]] = []
    pattern = re.compile(r"\bdiagnostic_(?:create|error|warning|fatal)\s*\(")
    for match in pattern.finditer(text):
        line_start = text.rfind("\n", 0, match.start()) + 1
        prefix = text[line_start:match.start()].strip()
        if prefix.startswith("proc "):
            continue
        args, _ = split_call_arguments(text, match.end())
        first_arg = args[0] if args else ""
        calls.append((line_for_offset(text, match.start()), first_arg, args))
    return calls


def diagnostic_call_first_args(text: str) -> list[tuple[int, str]]:
    return [(line, first_arg) for line, first_arg, _args in diagnostic_call_args(text)]


def is_dynamic_code_expression(first_arg: str) -> bool:
    if CODE_LITERAL.fullmatch(first_arg):
        return False
    if "+" in first_arg:
        return True
    if " as " in first_arg:
        return True
    if "(" in first_arg or ")" in first_arg:
        return True
    if "[" in first_arg or "]" in first_arg:
        return True
    return False


def diagnostic_message_arg(args: list[str]) -> str:
    if len(args) >= 5:
        return args[3]
    if len(args) >= 4:
        return args[2]
    return ""


def is_ice_code_expression(first_arg: str) -> bool:
    literal = CODE_LITERAL.fullmatch(first_arg)
    return bool(literal and literal.group(1).startswith("ICE"))


def is_registered_message_expression(message_arg: str) -> bool:
    value = message_arg.strip()
    if value in {"message", "resolved_message", "localized_message", "title", "msg"}:
        return True
    if value.startswith("diagnostic_message("):
        return True
    if value.startswith("fluent_catalog_lookup("):
        return True
    if value.startswith("linker_failure_message("):
        return True
    if value.startswith("backend_registered_message("):
        return True
    return False


def validate_source_shape_renderer_fixtures() -> list[str]:
    failures: list[str] = []
    schema_root = ROOT / "tests" / "diagnostics" / "schema"
    source_root = ROOT / "tests" / "diagnostics" / "sources"
    for fixture in SOURCE_SHAPE_FIXTURES:
        if not (schema_root / fixture).is_file():
            failures.append(f"tests/diagnostics/schema/{fixture}: source-shape renderer fixture is required")
    for source in ("ascii.vit", "accents.vit", "unicode.vit", "tabs.vit", "long-line.vit", "empty.vit", "lf.vit", "crlf.vit"):
        path = source_root / source
        if not path.is_file():
            failures.append(f"tests/diagnostics/sources/{source}: source-shape diagnostic source is required")
    crlf = source_root / "crlf.vit"
    if crlf.is_file() and b"\r\n" not in crlf.read_bytes():
        failures.append("tests/diagnostics/sources/crlf.vit: CRLF fixture must contain CRLF line endings")
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
        text = path.read_text(encoding="utf-8")
        for line_number, first_arg, args in diagnostic_call_args(text):
            if is_dynamic_code_expression(first_arg):
                failures.append(f"{rel}:{line_number}: diagnostic code must not be constructed dynamically")
                continue
            message_arg = diagnostic_message_arg(args)
            if message_arg and not is_ice_code_expression(first_arg) and not is_registered_message_expression(message_arg):
                if rel not in ALLOWED_DYNAMIC_PRODUCERS:
                    failures.append(f"{rel}:{line_number}: diagnostic message must come from the registered catalog")
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


def validate_dynamic_code_guard_contract() -> list[str]:
    sample = '''
proc demo() -> Diagnostic {
  give diagnostic_error(
    "TYPECK_" + suffix,
    DiagnosticPhase.Typeck,
    "message",
    span
  )
}
'''
    calls = diagnostic_call_first_args(sample)
    if len(calls) != 1:
        return ["dynamic diagnostic-code guard failed to find a multi-line diagnostic call"]
    _line, first_arg = calls[0]
    if not is_dynamic_code_expression(first_arg):
        return ["dynamic diagnostic-code guard failed to reject a constructed code expression"]
    literal = '''
proc demo() -> Diagnostic {
  give diagnostic_error(
    "TYPECK_E_ASSIGN_MISMATCH",
    DiagnosticPhase.Typeck,
    "message",
    span
  )
}
'''
    literal_calls = diagnostic_call_first_args(literal)
    if len(literal_calls) != 1 or is_dynamic_code_expression(literal_calls[0][1]):
        return ["dynamic diagnostic-code guard rejected a literal diagnostic code"]
    return []


def validate_free_message_guard_contract() -> list[str]:
    sample = '''
proc demo() -> Diagnostic {
  give diagnostic_error(
    "TYPECK_E_ASSIGN_MISMATCH",
    DiagnosticPhase.Typeck,
    "free message",
    span
  )
}
'''
    calls = diagnostic_call_args(sample)
    if len(calls) != 1:
        return ["free diagnostic-message guard failed to find a multi-line diagnostic call"]
    _line, first_arg, args = calls[0]
    message_arg = diagnostic_message_arg(args)
    if is_ice_code_expression(first_arg):
        return ["free diagnostic-message guard misclassified a non-ICE code"]
    if is_registered_message_expression(message_arg):
        return ["free diagnostic-message guard failed to reject a literal message"]
    registered = '''
proc demo() -> Diagnostic {
  give diagnostic_error(
    "TYPECK_E_ASSIGN_MISMATCH",
    DiagnosticPhase.Typeck,
    diagnostic_message("TYPECK_E_ASSIGN_MISMATCH"),
    span
  )
}
'''
    registered_calls = diagnostic_call_args(registered)
    if len(registered_calls) != 1 or not is_registered_message_expression(diagnostic_message_arg(registered_calls[0][2])):
        return ["free diagnostic-message guard rejected a registered catalog message"]
    ice = '''
proc demo() -> Diagnostic {
  give diagnostic_fatal(
    "ICE0001",
    DiagnosticPhase.Internal,
    "internal compiler error",
    span
  )
}
'''
    ice_calls = diagnostic_call_args(ice)
    if len(ice_calls) != 1 or not is_ice_code_expression(ice_calls[0][1]):
        return ["free diagnostic-message guard rejected the ICE exception"]
    return []


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
        "Vitte item: \" + diagnostic.subject_symbol",
        "DIAGNOSTIC_USER_COLUMN_BASE",
        "DIAGNOSTIC_USER_COLUMN_UNIT",
        "DIAGNOSTIC_INTERNAL_OFFSET_UNIT",
        "DIAGNOSTIC_TAB_WIDTH",
        "monochrome_render_config",
        "terminal_render_config",
        "ide_render_config",
        "diagnostic_position_policy_text",
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
        "backend_remap_diagnostic_to_vitte_function",
        "vitte_function=",
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
        *validate_central_catalog(),
        *validate_catalog_ci_invariant_contract(),
        *validate_message_style_ci_invariant_contract(),
        *validate_source_shape_renderer_fixtures(),
        *validate_diagnostic_code_usage(),
        *validate_dynamic_code_guard_contract(),
        *validate_free_message_guard_contract(),
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
