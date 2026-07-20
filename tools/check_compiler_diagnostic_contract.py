#!/usr/bin/env python3
"""Enforce compiler diagnostic-code and output-boundary contracts."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CODES = ROOT / "schemas" / "diagnostics" / "codes.json"
COMPILER_ROOT = ROOT / "src" / "vitte" / "compiler"
COMPILER_PACKAGE_ROOT = ROOT / "src" / "vitte" / "packages" / "compiler"
CANONICAL_DIAGNOSTIC = COMPILER_ROOT / "diagnostics" / "diagnostic.vit"
CANONICAL_SPAN = COMPILER_ROOT / "source" / "span.vit"
SEMA_DIAGNOSTICS = COMPILER_ROOT / "analysis" / "sema" / "errors.vit"
SEMA_RESOLVER = COMPILER_ROOT / "analysis" / "sema" / "resolver.vit"
PARSER = COMPILER_ROOT / "frontend" / "parse" / "parser.vit"
TYPECK_DIAGNOSTICS = COMPILER_ROOT / "analysis" / "typeck" / "errors.vit"
TYPECK_TRAITS = COMPILER_ROOT / "analysis" / "typeck" / "traits.vit"
BORROWCK_DIAGNOSTICS = COMPILER_ROOT / "analysis" / "borrowck" / "errors.vit"
BORROWCK_LIFETIMES = COMPILER_ROOT / "analysis" / "borrowck" / "lifetimes.vit"
TYPE_UNIFY = COMPILER_ROOT / "analysis" / "typeck" / "unify.vit"
DIAGNOSTIC_CATALOG = COMPILER_ROOT / "diagnostics" / "catalog.vit"
DIAGNOSTIC_RENDER = COMPILER_ROOT / "diagnostics" / "render.vit"
DIAGNOSTIC_JSON = COMPILER_ROOT / "diagnostics" / "json.vit"
DIAGNOSTIC_LSP = COMPILER_ROOT / "diagnostics" / "lsp.vit"
DIAGNOSTIC_SARIF = COMPILER_ROOT / "diagnostics" / "sarif.vit"
BACKEND_DIAGNOSTICS = COMPILER_ROOT / "backend" / "diagnostics.vit"
MIDDLE_TYPECHECK_DIAGNOSTICS = COMPILER_ROOT / "middle" / "typecheck" / "diagnostics.vit"
DIAGNOSTICS_MIGRATION_CHECK = ROOT / "tools" / "check_diagnostics_migration.py"
DIAGNOSTIC_MESSAGE_STYLE = ROOT / "tools" / "check_diagnostic_message_style.py"
DRIVER_COMPILER = COMPILER_ROOT / "driver" / "compiler.vit"
DIAGNOSTIC_SNAPSHOT_TESTS = COMPILER_ROOT / "tests" / "diagnostic_snapshot_tests.vit"
DIAGNOSTIC_SNAPSHOT_SUITE = ROOT / "tests" / "diagnostics" / "catalog" / "snapshots" / "diagnostic-suite.snap"
DIAGNOSTICS_DOC = ROOT / "docs" / "compiler" / "diagnostics.md"
DIAGNOSTIC_EXAMPLES_DOC = ROOT / "docs" / "compiler" / "diagnostic_examples.md"
DIAGNOSTIC_STYLE_GUIDE = ROOT / "docs" / "compiler" / "diagnostic_style_guide.md"
DIAGNOSTIC_VOCABULARY_AUDIT = ROOT / "docs" / "compiler" / "diagnostic_vocabulary_audit.md"

DIRECT_OUTPUT = re.compile(r"\b(?:print|printf|fprintf|eprintf|fputs|fwrite|fputc)\s*\(")
VAGUE_DRIVER_MESSAGES = (
    "compilation failed",
    "invalid program",
    "unexpected error",
    "semantic error",
    "type error",
    "unknown failure",
)
VAGUE_DIAGNOSTIC_PHRASES = (
    "incompatible types",
    "types incompatibles",
    "types are incompatible",
    "type compatibility rule was violated",
    "type mismatch",
)
FORBIDDEN_AUTOFIX_REPLACEMENTS = (
    '"todo()"',
    '"(...)"',
    '"<" +',
    '"<shorter borrow',
    '"use module {"',
    'extern \\"C\\" { ... }',
)
AUTOFIX_PRODUCERS = (
    "diagnostic_with_insert",
    "diagnostic_with_replace",
    "diagnostic_suggestion_applicability",
)
FORBIDDEN_USER_DIAGNOSTIC_FRAGMENTS = (
    "borrowck phase:",
    "entity:",
    "LoanId",
    "RegionId",
    "MovePath",
    "region constraints",
)
ALLOWED_OUTPUT_BOUNDARIES = {
    "src/vitte/compiler/driver/compiler.vit",
    "src/vitte/compiler/diagnostics/render.vit",
    "src/vitte/compiler/diagnostics/json.vit",
    "src/vitte/compiler/diagnostics/report.vit",
    "src/vitte/compiler/infrastructure/diagnostics/emitter.vit",
}
ALLOWED_RUNTIME_OUTPUT = {
    "src/vitte/compiler/backends/runtime_c/vitte_runtime.c",
    "src/vitte/compiler/backends/runtime_c/vitte_runtime.h",
}
REQUIRED_DIAGNOSTIC_FIELDS = (
    "code",
    "severity",
    "title",
    "message",
    "span",
    "secondary_spans",
    "notes",
    "helps",
    "suggestions",
    "phase",
    "file_id",
    "internal_cause",
)
REQUIRED_CANONICAL_INITIALIZERS = (
    "title: effective_message",
    "secondary_spans: []",
    "primary_span: span",
    "span: span",
    "file_id: 0",
    "internal_cause: cause",
)
REQUIRED_SPAN_WRAPPER_FORMS = (
    "PrimarySpan",
    "SecondarySpan",
)
REQUIRED_SPAN_WRAPPER_HELPERS = (
    "proc primary_span(span: SourceSpan, message: string) -> PrimarySpan",
    "proc secondary_span(span: SourceSpan, message: string) -> SecondarySpan",
    "proc primary_span_label(value: PrimarySpan) -> DiagnosticLabel",
    "proc secondary_span_label(value: SecondarySpan) -> DiagnosticLabel",
)


def fail(message: str) -> int:
    print(f"[compiler-diagnostics-contract][error] {message}", file=sys.stderr)
    return 1


def strip_string_literals(line: str) -> str:
    out: list[str] = []
    in_string = False
    escaped = False
    quote = ""
    for ch in line:
        if in_string:
            if escaped:
                escaped = False
                continue
            if ch == "\\":
                escaped = True
                continue
            if ch == quote:
                in_string = False
                quote = ""
            continue
        if ch in {"'", '"'}:
            in_string = True
            quote = ch
            continue
        out.append(ch)
    return "".join(out)


def compiler_sources() -> list[Path]:
    return sorted(
        path
        for path in COMPILER_ROOT.rglob("*")
        if path.suffix in {".vit", ".c", ".h"}
        and "/tests/" not in path.as_posix()
        and not path.as_posix().endswith("/tests.vit")
    )


def compiler_contract_sources() -> list[Path]:
    roots = (COMPILER_ROOT, COMPILER_PACKAGE_ROOT)
    return sorted(
        path
        for root in roots
        for path in root.rglob("*")
        if path.suffix in {".vit", ".c", ".h"}
        and "/tests/" not in path.as_posix()
        and not path.as_posix().endswith("/tests.vit")
    )


def check_code_documentation() -> list[str]:
    payload = json.loads(CODES.read_text(encoding="utf-8"))
    failures: list[str] = []
    if payload.get("schema") != "vitte.diagnostic-codes":
        failures.append("registry schema must be vitte.diagnostic-codes")
    if payload.get("schema_version") != "2.0.0":
        failures.append("registry schema_version must be 2.0.0")
    if payload.get("source") != "locales/en/diagnostics.ftl":
        failures.append("registry source must be locales/en/diagnostics.ftl")
    seen_codes: set[str] = set()
    seen_aliases: dict[str, str] = {}
    for entry in payload.get("codes", []):
        if not isinstance(entry, dict):
            failures.append("registry contains a non-object entry")
            continue
        code = entry.get("code")
        phase = entry.get("phase")
        documentation = entry.get("documentation")
        if not isinstance(code, str) or not isinstance(phase, str):
            failures.append(f"registry entry has invalid code or phase: {entry!r}")
            continue
        if code in seen_codes:
            failures.append(f"{code}: public code is reused")
        seen_codes.add(code)
        aliases = entry.get("aliases")
        if not isinstance(aliases, list) or not aliases:
            failures.append(f"{code}: missing legacy alias")
        else:
            for alias in aliases:
                if not isinstance(alias, str):
                    failures.append(f"{code}: non-string alias")
                    continue
                previous = seen_aliases.get(alias)
                if previous and previous != code:
                    failures.append(f"{alias}: alias maps to both {previous} and {code}")
                seen_aliases[alias] = code
        if not isinstance(documentation, dict):
            failures.append(f"{code}: missing documentation")
            continue
        for field in ("title", "summary", "cause", "action", "example", "url"):
            if not isinstance(documentation.get(field), str) or not documentation[field].strip():
                failures.append(f"{code}: documentation.{field} is required")
        if isinstance(documentation.get("url"), str) and f"/{phase}/{code}" not in documentation["url"]:
            failures.append(f"{code}: documentation url must include the producing phase")
        tests = entry.get("tests")
        if not isinstance(tests, list) or not tests:
            failures.append(f"{code}: at least one diagnostic test is required")
        else:
            for test in tests:
                if not isinstance(test, dict):
                    failures.append(f"{code}: diagnostic test entry must be an object")
                    continue
                test_path = test.get("path")
                test_case = test.get("case")
                asserts = test.get("asserts")
                if not isinstance(test_path, str) or not test_path:
                    failures.append(f"{code}: diagnostic test path is required")
                elif not (ROOT / test_path).exists():
                    failures.append(f"{code}: diagnostic test path does not exist: {test_path}")
                if test_case != code:
                    failures.append(f"{code}: diagnostic test case must match the code")
                if not isinstance(asserts, list) or "code" not in asserts or "span" not in asserts:
                    failures.append(f"{code}: diagnostic test must assert code and span")
    return failures


def check_catalog_codes_present() -> list[str]:
    payload = json.loads(CODES.read_text(encoding="utf-8"))
    failures: list[str] = []
    for index, entry in enumerate(payload.get("codes", [])):
        if not isinstance(entry, dict):
            continue
        code = entry.get("code")
        if not isinstance(code, str) or not code.strip():
            failures.append(f"catalog entry #{index}: diagnostic code is required")
    return failures


def check_catalog_codes_unique() -> list[str]:
    payload = json.loads(CODES.read_text(encoding="utf-8"))
    failures: list[str] = []
    seen: dict[str, int] = {}
    for index, entry in enumerate(payload.get("codes", [])):
        if not isinstance(entry, dict):
            continue
        code = entry.get("code")
        if not isinstance(code, str) or not code.strip():
            continue
        previous = seen.get(code)
        if previous is not None:
            failures.append(f"{code}: duplicate diagnostic code at entries #{previous} and #{index}")
        seen[code] = index
    return failures


def check_direct_output_boundaries() -> list[str]:
    failures: list[str] = []
    for path in compiler_sources():
        rel = path.relative_to(ROOT).as_posix()
        if rel in ALLOWED_OUTPUT_BOUNDARIES or rel in ALLOWED_RUNTIME_OUTPUT:
            continue
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            code_line = strip_string_literals(line)
            if DIRECT_OUTPUT.search(code_line):
                failures.append(f"{rel}:{line_number}: direct output call must flow through diagnostics/driver boundary")
    return failures


def check_driver_vague_messages() -> list[str]:
    driver = ROOT / "src" / "vitte" / "compiler" / "driver" / "compiler.vit"
    text = driver.read_text(encoding="utf-8").lower()
    return [
        f"{driver.relative_to(ROOT)}: vague driver message is forbidden: {message!r}"
        for message in VAGUE_DRIVER_MESSAGES
        if message in text
    ]


def check_vitte_vocabulary_contract() -> list[str]:
    failures: list[str] = []
    style = DIAGNOSTIC_STYLE_GUIDE.read_text(encoding="utf-8")
    audit = DIAGNOSTIC_VOCABULARY_AUDIT.read_text(encoding="utf-8")
    required = (
        "Use `procedure` for the user-facing callable concept.",
        "Do not use `function` for the user-facing callable concept.",
        "Do not use `routine` for the user-facing callable concept.",
        "Do not use `return` when the diagnostic is describing Vitte syntax.",
        "lexer, parser, resolver, sema, typeck, borrowck",
    )
    if "`procedure` for the user-facing concept, never function or routine" not in style:
        failures.append(f"{DIAGNOSTIC_STYLE_GUIDE.relative_to(ROOT)}: missing canonical procedure terminology rule")
    for fragment in required:
        if fragment not in audit:
            failures.append(f"{DIAGNOSTIC_VOCABULARY_AUDIT.relative_to(ROOT)}: missing vocabulary audit fragment {fragment!r}")
    return failures


def extract_form_body(text: str, form_name: str) -> str | None:
    match = re.search(rf"\bform\s+{re.escape(form_name)}\s*\{{", text)
    if not match:
        return None
    depth = 1
    i = match.end()
    while i < len(text):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[match.end():i]
        i += 1
    return None


def form_fields(text: str, form_name: str) -> set[str] | None:
    body = extract_form_body(text, form_name)
    if body is None:
        return None
    fields: set[str] = set()
    for line in body.splitlines():
        match = re.match(r"\s*([A-Za-z_][A-Za-z0-9_]*)\s*:", line)
        if match:
            fields.add(match.group(1))
    return fields


def check_diagnostic_object_contract() -> list[str]:
    failures: list[str] = []
    diagnostic_forms: list[str] = []
    for path in compiler_sources():
        text = path.read_text(encoding="utf-8")
        if re.search(r"(?m)^form\s+Diagnostic\s*\{", text):
            diagnostic_forms.append(path.relative_to(ROOT).as_posix())
    allowed = CANONICAL_DIAGNOSTIC.relative_to(ROOT).as_posix()
    for path in diagnostic_forms:
        if path != allowed:
            failures.append(f"{path}: only {allowed} may define form Diagnostic")
    if allowed not in diagnostic_forms:
        failures.append(f"{allowed}: missing unique form Diagnostic")

    text = CANONICAL_DIAGNOSTIC.read_text(encoding="utf-8")
    rel = CANONICAL_DIAGNOSTIC.relative_to(ROOT)
    fields = form_fields(text, "Diagnostic")
    if fields is None:
        failures.append(f"{rel}: missing form Diagnostic")
        return failures
    for field in REQUIRED_DIAGNOSTIC_FIELDS:
        if field not in fields:
            failures.append(f"{rel}: Diagnostic is missing required field {field!r}")
    for initializer in REQUIRED_CANONICAL_INITIALIZERS:
        if initializer not in text:
            failures.append(f"{rel}: diagnostic_create must initialize {initializer!r}")
    for form_name in REQUIRED_SPAN_WRAPPER_FORMS:
        fields = form_fields(text, form_name)
        if fields is None:
            failures.append(f"{rel}: missing form {form_name}")
            continue
        for field in ("span", "message", "valid"):
            if field not in fields:
                failures.append(f"{rel}: {form_name} is missing required field {field!r}")
    for helper in REQUIRED_SPAN_WRAPPER_HELPERS:
        if helper not in text:
            failures.append(f"{rel}: missing span wrapper helper {helper!r}")
    if "set diagnostic.secondary_spans = diagnostic.secondary_spans + [label.span]" not in text:
        failures.append(f"{rel}: secondary labels must preserve their spans in secondary_spans")
    if "set diagnostic.primary_span = label.span" not in text or "set diagnostic.span = label.span" not in text:
        failures.append(f"{rel}: primary labels must synchronize span and primary_span")
    if "proc diagnostic_with_file_id" not in text:
        failures.append(f"{rel}: missing diagnostic_with_file_id helper")
    if "proc diagnostic_with_internal_cause" not in text:
        failures.append(f"{rel}: missing diagnostic_with_internal_cause helper")
    return failures


def check_span_object_contract() -> list[str]:
    failures: list[str] = []
    span_forms: list[str] = []
    for path in compiler_contract_sources():
        text = path.read_text(encoding="utf-8")
        if re.search(r"(?m)^form\s+Span\s*\{", text):
            span_forms.append(path.relative_to(ROOT).as_posix())
    allowed = CANONICAL_SPAN.relative_to(ROOT).as_posix()
    for path in span_forms:
        if path != allowed:
            failures.append(f"{path}: only {allowed} may define form Span")
    if allowed not in span_forms:
        failures.append(f"{allowed}: missing unique form Span")
    return failures


def check_relational_diagnostic_contract() -> list[str]:
    failures: list[str] = []
    checked_paths = (
        CANONICAL_DIAGNOSTIC,
        SEMA_DIAGNOSTICS,
        SEMA_RESOLVER,
        PARSER,
        TYPECK_DIAGNOSTICS,
        BORROWCK_DIAGNOSTICS,
        BORROWCK_LIFETIMES,
        TYPE_UNIFY,
        DIAGNOSTIC_CATALOG,
        DIAGNOSTIC_RENDER,
        DIAGNOSTIC_JSON,
        DIAGNOSTIC_LSP,
        DIAGNOSTIC_SARIF,
        BACKEND_DIAGNOSTICS,
        MIDDLE_TYPECHECK_DIAGNOSTICS,
        DRIVER_COMPILER,
    )
    for path in checked_paths:
        text = path.read_text(encoding="utf-8").lower()
        rel = path.relative_to(ROOT)
        for phrase in VAGUE_DIAGNOSTIC_PHRASES:
            if phrase in text:
                failures.append(f"{rel}: vague diagnostic phrase is forbidden: {phrase!r}")
        for line_number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if any(producer in line for producer in AUTOFIX_PRODUCERS):
                for fragment in FORBIDDEN_AUTOFIX_REPLACEMENTS:
                    if fragment in line:
                        failures.append(f"{rel}:{line_number}: non-canonical Vitte autofix replacement is forbidden: {fragment!r}")
            for string_literal in re.findall(r'"([^"\\\\]*(?:\\\\.[^"\\\\]*)*)"', line):
                for fragment in FORBIDDEN_USER_DIAGNOSTIC_FRAGMENTS:
                    if fragment.lower() in string_literal.lower():
                        failures.append(f"{rel}:{line_number}: internal borrow diagnostic jargon is forbidden: {fragment!r}")

    required_fragments = (
        (SEMA_DIAGNOSTICS, "sema_duplicate_symbol_redefinition"),
        (SEMA_DIAGNOSTICS, "first declaration of"),
        (SEMA_DIAGNOSTICS, "redefinition appears here"),
        (SEMA_DIAGNOSTICS, "SemaResolutionFailureKind"),
        (SEMA_DIAGNOSTICS, "sema_public_code"),
        (SEMA_DIAGNOSTICS, "SEMA_E_INVALID_ASSIGN_TARGET"),
        (SEMA_DIAGNOSTICS, "give \"SEM0015\";"),
        (SEMA_DIAGNOSTICS, "SEMA_E_INVALID_EXPORT"),
        (SEMA_DIAGNOSTICS, "give \"SEM0018\";"),
        (SEMA_DIAGNOSTICS, "SEMA_E_INVALID_MODULE"),
        (SEMA_DIAGNOSTICS, "give \"SEM0021\";"),
        (SEMA_DIAGNOSTICS, "SEMA_E_UNKNOWN_SYMBOL"),
        (SEMA_DIAGNOSTICS, "give \"SEM0029\";"),
        (SEMA_DIAGNOSTICS, "RES0072"),
        (SEMA_DIAGNOSTICS, "RES0119"),
        (SEMA_DIAGNOSTICS, "RES0067"),
        (SEMA_DIAGNOSTICS, "RES0070"),
        (SEMA_DIAGNOSTICS, "RES0065"),
        (SEMA_DIAGNOSTICS, "RES0064"),
        (SEMA_DIAGNOSTICS, "RES0095"),
        (SEMA_DIAGNOSTICS, "RES0127"),
        (SEMA_DIAGNOSTICS, "MissingSymbol"),
        (SEMA_DIAGNOSTICS, "OutOfScopeSymbol"),
        (SEMA_DIAGNOSTICS, "PrivateSymbol"),
        (SEMA_DIAGNOSTICS, "RenamedImport"),
        (SEMA_DIAGNOSTICS, "AmbiguousSymbol"),
        (SEMA_DIAGNOSTICS, "MissingModule"),
        (SEMA_DIAGNOSTICS, "InaccessibleModule"),
        (SEMA_DIAGNOSTICS, "ImportCycle"),
        (SEMA_DIAGNOSTICS, "InvalidModulePath"),
        (SEMA_DIAGNOSTICS, "resolution failure kind: "),
        (SEMA_DIAGNOSTICS, "symbol `\" + name + \"` exists but is outside the current scope"),
        (SEMA_DIAGNOSTICS, "symbol `\" + name + \"` is private in the exporting module"),
        (SEMA_DIAGNOSTICS, "symbol `\" + name + \"` was imported under another name"),
        (SEMA_DIAGNOSTICS, "symbol `\" + name + \"` resolves to multiple visible candidates"),
        (SEMA_DIAGNOSTICS, "module `\" + name + \"` does not exist"),
        (SEMA_DIAGNOSTICS, "module `\" + name + \"` exists but is not accessible from here"),
        (SEMA_DIAGNOSTICS, "import cycle reaches module"),
        (SEMA_DIAGNOSTICS, "module path `\" + name + \"` is not a valid Vitte module path"),
        (SEMA_DIAGNOSTICS, "sema_symbol_category_compatible"),
        (SEMA_DIAGNOSTICS, "candidate accepted for `\" + expected_context + \"` context; ranked by scope, symbol kind, argument count when known, expected type when known, visibility, and spelling distance"),
        (SEMA_DIAGNOSTICS, "suggestions are limited to the three most credible compatible candidates"),
        (SEMA_DIAGNOSTICS, "emitted_count >= 3"),
        (SEMA_DIAGNOSTICS, "sema_symbol_suggestion_seen"),
        (SEMA_DIAGNOSTICS, "skipped duplicate suggestion"),
        (SEMA_DIAGNOSTICS, "skipped \" + category + \" `\" + nearby + \"` because `\" + expected_context + \"` context does not accept that symbol kind"),
        (SEMA_DIAGNOSTICS, "false, \"medium\", \"the symbol may be exported by another module, but the exact module path must be confirmed\""),
        (SEMA_DIAGNOSTICS, "write a canonical `use` path that names an existing module and exported symbol"),
        (SEMA_RESOLVER, "sema_ambiguous_symbol"),
        (SEMA_RESOLVER, "sema_out_of_scope_symbol"),
        (SEMA_RESOLVER, "sema_inaccessible_module"),
        (PARSER, "parser_rule_for_code"),
        (PARSER, "parser_public_code"),
        (PARSER, "parser_recovery_allowed"),
        (PARSER, "default_recovery_control"),
        (PARSER, "recovery_allowed(control, len(state.diagnostics), state.recovery_count)"),
        (PARSER, "P000_UNBALANCED"),
        (PARSER, "give \"PAR0022\";"),
        (PARSER, "PATTR003"),
        (PARSER, "give \"PAR0039\";"),
        (PARSER, "PLOOP"),
        (PARSER, "give \"PAR0040\";"),
        (PARSER, "PPRIMARY999"),
        (PARSER, "give \"PAR0041\";"),
        (PARSER, "PSTMT007"),
        (PARSER, "give \"PAR0042\";"),
        (PARSER, "parser_context_for_code"),
        (PARSER, "expected_text_message_for_code"),
        (PARSER, "is missing semicolon `;`"),
        (PARSER, "is missing closing brace `}`"),
        (PARSER, "is missing closing parenthesis `)`"),
        (PARSER, "is missing comma `,`"),
        (PARSER, "type annotation is missing a Vitte type name or type expression"),
        (PARSER, "expression is missing a Vitte value"),
        (PARSER, "procedure declaration is missing its body"),
        (PARSER, "declaration is incomplete"),
        (PARSER, "modifier `\" + last_modifier + \"` is only valid before a procedure declaration here"),
        (PARSER, "top-level declaration expected a Vitte declaration keyword"),
        (PARSER, "assignment statement is missing an assignment operator"),
        (PARSER, "old Vitte container keyword"),
        (PARSER, "write `form` for this declaration"),
        (PARSER, "write `pick` for this declaration"),
        (TYPECK_DIAGNOSTICS, "why expected type is imposed:"),
        (TYPECK_DIAGNOSTICS, "typeck_expected_obtained_explanation"),
        (TYPECK_DIAGNOSTICS, "Expected type `\" + expected + \"` because"),
        (TYPECK_DIAGNOSTICS, "obtained type `\" + found + \"` from"),
        (TYPECK_DIAGNOSTICS, "typeck_public_code"),
        (TYPECK_DIAGNOSTICS, "TYPECK_E_ASSIGN_MISMATCH"),
        (TYPECK_DIAGNOSTICS, "give \"TYP0213\";"),
        (TYPECK_DIAGNOSTICS, "TYPECK_E_ARGUMENT_MISMATCH"),
        (TYPECK_DIAGNOSTICS, "give \"TYP0212\";"),
        (TYPECK_DIAGNOSTICS, "TYPECK_E_RETURN_MISMATCH"),
        (TYPECK_DIAGNOSTICS, "give \"TYP0243\";"),
        (TYPECK_DIAGNOSTICS, "TYPECK_E_UNKNOWN_TYPE"),
        (TYPECK_DIAGNOSTICS, "give \"TYP0249\";"),
        (TYPECK_DIAGNOSTICS, "origin of obtained type:"),
        (TYPECK_DIAGNOSTICS, "Expected type `"),
        (TYPECK_DIAGNOSTICS, "obtained type `"),
        (TYPECK_DIAGNOSTICS, "Convert the value or modify the signature"),
        (TYPECK_DIAGNOSTICS, "expected `"),
        (TYPECK_DIAGNOSTICS, "obtained `"),
        (TYPECK_DIAGNOSTICS, "pick pattern is not exhaustive"),
        (TYPECK_DIAGNOSTICS, "proc argument compatibility"),
        (TYPECK_DIAGNOSTICS, "proc call arity"),
        (TYPECK_DIAGNOSTICS, "expression is not a callable proc"),
        (BORROWCK_DIAGNOSTICS, "value was moved here"),
        (BORROWCK_DIAGNOSTICS, "borrowck_public_code"),
        (BORROWCK_DIAGNOSTICS, "BORROWCK_E_USE_AFTER_MOVE"),
        (BORROWCK_DIAGNOSTICS, "give \"BOR0020\";"),
        (BORROWCK_DIAGNOSTICS, "BORROWCK_E_MOVE_WHILE_BORROWED"),
        (BORROWCK_DIAGNOSTICS, "give \"BOR0011\";"),
        (BORROWCK_DIAGNOSTICS, "BORROWCK_E_MUTABLE_BORROW_CONFLICT"),
        (BORROWCK_DIAGNOSTICS, "give \"BOR0013\";"),
        (BORROWCK_DIAGNOSTICS, "BORROWCK_E_LIFETIME_TOO_SHORT"),
        (BORROWCK_DIAGNOSTICS, "give \"BOR0008\";"),
        (BORROWCK_DIAGNOSTICS, "borrowck_chronology_step"),
        (BORROWCK_DIAGNOSTICS, "\"move\""),
        (BORROWCK_DIAGNOSTICS, "\"borrow\""),
        (BORROWCK_DIAGNOSTICS, "\"reuse\""),
        (BORROWCK_DIAGNOSTICS, "\"chronology step \" + step_text + \" [\" + event_kind + \"]: `\" + name + \"` \" + detail"),
        (BORROWCK_DIAGNOSTICS, "borrow is still active at the conflict"),
        (BORROWCK_DIAGNOSTICS, "conflict appears here"),
        (BORROWCK_DIAGNOSTICS, "borrowck_chronology_step(kind, name, 1"),
        (BORROWCK_DIAGNOSTICS, "borrowck_chronology_step(kind, name, 2"),
        (BORROWCK_DIAGNOSTICS, "borrowck_chronology_step(kind, name, 3"),
        (BORROWCK_DIAGNOSTICS, "borrowck_chronology_step(kind, name, 4"),
        (BORROWCK_DIAGNOSTICS, "where the value is reused:"),
        (BORROWCK_DIAGNOSTICS, "the compiler is following ownership through the control flow"),
        (BORROWCK_DIAGNOSTICS, "the compiler is checking that every borrow ends before its owner is destroyed"),
        (BORROWCK_DIAGNOSTICS, "BorrowErrorKind.PartialMove"),
        (BORROWCK_DIAGNOSTICS, "borrow_error_partial_move_story"),
        (BORROWCK_DIAGNOSTICS, "a mutable borrow requires exclusive access, so it cannot overlap another mutable or immutable borrow"),
        (BORROWCK_DIAGNOSTICS, "immutable borrow of `\" + name + \"` conflicts with an active mutable borrow"),
        (BORROWCK_DIAGNOSTICS, "cannot `set` value while it is borrowed"),
        (BORROWCK_DIAGNOSTICS, "cannot destroy value while it is borrowed"),
        (BORROWCK_DIAGNOSTICS, "borrow_error_drop_while_borrowed_story"),
        (BORROWCK_DIAGNOSTICS, "borrow_error_assign_while_borrowed_story"),
        (BORROWCK_DIAGNOSTICS, "borrow_error_return_ref_to_local_story"),
        (BORROWCK_LIFETIMES, "borrow may outlive its owner"),
        (BORROWCK_LIFETIMES, "owner must stay alive for this use"),
        (BORROWCK_DIAGNOSTICS, "shorten the borrow, clone explicitly, or move the later access"),
        (CANONICAL_DIAGNOSTIC, "mir_ir_public_code"),
        (CANONICAL_DIAGNOSTIC, "MIR_E_INVALID_TERMINATOR"),
        (CANONICAL_DIAGNOSTIC, "give \"MIR0113\";"),
        (CANONICAL_DIAGNOSTIC, "MIR_E_CFG_INCOHERENT"),
        (CANONICAL_DIAGNOSTIC, "give \"MIR0139\";"),
        (CANONICAL_DIAGNOSTIC, "IR_E_INVALID_MODULE"),
        (CANONICAL_DIAGNOSTIC, "give \"IR0039\";"),
        (CANONICAL_DIAGNOSTIC, "IR_E_INVALID_TERMINATOR"),
        (CANONICAL_DIAGNOSTIC, "give \"IR0036\";"),
        (DIAGNOSTICS_MIGRATION_CHECK, "UNSTRUCTURED_DIAGNOSTIC_RE"),
        (DIAGNOSTICS_MIGRATION_CHECK, "STRUCTURED_DIAGNOSTIC_ALLOWLIST"),
        (DIAGNOSTICS_MIGRATION_CHECK, "validate_no_unstructured_diagnostics"),
        (DIAGNOSTIC_RENDER, "render_terminal_report"),
        (DIAGNOSTIC_RENDER, "render_terminal_diagnostic"),
        (DIAGNOSTIC_RENDER, "set ctx.color_mode = color_mode;"),
        (DIAGNOSTIC_RENDER, "set ctx.config.width = width;"),
        (DIAGNOSTIC_JSON, "json_diagnostic_output"),
        (DIAGNOSTIC_JSON, "\"canonical Diagnostic JSON renderer\""),
        (DIAGNOSTIC_JSON, "\"diagnostics are serialized from the shared DiagnosticReport\""),
        (DIAGNOSTIC_JSON, "share json_diagnostic_output"),
        (DIAGNOSTIC_LSP, "lsp_diagnostic_output"),
        (DIAGNOSTIC_LSP, "\"canonical Diagnostic LSP renderer\""),
        (DIAGNOSTIC_LSP, "lsp_publish_report_with_canonical_context(uri, report, version, context)"),
        (DIAGNOSTIC_LSP, "share lsp_diagnostic_output"),
        (DRIVER_COMPILER, "cli_explain_command_contract"),
        (DRIVER_COMPILER, "usage: vitte explain CODE"),
        (DRIVER_COMPILER, "diagnostic_explain_text_lang(request.input_path, request.lang)"),
        (DRIVER_COMPILER, "cli_check_command_contract"),
        (DRIVER_COMPILER, "command == \"check\" and mode == CompilerMode::Check"),
        (DRIVER_COMPILER, "check command must run diagnostics without final generation"),
        (DRIVER_COMPILER, "error_format: string"),
        (DRIVER_COMPILER, "cli_normalize_error_format"),
        (DRIVER_COMPILER, "text == \"json\""),
        (DRIVER_COMPILER, "text == \"lsp\""),
        (DRIVER_COMPILER, "arg == \"--error-format\""),
        (DRIVER_COMPILER, "color: string"),
        (DRIVER_COMPILER, "cli_normalize_color"),
        (DRIVER_COMPILER, "text == \"auto\""),
        (DRIVER_COMPILER, "text == \"always\""),
        (DRIVER_COMPILER, "text == \"never\""),
        (DRIVER_COMPILER, "arg == \"--color\""),
        (DRIVER_COMPILER, "max_errors: u64"),
        (DRIVER_COMPILER, "arg == \"--max-errors\""),
        (DRIVER_COMPILER, "set request.max_errors = args[i + 1] as u64;"),
        (DRIVER_COMPILER, "deny_warnings: bool"),
        (DRIVER_COMPILER, "arg == \"--deny-warnings\""),
        (DRIVER_COMPILER, "set request.deny_warnings = true;"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_public_severity_level"),
        (CANONICAL_DIAGNOSTIC, "give \"help\";"),
        (CANONICAL_DIAGNOSTIC, "give \"note\";"),
        (CANONICAL_DIAGNOSTIC, "give \"warning\";"),
        (CANONICAL_DIAGNOSTIC, "give \"error\";"),
        (CANONICAL_DIAGNOSTIC, "share diagnostic_public_severity_level"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_suggestion_with_confidence"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_has_machine_applicable_fix"),
        (CANONICAL_DIAGNOSTIC, "confidence == DIAGNOSTIC_CONFIDENCE_HIGH and replacement != \"\""),
        (CANONICAL_DIAGNOSTIC, "confidence == DIAGNOSTIC_CONFIDENCE_MEDIUM"),
        (CANONICAL_DIAGNOSTIC, "DiagnosticApplicability.Manual"),
        (CANONICAL_DIAGNOSTIC, "share diagnostic_suggestion_with_confidence"),
        (CANONICAL_DIAGNOSTIC, "share diagnostic_has_machine_applicable_fix"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_suggestion_replacement_is_valid_vitte"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_suggestions_produce_valid_vitte"),
        (CANONICAL_DIAGNOSTIC, "share diagnostic_suggestions_produce_valid_vitte"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "test_machine_applicable_fix_contract"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "diagnostic_has_machine_applicable_fix(diag)"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "test_automatic_correction_application_contract"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "apply_parser_missing_brace_fix_for_test"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "fixed == \"proc main() -> int { give 0 }\""),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "test_recompile_after_applied_fix_contract"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "vitte_fix_recompile_probe_for_test"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "vitte_fix_recompile_probe_for_test(fixed)"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "test_each_suggestion_produces_valid_vitte_contract"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "diagnostic_suggestions_produce_valid_vitte(parser)"),
        (DIAGNOSTICS_DOC, "The compiler diagnostic system is a stable API"),
        (DIAGNOSTICS_DOC, "Every user-facing diagnostic is represented by"),
        (DIAGNOSTICS_DOC, "`vitte check` is the reference command for diagnostics without final"),
        (DIAGNOSTICS_DOC, "Machine-applicable suggestions must include a concrete replacement"),
        (DIAGNOSTIC_EXAMPLES_DOC, "# Diagnostic Examples"),
        (DIAGNOSTIC_EXAMPLES_DOC, "## Parser"),
        (DIAGNOSTIC_EXAMPLES_DOC, "## Resolver"),
        (DIAGNOSTIC_EXAMPLES_DOC, "## Typeck"),
        (DIAGNOSTIC_EXAMPLES_DOC, "## Borrowck"),
        (DIAGNOSTIC_EXAMPLES_DOC, "## Linker"),
        (DIAGNOSTIC_EXAMPLES_DOC, "Corrected:"),
        (DIAGNOSTIC_MESSAGE_STYLE, "VAGUE_TERMS_REQUIRING_CAUSE"),
        (DIAGNOSTIC_MESSAGE_STYLE, "CAUSE_MARKERS"),
        (DIAGNOSTIC_MESSAGE_STYLE, "vague_term_has_precise_cause"),
        (DIAGNOSTIC_MESSAGE_STYLE, "uses vague term without a precise cause"),
        (DIAGNOSTIC_SNAPSHOT_SUITE, "schema: vitte.diagnostic-snapshot-suite"),
        (DIAGNOSTIC_SNAPSHOT_SUITE, "one-code-one-snapshot"),
        (DIAGNOSTIC_SNAPSHOT_SUITE, "machine-applicable-fix"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "test_multiple_independent_errors_in_one_file"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "multi_errors.vit"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "test_multifile_diagnostic_spans"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "app/helper.vit"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "test_unicode_and_tab_diagnostic_rendering"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "caractère café\\tinterdit"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "DIAGNOSTIC_TAB_WIDTH == 4"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "test_cascade_diagnostics_link_to_root"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "cascade_of == report.diagnostics[0].primary_error_key"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "test_duplicate_diagnostics_are_suppressed"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "duplicate_count == 1"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "test_diagnostic_order_is_stable_across_runs"),
        (DIAGNOSTIC_SNAPSHOT_TESTS, "left.diagnostics[0].code == right.diagnostics[0].code"),
        (CANONICAL_DIAGNOSTIC, "why expected type is imposed:"),
        (CANONICAL_DIAGNOSTIC, "origin of obtained type:"),
        (CANONICAL_DIAGNOSTIC, "give 3 as u64;"),
        (CANONICAL_DIAGNOSTIC, "vitte_canonical_keywords"),
        (CANONICAL_DIAGNOSTIC, "vitte_legacy_keyword_replacement"),
        (CANONICAL_DIAGNOSTIC, "vitte_suggestion_is_canonical"),
        (CANONICAL_DIAGNOSTIC, "secondary_spans: [SourceSpan]"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_suggestion_with_secondary_spans"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_multispan_fix_plan"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_with_multispan_suggestion"),
        (CANONICAL_DIAGNOSTIC, "machine_applicable and suggestion.confidence == \"high\" and replacement != \"\""),
        (CANONICAL_DIAGNOSTIC, "give \"\";"),
        (CANONICAL_DIAGNOSTIC, "give 2 as u64;"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_error_type_cascade_candidate"),
        (CANONICAL_DIAGNOSTIC, "candidate.phase == DiagnosticPhase.Typeck"),
        (CANONICAL_DIAGNOSTIC, "space, use, form, pick, proc, let, set, give"),
        (CANONICAL_DIAGNOSTIC, "parser_old_vitte_syntax"),
        (CANONICAL_DIAGNOSTIC, "typeck_give_missing"),
        (CANONICAL_DIAGNOSTIC, "typeck_give_without_value"),
        (CANONICAL_DIAGNOSTIC, "typeck_give_type_mismatch"),
        (CANONICAL_DIAGNOSTIC, "typeck_give_missing_control_path"),
        (CANONICAL_DIAGNOSTIC, "sema_code_after_give"),
        (CANONICAL_DIAGNOSTIC, "sema_set_immutable_binding"),
        (CANONICAL_DIAGNOSTIC, "sema_set_reassignment_impossible"),
        (CANONICAL_DIAGNOSTIC, "sema_mutable_never_modified"),
        (CANONICAL_DIAGNOSTIC, "typeck_let_missing_initialization"),
        (CANONICAL_DIAGNOSTIC, "typeck_let_type_inference_failed"),
        (CANONICAL_DIAGNOSTIC, "typeck_form_unknown_field"),
        (CANONICAL_DIAGNOSTIC, "typeck_form_missing_field"),
        (CANONICAL_DIAGNOSTIC, "typeck_form_duplicate_field"),
        (CANONICAL_DIAGNOSTIC, "typeck_form_field_type_mismatch"),
        (CANONICAL_DIAGNOSTIC, "typeck_form_field_order"),
        (CANONICAL_DIAGNOSTIC, "typeck_form_incomplete"),
        (CANONICAL_DIAGNOSTIC, "typeck_pick_unknown_variant"),
        (CANONICAL_DIAGNOSTIC, "typeck_pick_non_exhaustive"),
        (CANONICAL_DIAGNOSTIC, "typeck_pick_unreachable_variant"),
        (CANONICAL_DIAGNOSTIC, "typeck_pick_payload_mismatch"),
        (CANONICAL_DIAGNOSTIC, "typeck_pick_impossible_pattern"),
        (CANONICAL_DIAGNOSTIC, "typeck_pick_duplicate_branch"),
        (CANONICAL_DIAGNOSTIC, "typeck_proc_unknown"),
        (CANONICAL_DIAGNOSTIC, "typeck_proc_argument_arity"),
        (CANONICAL_DIAGNOSTIC, "typeck_proc_named_argument_unknown"),
        (CANONICAL_DIAGNOSTIC, "typeck_proc_argument_order"),
        (CANONICAL_DIAGNOSTIC, "typeck_proc_argument_type"),
        (CANONICAL_DIAGNOSTIC, "typeck_proc_call_ambiguous"),
        (CANONICAL_DIAGNOSTIC, "typeck_proc_result_ignored"),
        (CANONICAL_DIAGNOSTIC, "typeck_proc_not_callable"),
        (CANONICAL_DIAGNOSTIC, "expected parameters: "),
        (CANONICAL_DIAGNOSTIC, "parameter `\" + parameter_name + \"` is declared here"),
        (CANONICAL_DIAGNOSTIC, "borrowck_borrow_of_moved_value"),
        (CANONICAL_DIAGNOSTIC, "borrowck_write_while_borrowed"),
        (CANONICAL_DIAGNOSTIC, "borrowck_dangling_reference"),
        (TYPECK_DIAGNOSTICS, "`give` in `\" + proc_name + \"` produces"),
        (TYPECK_DIAGNOSTICS, "give a `\" + expected + \"` value or change the proc signature"),
        (TYPECK_DIAGNOSTICS, "`let \" + name + \"` is read before it has a value"),
        (SEMA_DIAGNOSTICS, "use module.{ \" + name + \" }"),
        (PARSER, "a complete extern ABI block"),
        (CANONICAL_DIAGNOSTIC, "set diagnostic.suggestions = diagnostic_suggestion_list_add(diagnostic.suggestions, suggestion);"),
        (TYPECK_TRAITS, "TYPE_SENTINEL_INVALID"),
        (TYPECK_TRAITS, "TypeClass.Invalid"),
        (TYPECK_TRAITS, "type_invalid_sentinel"),
        (TYPECK_TRAITS, "type_is_invalid_sentinel"),
        (TYPECK_TRAITS, "type_is_error_like"),
        (CANONICAL_DIAGNOSTIC, "left.phase != right.phase"),
        (CANONICAL_DIAGNOSTIC, "left.severity != right.severity"),
        (CANONICAL_DIAGNOSTIC, "left.span.end_line != right.span.end_line"),
        (CANONICAL_DIAGNOSTIC, "left.span.length != right.span.length"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_span_sort_offset"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_phase_priority(left.phase)"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_severity_priority(left.severity)"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_direct_root_link"),
        (CANONICAL_DIAGNOSTIC, "candidate.cascade_of == primary.primary_error_key"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_phase_is_blocking_for"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_phase_blocking_reason"),
        (CANONICAL_DIAGNOSTIC, "diagnostic_has_blocking_error_for_phase"),
        (CANONICAL_DIAGNOSTIC, "set primary = candidate;"),
        (CANONICAL_DIAGNOSTIC, "set cascade_count = 0 as u64;"),
        (CANONICAL_DIAGNOSTIC, "__vitte_type_error__"),
        (CANONICAL_DIAGNOSTIC, "pick DiagnosticKind"),
        (CANONICAL_DIAGNOSTIC, "DiagnosticKind.User"),
        (CANONICAL_DIAGNOSTIC, "DiagnosticKind.Configuration"),
        (CANONICAL_DIAGNOSTIC, "DiagnosticKind.Environment"),
        (CANONICAL_DIAGNOSTIC, "DiagnosticKind.Linker"),
        (CANONICAL_DIAGNOSTIC, "DiagnosticKind.InternalCompiler"),
        (CANONICAL_DIAGNOSTIC, "kind: diagnostic_default_kind(effective_phase, effective_code)"),
        (CANONICAL_DIAGNOSTIC, "external_command: \"\""),
        (CANONICAL_DIAGNOSTIC, "external_stderr: \"\""),
        (CANONICAL_DIAGNOSTIC, "internal_operation: \"\""),
        (CANONICAL_DIAGNOSTIC, "reproduction: \"\""),
        (CANONICAL_DIAGNOSTIC, "diagnostic_ice"),
        (CANONICAL_DIAGNOSTIC, "The compiler violated one of its own internal invariants. This is not a user-program error."),
        (CANONICAL_DIAGNOSTIC, "DIAGNOSTIC_USER_COLUMN_BASE"),
        (CANONICAL_DIAGNOSTIC, "DIAGNOSTIC_INTERNAL_OFFSET_UNIT"),
        (CANONICAL_DIAGNOSTIC, "DiagnosticStyle.Short"),
        (CANONICAL_DIAGNOSTIC, "    Lsp"),
        (CANONICAL_DIAGNOSTIC, "    Sarif"),
        (CANONICAL_DIAGNOSTIC, "if config.verbose and diagnostic.external_command != \"\""),
        (CANONICAL_DIAGNOSTIC, "diagnostic_external_summary"),
        (DIAGNOSTIC_LSP, "USER_DIAGNOSTIC_COLUMNS"),
        (DIAGNOSTIC_LSP, "LSP_DIAGNOSTIC_COLUMNS"),
        (DIAGNOSTIC_LSP, "diagnostic_kind_name(diagnostic.kind)"),
        (DIAGNOSTIC_SARIF, "sarif_report"),
        (DIAGNOSTIC_SARIF, "This renderer consumes the canonical Diagnostic object."),
        (BACKEND_DIAGNOSTICS, "vitte_demangle_symbol"),
        (BACKEND_DIAGNOSTICS, "backend_public_code"),
        (BACKEND_DIAGNOSTICS, "BACKEND_E_CODEGEN_FAILED"),
        (BACKEND_DIAGNOSTICS, "give \"GEN0009\";"),
        (BACKEND_DIAGNOSTICS, "BACKEND_E_NATIVE_TOOL_MISSING"),
        (BACKEND_DIAGNOSTICS, "give \"GEN0050\";"),
        (BACKEND_DIAGNOSTICS, "BACKEND_E_UNSUPPORTED_FEATURE"),
        (BACKEND_DIAGNOSTICS, "give \"GEN0064\";"),
        (BACKEND_DIAGNOSTICS, "BACKEND_E_UNSUPPORTED_TARGET"),
        (BACKEND_DIAGNOSTICS, "give \"GEN0065\";"),
        (BACKEND_DIAGNOSTICS, "linker_failure_code"),
        (BACKEND_DIAGNOSTICS, "linker_public_code"),
        (BACKEND_DIAGNOSTICS, "LINK_E_UNDEFINED_SYMBOL"),
        (BACKEND_DIAGNOSTICS, "give \"LNK0058\";"),
        (BACKEND_DIAGNOSTICS, "LINK_E_SYSTEM_LINKER_FAILED"),
        (BACKEND_DIAGNOSTICS, "give \"LNK0054\";"),
        (BACKEND_DIAGNOSTICS, "LINK_E_ARCHITECTURE_INCOMPATIBLE"),
        (BACKEND_DIAGNOSTICS, "give \"LNK0070\";"),
        (BACKEND_DIAGNOSTICS, "LINK_E_PERMISSION_DENIED"),
        (BACKEND_DIAGNOSTICS, "give \"LNK0072\";"),
        (BACKEND_DIAGNOSTICS, "LINK_E_ARCHITECTURE_INCOMPATIBLE"),
        (BACKEND_DIAGNOSTICS, "LINK_E_LIBRARY_NOT_FOUND"),
        (BACKEND_DIAGNOSTICS, "LINK_E_UNSUPPORTED_FORMAT"),
        (BACKEND_DIAGNOSTICS, "LINK_E_DUPLICATE_SYMBOL"),
        (BACKEND_DIAGNOSTICS, "LINK_E_ENTRYPOINT_MISSING"),
        (BACKEND_DIAGNOSTICS, "LINK_E_PERMISSION_DENIED"),
        (BACKEND_DIAGNOSTICS, "diagnostic_with_external_command"),
        (BACKEND_DIAGNOSTICS, "diagnostic_with_external_stderr"),
        (DIAGNOSTIC_CATALOG, "LINK_E_ARCHITECTURE_INCOMPATIBLE"),
        (DIAGNOSTIC_CATALOG, "LINK_E_ENTRYPOINT_MISSING"),
        (DIAGNOSTIC_CATALOG, "LINK_E_PERMISSION_DENIED"),
    )
    for path, fragment in required_fragments:
        text = path.read_text(encoding="utf-8")
        if fragment not in text:
            failures.append(f"{path.relative_to(ROOT)}: missing relational diagnostic fragment {fragment!r}")
    return failures


def main() -> int:
    failures = [
        *check_code_documentation(),
        *check_catalog_codes_present(),
        *check_catalog_codes_unique(),
        *check_diagnostic_object_contract(),
        *check_span_object_contract(),
        *check_relational_diagnostic_contract(),
        *check_direct_output_boundaries(),
        *check_driver_vague_messages(),
        *check_vitte_vocabulary_contract(),
    ]
    if failures:
        for failure in failures:
            print(f"[compiler-diagnostics-contract][error] {failure}", file=sys.stderr)
        return 1
    print("[compiler-diagnostics-contract] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
