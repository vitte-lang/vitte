#!/usr/bin/env python3
"""Enforce source-span provenance contracts across frontend, HIR, MIR, and diagnostics."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

SPAN = ROOT / "src" / "vitte" / "compiler" / "source" / "span.vit"
TOKEN = ROOT / "src" / "vitte" / "compiler" / "frontend" / "lexer" / "token.vit"
SCANNER = ROOT / "src" / "vitte" / "compiler" / "frontend" / "lexer" / "scanner.vit"
FRONTEND = ROOT / "src" / "vitte" / "compiler" / "frontend" / "pipeline.vit"
DRIVER = ROOT / "src" / "vitte" / "compiler" / "driver" / "compiler.vit"
AST_FORMS = (
    ROOT / "src" / "vitte" / "compiler" / "frontend" / "ast" / "expr.vit",
    ROOT / "src" / "vitte" / "compiler" / "frontend" / "ast" / "stmt.vit",
    ROOT / "src" / "vitte" / "compiler" / "frontend" / "ast" / "item.vit",
)
HIR = ROOT / "src" / "vitte" / "compiler" / "middle" / "hir" / "hir.vit"
LOWER_AST = ROOT / "src" / "vitte" / "compiler" / "middle" / "hir" / "lower_ast.vit"
MIR = ROOT / "src" / "vitte" / "compiler" / "middle" / "mir" / "mir.vit"
LOWER_MIR = ROOT / "src" / "vitte" / "compiler" / "middle" / "lower" / "hir_to_mir.vit"
DIAGNOSTIC = ROOT / "src" / "vitte" / "compiler" / "diagnostics" / "diagnostic.vit"


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


def form_fields(path: Path, form_name: str) -> set[str] | None:
    body = extract_form_body(path.read_text(encoding="utf-8"), form_name)
    if body is None:
        return None
    fields: set[str] = set()
    for line in body.splitlines():
        match = re.match(r"\s*([A-Za-z_][A-Za-z0-9_]*)\s*:", line)
        if match:
            fields.add(match.group(1))
    return fields


def require_field(failures: list[str], path: Path, form_name: str, field: str) -> None:
    fields = form_fields(path, form_name)
    rel = path.relative_to(ROOT)
    if fields is None:
        failures.append(f"{rel}: missing form {form_name}")
        return
    if field not in fields:
        failures.append(f"{rel}: {form_name} must carry {field!r}")


def check_span_contract() -> list[str]:
    text = SPAN.read_text(encoding="utf-8")
    failures: list[str] = []
    for required in (
        "pick SpanOriginKind",
        "Source",
        "Synthetic",
        "Missing",
        "origin: SpanOriginKind",
        "synthetic_reason: string",
        "proc synthetic_span",
        "proc span_missing",
        "proc span_has_source_location",
    ):
        if required not in text:
            failures.append(f"{SPAN.relative_to(ROOT)}: missing span provenance contract fragment {required!r}")
    if "start_line: 0" not in text or "start_column: 0" not in text:
        failures.append(f"{SPAN.relative_to(ROOT)}: missing spans must not be represented as arbitrary 1:1 positions")
    return failures


def check_token_contract() -> list[str]:
    failures: list[str] = []
    require_field(failures, TOKEN, "Token", "span")
    token_text = TOKEN.read_text(encoding="utf-8")
    scanner_text = SCANNER.read_text(encoding="utf-8")
    frontend_text = FRONTEND.read_text(encoding="utf-8")
    driver_text = DRIVER.read_text(encoding="utf-8")
    for required in ("proc token_span_for_file", "proc token_with_file_offset", "proc token_with_file"):
        if required not in token_text:
            failures.append(f"{TOKEN.relative_to(ROOT)}: missing {required}")
    if "proc scan_tokens_from_file" not in scanner_text or "tokens_with_source_file" not in scanner_text:
        failures.append(f"{SCANNER.relative_to(ROOT)}: scanner must expose a file-aware token API")
    if "scan_tokens_from_file(source_path" not in frontend_text:
        failures.append(f"{FRONTEND.relative_to(ROOT)}: frontend pipeline must preserve exact token file paths")
    if "scan_tokens_from_file(source_path" not in driver_text:
        failures.append(f"{DRIVER.relative_to(ROOT)}: dump-tokens must preserve exact token file paths")
    return failures


def check_ast_hir_mir_contract() -> list[str]:
    failures: list[str] = []
    for path, form_name in (
        (AST_FORMS[0], "AstExpr"),
        (AST_FORMS[1], "AstStmt"),
        (AST_FORMS[2], "AstGenericParam"),
        (AST_FORMS[2], "AstParam"),
        (AST_FORMS[2], "AstProcSignature"),
        (AST_FORMS[2], "AstNominalMember"),
        (AST_FORMS[2], "AstItem"),
        (AST_FORMS[2], "AstModule"),
    ):
        require_field(failures, path, form_name, "span")

    for form_name in (
        "HirExpr",
        "HirStmt",
        "HirProcParam",
        "HirNominalMember",
        "HirGenericParam",
        "HirItem",
        "HirModule",
        "HirUnit",
    ):
        require_field(failures, HIR, form_name, "span")

    hir_text = HIR.read_text(encoding="utf-8")
    lower_ast_text = LOWER_AST.read_text(encoding="utf-8")
    for required in ("proc hir_synthetic_span", "proc hir_expr_is_synthetic", "proc hir_stmt_is_synthetic", "proc hir_item_is_synthetic"):
        if required not in hir_text:
            failures.append(f"{HIR.relative_to(ROOT)}: missing {required}")
    for required in ("hir_proc_param_with_span", "hir_generic_param_with_span", "hir_module_with_span(frontend.ast_root.span"):
        if required not in lower_ast_text:
            failures.append(f"{LOWER_AST.relative_to(ROOT)}: AST to HIR lowering must preserve {required}")

    for form_name in ("MirStatement", "MirTerminator", "MirBlock", "MirFunction", "MirModule", "MirUnit"):
        require_field(failures, MIR, form_name, "span")

    mir_text = MIR.read_text(encoding="utf-8")
    lower_mir_text = LOWER_MIR.read_text(encoding="utf-8")
    for required in ("proc mir_synthetic_span", "proc mir_statement_is_synthetic", "proc mir_terminator_is_synthetic", "proc mir_block_is_synthetic", "proc mir_function_is_synthetic"):
        if required not in mir_text:
            failures.append(f"{MIR.relative_to(ROOT)}: missing {required}")
    for required in ("mir_terminator_branch_at", "mir_terminator_goto_at(stmt.span", "mir_terminator_return_at(stmt.span", "mir_function_with_span", "mir_module_with_span", "mir_unit_with_span"):
        if required not in lower_mir_text:
            failures.append(f"{LOWER_MIR.relative_to(ROOT)}: HIR to MIR lowering must preserve {required}")
    return failures


def check_diagnostic_missing_span_contract() -> list[str]:
    text = DIAGNOSTIC.read_text(encoding="utf-8")
    failures: list[str] = []
    for required in (
        "proc diagnostic_phase_requires_user_span",
        "proc diagnostic_missing_user_span_code",
        "ICE0001",
        "requires an exact source span",
        "diagnostic producer emitted user-facing diagnostic without an exact source span",
    ):
        if required not in text:
            failures.append(f"{DIAGNOSTIC.relative_to(ROOT)}: missing user diagnostic span guard {required!r}")
    return failures


def main() -> int:
    failures = [
        *check_span_contract(),
        *check_token_contract(),
        *check_ast_hir_mir_contract(),
        *check_diagnostic_missing_span_contract(),
    ]
    if failures:
        for failure in failures:
            print(f"[span-provenance-contract][error] {failure}", file=sys.stderr)
        return 1
    print("[span-provenance-contract] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
