#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[1]


@dataclass(frozen=True)
class Requirement:
    label: str
    path: str
    needles: tuple[str, ...]


REQUIREMENTS: tuple[Requirement, ...] = (
    Requirement(
        "src module grammar keeps bootstrap module syntax",
        "src/vitte/grammar/ebnf/modules.ebnf",
        ("module_declaration", '"space"', "module_path", 'path_separator', '"::"', '"/"'),
    ),
    Requirement(
        "src import grammar keeps path alias group glob imports",
        "src/vitte/grammar/ebnf/imports.ebnf",
        ("import_path", "import_alias", "import_group", "import_glob", '"use"', '"as"', '"*"'),
    ),
    Requirement(
        "src export grammar keeps explicit, group, alias, and star exports",
        "src/vitte/grammar/ebnf/exports.ebnf",
        ("export_declaration", "export_all", "export_group", "export_alias", '"export"', '"*"'),
    ),
    Requirement(
        "src compiler exposes canonical backend IR",
        "src/vitte/compiler/backend/ir/README.md",
        ("vitte/compiler/backend/ir::IrUnit", "only IR contract accepted", "Validation boundary", "verify_unit"),
    ),
    Requirement(
        "src C backend consumes backend IR",
        "src/vitte/compiler/backend/c/README.md",
        ("Function ABI metadata is preserved from HIR to `backend/ir::IrFunction`", "canonical MIR/IR to C model lowering"),
    ),
    Requirement(
        "bootstrap parser accepts aligned module/import/export surface",
        "bootstrap/src/parser/parser.c",
        (
            "vitte_parser_parse_module_header",
            "vitte_parser_parse_import_item",
            "vitte_parser_parse_export_top_level",
            "VITTE_AST_IMPORT_GLOB",
            "VITTE_PARSER_E_REEXPORT",
        ),
    ),
    Requirement(
        "bootstrap AST stores aligned import/export metadata",
        "bootstrap/src/ast/ast.h",
        (
            "VITTE_AST_NODE_IMPORT_DECL",
            "VITTE_AST_NODE_EXPORT_DECL",
            "VITTE_AST_IMPORT_MODULE",
            "VITTE_AST_IMPORT_SYMBOL",
            "VITTE_AST_IMPORT_GLOB",
            "export_all",
        ),
    ),
    Requirement(
        "bootstrap driver resolves imports before backend lowering",
        "bootstrap/src/driver/driver.c",
        (
            "vitte_module_resolve_imports",
            "vitte_driver_flatten_imported_modules",
            "vitte_driver_lookup_lowered_visible_name",
            "VITTE_DRIVER_E_IMPORT_USE_CONTRACT",
            "VITTE_DRIVER_STAGE_BACKEND",
            "VITTE_DRIVER_STAGE_CODEGEN_C",
        ),
    ),
    Requirement(
        "bootstrap pipeline lowers AST to HIR then IR before C17",
        "bootstrap/src/driver/driver.c",
        (
            "vitte_hir_lower_ast",
            "vitte_hir_validate",
            "vitte_ir_lower_hir",
            "vitte_ir_validate",
            "VITTE_CODEGEN_INPUT_IR",
        ),
    ),
    Requirement(
        "bootstrap codegen is IR-first C17",
        "bootstrap/src/codegen/codegen.c",
        ("vitte_codegen_validate_ir", "vitte_c17_backend_emit_ir_to_file", "vitte_c17_backend_emit_ir_to_buffer"),
    ),
    Requirement(
        "bootstrap C17 backend has no AST-only entrypoint",
        "bootstrap/src/backend/c17/module.h",
        ("vitte_c17_module_init_ir",),
    ),
    Requirement(
        "bootstrap C17 backend emits from IR only",
        "bootstrap/src/backend/c17/module.c",
        ("vitte_c17_module_emit_ir", "vitte_c17_emit_ir_function_prototype", "vitte_c17_emit_ir_function_body"),
    ),
    Requirement(
        "bootstrap sema carries structured import/export diagnostics",
        "bootstrap/src/sema/sema.c",
        (
            "VITTE_SEMA_E_IMPORT",
            "VITTE_SEMA_E_IMPORT_PRIVATE",
            "VITTE_SEMA_E_IMPORT_CONFLICT",
            "VITTE_SEMA_E_EXPORT",
            "VITTE_SEMA_E_EXPORT_CONFLICT",
            "VITTE_SEMA_E_REEXPORT",
            "VITTE_SEMA_E_CALL",
            "VITTE_SEMA_E_OPERATOR",
        ),
    ),
    Requirement(
        "bootstrap IR validates backend-facing structure and types",
        "bootstrap/src/ir/ir.c",
        (
            "vitte_ir_validate",
            "vitte_ir_validate_call_signature",
            "VITTE_IR_E_FUNCTION",
            "VITTE_IR_E_PARAMETER",
            "VITTE_IR_E_CALL",
            "VITTE_IR_E_RETURN",
            "VITTE_IR_E_TERMINATOR",
        ),
    ),
    Requirement(
        "bootstrap coverage docs name the aligned pipeline",
        "bootstrap/docs/language_coverage.md",
        ("AST -> HIR -> IR -> C17", "Bootstrap Alignment Profile", "re-exports are explicitly unsupported"),
    ),
)

FORBIDDEN: tuple[Requirement, ...] = (
    Requirement(
        "bootstrap C17 backend must not expose an AST-only module initializer",
        "bootstrap/src/backend/c17/module.h",
        ("vitte_c17_module_init_ast", "VITTE_C17_MODULE_INPUT_AST"),
    ),
    Requirement(
        "bootstrap C17 backend must not branch on AST input",
        "bootstrap/src/backend/c17/module.c",
        ("vitte_c17_module_init_ast", "VITTE_C17_MODULE_INPUT_AST"),
    ),
)

FIXTURES: tuple[str, ...] = (
    "bootstrap/tests/grammar_alignment/let_mut_ok.vit",
    "bootstrap/tests/import_export_coverage/multi_decl_main.vit",
    "bootstrap/tests/import_export_coverage/module_alias_rich_main.vit",
    "bootstrap/tests/import_export_coverage/coverage_chain_main.vit",
    "bootstrap/tests/import_export_coverage/coverage_star_main.vit",
    "bootstrap/tests/import_export_coverage/module_path_unsupported.vit",
    "bootstrap/tests/import_collision/glob_conflict_main.vit",
    "bootstrap/tests/export_forms/reexport_group_unsupported.vit",
    "bootstrap/tests/export_forms/reexport_symbol_unsupported.vit",
    "bootstrap/tests/sema_calls/import_param_mismatch.vit",
)


def read_text(rel: str) -> str:
    path = ROOT / rel
    if not path.is_file():
        raise AssertionError(f"missing file: {rel}")
    return path.read_text(encoding="utf-8")


def check_required(requirement: Requirement) -> list[str]:
    text = read_text(requirement.path)
    return [needle for needle in requirement.needles if needle not in text]


def check_forbidden(requirement: Requirement) -> list[str]:
    text = read_text(requirement.path)
    return [needle for needle in requirement.needles if needle in text]


def main() -> int:
    failures: list[str] = []

    for requirement in REQUIREMENTS:
        missing = check_required(requirement)
        if missing:
            failures.append(
                f"{requirement.label}: {requirement.path} missing {', '.join(missing)}"
            )

    for requirement in FORBIDDEN:
        present = check_forbidden(requirement)
        if present:
            failures.append(
                f"{requirement.label}: {requirement.path} still contains {', '.join(present)}"
            )

    for fixture in FIXTURES:
        if not (ROOT / fixture).is_file():
            failures.append(f"alignment fixture missing: {fixture}")

    if failures:
        for failure in failures:
            print(f"[bootstrap-src-alignment][error] {failure}", file=sys.stderr)
        return 1

    print("[bootstrap-src-alignment] ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
