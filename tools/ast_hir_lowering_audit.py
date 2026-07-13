#!/usr/bin/env python3
from __future__ import annotations

import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
AST_DIR = ROOT / "src" / "vitte" / "compiler" / "frontend" / "ast"
HIR_FILE = ROOT / "src" / "vitte" / "compiler" / "middle" / "hir" / "hir.vit"
LOWERING_FILE = ROOT / "src" / "vitte" / "compiler" / "middle" / "hir" / "lower_ast.vit"
REPORT = ROOT / "target" / "reports" / "ast_hir_lowering" / "audit.json"


@dataclass(frozen=True)
class DirectSurface:
    ast_file: Path
    ast_pick: str
    hir_pick: str
    lower_proc: str


DIRECT_SURFACES = (
    DirectSurface(AST_DIR / "expr.vit", "AstExprKind", "HirExprKind", "lower_expr_kind"),
    DirectSurface(AST_DIR / "stmt.vit", "AstStmtKind", "HirStmtKind", "lower_stmt_kind"),
    DirectSurface(AST_DIR / "item.vit", "AstItemKind", "HirItemKind", "lower_item_kind"),
    DirectSurface(AST_DIR / "item.vit", "AstNominalMemberKind", "HirNominalMemberKind", "lower_member_kind"),
)

NORMALIZED_SURFACES = (
    (AST_DIR / "pattern.vit", "AstPatternKind", "ast_pattern_kind_name"),
    (AST_DIR / "type_expr.vit", "AstTypeKind", "ast_type_kind_name"),
)

REQUIRED_NORMALIZATION_CALLS = (
    "proc lower_pattern_text(pattern: AstPattern) -> string",
    "ast_pattern_kind_name(pattern.kind)",
    "lower_pattern_binding_name(param.pattern)",
    "lower_pattern_binding_name(stmt.pattern)",
    "proc lower_type_text(type_expr: AstTypeExpr) -> string",
    "ast_type_kind_name(type_expr.kind)",
    "lower_type_text(expr.type_annotation)",
    "lower_type_text(param.annotation)",
    "lower_type_text(stmt.annotation)",
    "lower_type_text(item.annotation)",
    "lower_type_text(item.target_type)",
)

REQUIRED_RECURSIVE_CALLS = (
    "lower_expr_children(expr.children)",
    "lower_stmt_list(stmt.then_stmts, scope_depth + 1)",
    "lower_stmt_list(stmt.else_stmts, scope_depth + 1)",
    "lower_stmt_list(stmt.catch_stmts, scope_depth + 1)",
    "lower_nominal_members(item.members)",
    "lower_member_params(member.signature.params)",
    "lower_stmt_list(member.body_stmts, 1)",
    "lower_item_stmts(item)",
)

REQUIRED_ITEM_METADATA = (
    ("hir.vit", "generic_params: [string]"),
    ("lower_ast.vit", "proc item_generic_param_names(item: AstItem) -> [string]"),
    ("lower_ast.vit", "let generic_params: [string] = item_generic_param_names(item);"),
    ("lower_ast.vit", "len(generic_params),\n    generic_params,"),
)


def read(path: Path) -> str:
    if not path.is_file():
        print(f"[ast-hir-lowering][error] missing file: {path.relative_to(ROOT)}", file=sys.stderr)
        raise SystemExit(1)
    return path.read_text(encoding="utf-8", errors="replace")


def braced_body(text: str, header_pattern: str) -> str:
    match = re.search(header_pattern, text)
    if match is None:
        raise ValueError(f"missing declaration matching {header_pattern!r}")
    start = text.find("{", match.start())
    if start < 0:
        raise ValueError(f"missing opening brace for {header_pattern!r}")
    depth = 0
    for index in range(start, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1 : index]
    raise ValueError(f"missing closing brace for {header_pattern!r}")


def pick_variants(text: str, pick_name: str) -> list[str]:
    body = braced_body(text, rf"\bpick\s+{re.escape(pick_name)}\s*\{{")
    return re.findall(r"(?m)^\s*([A-Z][A-Za-z0-9_]*)\s*,?\s*$", body)


def proc_body(text: str, proc_name: str) -> str:
    return braced_body(text, rf"\bproc\s+{re.escape(proc_name)}\s*\(")


def direct_mapping(body: str, ast_pick: str, hir_pick: str) -> list[tuple[str, str]]:
    pattern = re.compile(
        rf"kind\s*==\s*{re.escape(ast_pick)}\.([A-Za-z0-9_]+)\s*"
        rf"\{{\s*give\s+{re.escape(hir_pick)}\.([A-Za-z0-9_]+)\s*;?\s*\}}",
        re.MULTILINE,
    )
    return pattern.findall(body)


def audit_direct_surface(surface: DirectSurface, hir_text: str, lowering_text: str) -> tuple[dict[str, object], list[str]]:
    ast_variants = pick_variants(read(surface.ast_file), surface.ast_pick)
    hir_variants = pick_variants(hir_text, surface.hir_pick)
    mappings = direct_mapping(proc_body(lowering_text, surface.lower_proc), surface.ast_pick, surface.hir_pick)
    mapped_sources = [source for source, _ in mappings]
    duplicate_sources = sorted({name for name in mapped_sources if mapped_sources.count(name) > 1})
    mapping_by_source = dict(mappings)
    missing = sorted(set(ast_variants) - set(mapped_sources))
    unexpected = sorted(set(mapped_sources) - set(ast_variants))
    missing_hir_targets = sorted({target for _, target in mappings} - set(hir_variants))
    renamed = sorted(
        f"{source}->{target}"
        for source, target in mappings
        if source != target
    )
    failures: list[str] = []
    if missing:
        failures.append(f"{surface.ast_pick}: missing lowering for {', '.join(missing)}")
    if unexpected:
        failures.append(f"{surface.ast_pick}: mappings for unknown AST variants {', '.join(unexpected)}")
    if duplicate_sources:
        failures.append(f"{surface.ast_pick}: duplicate mappings for {', '.join(duplicate_sources)}")
    if missing_hir_targets:
        failures.append(f"{surface.hir_pick}: undeclared targets {', '.join(missing_hir_targets)}")
    if renamed:
        failures.append(f"{surface.ast_pick}: non-canonical mappings {', '.join(renamed)}")
    status = "pass" if not failures else "fail"
    result: dict[str, object] = {
        "ast_pick": surface.ast_pick,
        "ast_variants": ast_variants,
        "hir_pick": surface.hir_pick,
        "lower_proc": surface.lower_proc,
        "mapped": mapping_by_source,
        "mapped_count": len(mapping_by_source),
        "status": status,
        "variant_count": len(ast_variants),
    }
    return result, failures


def audit_normalized_surface(path: Path, pick_name: str, naming_proc: str) -> tuple[dict[str, object], list[str]]:
    text = read(path)
    variants = pick_variants(text, pick_name)
    body = proc_body(text, naming_proc)
    named_variants = sorted(set(re.findall(rf"{re.escape(pick_name)}\.([A-Za-z0-9_]+)", body)))
    missing = sorted(set(variants) - set(named_variants))
    unexpected = sorted(set(named_variants) - set(variants))
    failures: list[str] = []
    if missing:
        failures.append(f"{pick_name}: missing canonical text normalization for {', '.join(missing)}")
    if unexpected:
        failures.append(f"{pick_name}: normalization references unknown variants {', '.join(unexpected)}")
    result: dict[str, object] = {
        "ast_pick": pick_name,
        "naming_proc": naming_proc,
        "normalized_count": len(named_variants),
        "status": "pass" if not failures else "fail",
        "strategy": "canonical_text",
        "variant_count": len(variants),
    }
    return result, failures


def main() -> int:
    hir_text = read(HIR_FILE)
    lowering_text = read(LOWERING_FILE)
    direct_results: list[dict[str, object]] = []
    normalized_results: list[dict[str, object]] = []
    failures: list[str] = []

    try:
        for surface in DIRECT_SURFACES:
            result, surface_failures = audit_direct_surface(surface, hir_text, lowering_text)
            direct_results.append(result)
            failures.extend(surface_failures)
        for path, pick_name, naming_proc in NORMALIZED_SURFACES:
            result, surface_failures = audit_normalized_surface(path, pick_name, naming_proc)
            normalized_results.append(result)
            failures.extend(surface_failures)
    except ValueError as error:
        failures.append(str(error))

    normalization_calls = {
        needle: "present" if needle in lowering_text else "missing"
        for needle in REQUIRED_NORMALIZATION_CALLS
    }
    for needle, status in normalization_calls.items():
        if status != "present":
            failures.append(f"lower_ast.vit: missing normalization call `{needle}`")

    recursive_calls = {
        needle: "present" if needle in lowering_text else "missing"
        for needle in REQUIRED_RECURSIVE_CALLS
    }
    for needle, status in recursive_calls.items():
        if status != "present":
            failures.append(f"lower_ast.vit: missing recursive lowering call `{needle}`")

    item_metadata = {}
    for owner, needle in REQUIRED_ITEM_METADATA:
        text = hir_text if owner == "hir.vit" else lowering_text
        status = "present" if needle in text else "missing"
        item_metadata[f"{owner}:{needle}"] = status
        if status != "present":
            failures.append(f"{owner}: missing item metadata contract `{needle}`")

    payload = {
        "schema": "vitte.compiler.ast_hir_lowering_audit",
        "schema_version": "1.0.0",
        "status": "fail" if failures else "pass",
        "direct_surfaces": direct_results,
        "normalized_surfaces": normalized_results,
        "normalization_calls": normalization_calls,
        "recursive_calls": recursive_calls,
        "item_metadata": item_metadata,
        "failures": failures,
    }
    REPORT.parent.mkdir(parents=True, exist_ok=True)
    REPORT.write_text(json.dumps(payload, ensure_ascii=True, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    total_direct = sum(int(item["variant_count"]) for item in direct_results)
    total_normalized = sum(int(item["variant_count"]) for item in normalized_results)
    print(
        "[ast-hir-lowering] "
        f"status={payload['status']} direct={total_direct} normalized={total_normalized} "
        f"report={REPORT.relative_to(ROOT)}"
    )
    for failure in failures:
        print(f"[ast-hir-lowering][error] {failure}", file=sys.stderr)
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
