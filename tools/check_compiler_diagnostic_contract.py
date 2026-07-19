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
CANONICAL_DIAGNOSTIC = COMPILER_ROOT / "diagnostics" / "diagnostic.vit"
INFRA_DIAGNOSTIC = COMPILER_ROOT / "infrastructure" / "diagnostics" / "diagnostic.vit"
SEMA_DIAGNOSTICS = COMPILER_ROOT / "analysis" / "sema" / "errors.vit"
SEMA_RESOLVER = COMPILER_ROOT / "analysis" / "sema" / "resolver.vit"
PARSER = COMPILER_ROOT / "frontend" / "parse" / "parser.vit"
TYPECK_DIAGNOSTICS = COMPILER_ROOT / "analysis" / "typeck" / "errors.vit"
TYPECK_TRAITS = COMPILER_ROOT / "analysis" / "typeck" / "traits.vit"
BORROWCK_DIAGNOSTICS = COMPILER_ROOT / "analysis" / "borrowck" / "errors.vit"
BORROWCK_LIFETIMES = COMPILER_ROOT / "analysis" / "borrowck" / "lifetimes.vit"
TYPE_UNIFY = COMPILER_ROOT / "analysis" / "typeck" / "unify.vit"
DIAGNOSTIC_CATALOG = COMPILER_ROOT / "diagnostics" / "catalog.vit"
MIDDLE_TYPECHECK_DIAGNOSTICS = COMPILER_ROOT / "middle" / "typecheck" / "diagnostics.vit"

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
REQUIRED_INFRA_INITIALIZERS = (
    "title: message",
    "secondary_spans: []",
    "primary_span: span",
    "span: span",
    "file_id: 0",
    "internal_cause: diagnostic_default_explanation(phase)",
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


def check_code_documentation() -> list[str]:
    payload = json.loads(CODES.read_text(encoding="utf-8"))
    failures: list[str] = []
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
    for path, required_initializers in (
        (CANONICAL_DIAGNOSTIC, REQUIRED_CANONICAL_INITIALIZERS),
        (INFRA_DIAGNOSTIC, REQUIRED_INFRA_INITIALIZERS),
    ):
        text = path.read_text(encoding="utf-8")
        rel = path.relative_to(ROOT)
        fields = form_fields(text, "Diagnostic")
        if fields is None:
            failures.append(f"{rel}: missing form Diagnostic")
            continue
        for field in REQUIRED_DIAGNOSTIC_FIELDS:
            if field not in fields:
                failures.append(f"{rel}: Diagnostic is missing required field {field!r}")
        for initializer in required_initializers:
            if initializer not in text:
                failures.append(f"{rel}: diagnostic_create must initialize {initializer!r}")
        if "set diagnostic.secondary_spans = diagnostic.secondary_spans + [label.span]" not in text:
            failures.append(f"{rel}: secondary labels must preserve their spans in secondary_spans")
        if "set diagnostic.primary_span = label.span" not in text or "set diagnostic.span = label.span" not in text:
            failures.append(f"{rel}: primary labels must synchronize span and primary_span")
        if "proc diagnostic_with_file_id" not in text:
            failures.append(f"{rel}: missing diagnostic_with_file_id helper")
        if "proc diagnostic_with_internal_cause" not in text:
            failures.append(f"{rel}: missing diagnostic_with_internal_cause helper")
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
        MIDDLE_TYPECHECK_DIAGNOSTICS,
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
        (SEMA_DIAGNOSTICS, "skipped \" + category + \" `\" + nearby + \"` because `\" + expected_context + \"` context does not accept that symbol kind"),
        (SEMA_DIAGNOSTICS, "false, \"medium\", \"the symbol may be exported by another module, but the exact module path must be confirmed\""),
        (SEMA_DIAGNOSTICS, "write a canonical `use` path that names an existing module and exported symbol"),
        (SEMA_RESOLVER, "sema_ambiguous_symbol"),
        (SEMA_RESOLVER, "sema_out_of_scope_symbol"),
        (SEMA_RESOLVER, "sema_inaccessible_module"),
        (PARSER, "parser_rule_for_code"),
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
        (BORROWCK_DIAGNOSTICS, "borrow is still active at the conflict"),
        (BORROWCK_DIAGNOSTICS, "conflict appears here"),
        (BORROWCK_DIAGNOSTICS, "chronology step 1:"),
        (BORROWCK_DIAGNOSTICS, "chronology step 2:"),
        (BORROWCK_DIAGNOSTICS, "chronology step 3:"),
        (BORROWCK_DIAGNOSTICS, "chronology step 4:"),
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
    )
    for path, fragment in required_fragments:
        text = path.read_text(encoding="utf-8")
        if fragment not in text:
            failures.append(f"{path.relative_to(ROOT)}: missing relational diagnostic fragment {fragment!r}")
    return failures


def main() -> int:
    failures = [
        *check_code_documentation(),
        *check_diagnostic_object_contract(),
        *check_relational_diagnostic_contract(),
        *check_direct_output_boundaries(),
        *check_driver_vague_messages(),
    ]
    if failures:
        for failure in failures:
            print(f"[compiler-diagnostics-contract][error] {failure}", file=sys.stderr)
        return 1
    print("[compiler-diagnostics-contract] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
