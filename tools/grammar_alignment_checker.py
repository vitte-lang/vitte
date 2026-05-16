#!/usr/bin/env python3
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import argparse
import json
import sys


ROOT = Path(__file__).resolve().parent.parent
PARSER = ROOT / "src" / "vitte" / "compiler" / "frontend" / "parse" / "parser.vit"
IR_AST = ROOT / "src" / "vitte" / "compiler" / "ir" / "ast.vit"
FRONTEND_AST = ROOT / "src" / "vitte" / "compiler" / "frontend" / "ast"
VALID_FIXTURES = ROOT / "tests" / "frontend_syntax" / "valid"


@dataclass(frozen=True)
class Rule:
    category: str
    name: str
    parser_terms: tuple[str, ...]
    ast_terms: tuple[str, ...]
    fixture_terms: tuple[str, ...] = ()


DECL_RULES = [
    Rule("declaration", "inner_attribute", ("parse_attrs", "#!"), ("AstAttribute",), ("#!",)),
    Rule("declaration", "space_decl", ('tok.lexeme == "space"',), ("Space",), ("space ",)),
    Rule("declaration", "use_decl", ('tok.lexeme == "use"', "parse_module_path"), ("Use",), ("use ",)),
    Rule("declaration", "export_decl", ('tok.lexeme == "export"',), ("Export",), ("export ",)),
    Rule("declaration", "const_decl", ('tok.lexeme == "const"', "parse_const_decl"), ("Const",), ("const ",)),
    Rule("declaration", "static_decl", ('tok.lexeme == "static"', "parse_const_decl"), ("Static",), ("static ",)),
    Rule("declaration", "global_decl", ('tok.lexeme == "global"', "parse_const_decl"), ("Global",), ("global ",)),
    Rule("declaration", "type_alias_decl", ('tok.lexeme == "type"', "parse_type_decl"), ("TypeAlias",), ("type ",)),
    Rule("declaration", "opaque_type_decl", ('tok.lexeme == "opaque"',), ("OpaqueType",), ("opaque type",)),
    Rule("declaration", "extern_type_decl", ('tok.lexeme == "extern"', 'at_text(walk, "type")'), ("ExternType",), ("extern type",)),
    Rule("declaration", "extern_block", ('tok.lexeme == "extern"', "skip_balanced"), ("ExternBlock",), ("extern C",)),
    Rule("declaration", "form_decl", ('tok.lexeme == "form"', "parse_container_decl"), ("Form",), ("form ",)),
    Rule("declaration", "class_decl", ('tok.lexeme == "class"', "parse_container_decl"), ("Class",), ("class ",)),
    Rule("declaration", "union_decl", ('tok.lexeme == "union"', "parse_container_decl"), ("Union",), ("union ",)),
    Rule("declaration", "bits_decl", ('tok.lexeme == "bits"', "parse_container_decl"), ("Bits",), ("bits ",)),
    Rule("declaration", "pick_decl", ('tok.lexeme == "pick"', "parse_container_decl"), ("Pick",), ("pick ",)),
    Rule("declaration", "flags_decl", ('tok.lexeme == "flags"', "parse_container_decl"), ("Flags",), ("flags ",)),
    Rule("declaration", "trait_decl", ('tok.lexeme == "trait"', "parse_container_decl"), ("Trait",), ("trait ",)),
    Rule("declaration", "impl_decl", ('tok.lexeme == "impl"', "parse_container_decl"), ("Impl",), ("impl ",)),
    Rule("declaration", "proc_decl", ("parse_proc_decl", 'expect_text(current, "proc"'), ("Proc",), (" proc ",)),
    Rule("declaration", "intrinsic_decl", ('tok.lexeme == "intrinsic"',), ("Intrinsic",), ("intrinsic ",)),
    Rule("declaration", "compiler_decl", ('tok.lexeme == "compiler"', "parse_meta_decl"), ("Compiler",), ("compiler ",)),
    Rule("declaration", "query_decl", ('tok.lexeme == "query"',), ("Query",), ("query ",)),
    Rule("declaration", "pass_decl", ('tok.lexeme == "pass"', "parse_meta_decl"), ("Pass",), ("pass ",)),
    Rule("declaration", "backend_decl", ('tok.lexeme == "backend"', "parse_meta_decl"), ("Backend",), ("backend ",)),
    Rule("declaration", "diagnostic_decl", ('tok.lexeme == "diagnostic"', "parse_meta_decl"), ("Diagnostic",), ("diagnostic ",)),
    Rule("declaration", "macro_decl", ('tok.lexeme == "macro"', "parse_meta_decl"), ("Macro",), ("macro ",)),
    Rule("declaration", "comptime_decl", ('tok.lexeme == "comptime"',), ("Comptime",), ("comptime ",)),
    Rule("declaration", "static_assert_decl", ('tok.lexeme == "static_assert"',), ("StaticAssert",), ("static_assert",)),
    Rule("declaration", "test_decl", ('tok.lexeme == "test"',), ("Test",), ("test ")),
    Rule("declaration", "bench_decl", ('tok.lexeme == "bench"',), ("Bench",), ("bench ")),
    Rule("declaration", "entry_decl", ('tok.lexeme == "entry"', "parse_meta_decl"), ("Entry",), ("entry ",)),
]


STMT_RULES = [
    Rule("statement", "local_const_stmt", ('at_text(current, "const")', "PSTMT001"), ("LocalConst",), ("const local_id",)),
    Rule("statement", "let_stmt", ('at_text(current, "let")', "PSTMT004"), ("Let",), ("let ",)),
    Rule("statement", "set_stmt", ('at_text(current, "set")', "PSTMT006"), ("Set",), ("set ",)),
    Rule("statement", "give_stmt", ('at_text(current, "give")', "PSTMT008"), ("Give",), ("give ",)),
    Rule("statement", "try_stmt", ('at_text(current, "try")', "PSTMT009"), ("Try",), ("try ",)),
    Rule("statement", "defer_stmt", ('at_text(current, "defer")',), ("Defer",), ("defer ",)),
    Rule("statement", "asm_stmt", ('at_text(current, "asm")',), ("Asm",), ("asm(",)),
    Rule("statement", "unsafe_stmt", ('at_text(current, "unsafe")',), ("Unsafe",), ("unsafe ",)),
    Rule("statement", "emit_stmt", ('at_text(current, "emit")',), ("Emit",), ("emit ",)),
    Rule("statement", "assert_stmt", ('at_text(current, "assert")',), ("Assert",), ("assert ",)),
    Rule("statement", "panic_stmt", ('at_text(current, "panic")',), ("Panic",), ("panic ",)),
    Rule("statement", "unreachable_stmt", ('at_text(current, "unreachable")',), ("Unreachable",), ("unreachable",)),
    Rule("statement", "if_stmt", ('at_text(current, "if")', "parse_if_expr"), ("If",), ("if ",)),
    Rule("statement", "while_stmt", ('at_text(current, "while")',), ("While",), ("while ",)),
    Rule("statement", "loop_stmt", ('at_text(current, "loop")',), ("Loop",), ("loop ",)),
    Rule("statement", "for_stmt", ('at_text(current, "for")',), ("For",), ("for ",)),
    Rule("statement", "break_stmt", ('at_text(current, "break")',), ("Break",), ("break",)),
    Rule("statement", "continue_stmt", ('at_text(current, "continue")',), ("Continue",), ("continue",)),
    Rule("statement", "select_stmt", ('at_text(current, "select")',), ("Select",), ("select ",)),
    Rule("statement", "match_stmt", ('at_text(current, "match")', "parse_match_expr"), ("Match",), ("match ",)),
    Rule("statement", "when_match_stmt", ('at_text(current, "when")', 'expect_text(walk, "is"'), ("WhenMatch",), ("when ",)),
    Rule("statement", "with_stmt", ('at_text(current, "with")',), ("With",), ("with ",)),
    Rule("statement", "critical_stmt", ('at_text(current, "critical")',), ("Critical",), ("critical ",)),
    Rule("statement", "expr_stmt", ("PSTMT018", "parse_expr(current)"), ("Expr",), ()),
]


EXPR_RULES = [
    Rule("expression", "assign_expr", ("+=", ">>=", "parse_expr(walk)"), ("Assign",), ("set total +=",)),
    Rule("expression", "ternary_expr", ('at_text(walk, "?")', "PTERN001"), ("Ternary",), ()),
    Rule("expression", "coalesce_expr", ('at_text(walk, "??")',), ("Binary",), ("??",)),
    Rule("expression", "range_expr", ('at_text(walk, "..")', 'at_text(walk, "..=")'), ("Range",), ()),
    Rule("expression", "binary_expr", ('at_text(walk, "+")', 'at_text(walk, "and")'), ("Binary",), (" + ",)),
    Rule("expression", "cast_expr", ('op == "as"', "parse_type_expr(walk)"), ("Cast",), (" as ")),
    Rule("expression", "is_expr", ('op == "is"', "parse_pattern(walk)"), ("Is",), (" is ")),
    Rule("expression", "unary_expr", ("parse_unary_expr", 'at_text(state, "not")'), ("Unary",), ("*&total",)),
    Rule("expression", "call_expr", ("PCALL001", "parse_arg_list"), ("Call",), ("callback(")),
    Rule("expression", "member_expr", ("PMEMBER001", 'at_text(current, ".")'), ("Member",), (".len",)),
    Rule("expression", "index_expr", ("PINDEX001",), ("Index",), ("table[")),
    Rule("expression", "struct_literal", ("path_starts_struct_literal", "PSTRUCT001"), ("StructLiteral",), ("Pair {")),
    Rule("expression", "list_literal", ("PPRIMARY002",), ("ListLiteral",), ("[1, 2, 3]")),
    Rule("expression", "set_literal", ('at_text(current, "set")', "PPRIMARY003"), ("SetLiteral",), ("set {")),
    Rule("expression", "map_literal", ('at_text(current, "map")', "PPRIMARY003"), ("MapLiteral",), ("map {")),
    Rule("expression", "resource_literal", ('at_text(current, "resource")', "PPRIMARY003"), ("ResourceLiteral",), ("resource {")),
    Rule("expression", "bytes_literal", ('at_text(current, "b")',), ("BytesLiteral",), ('b"abc"',)),
    Rule("expression", "if_expr", ('at_text(current, "if")', "parse_if_expr"), ("IfExpr",), ("if ",)),
    Rule("expression", "match_expr", ('at_text(current, "match")', "parse_match_expr"), ("MatchExpr",), ("match ",)),
    Rule("expression", "lambda_expr", ('at_text(current, "|")', "PLAMBDA001"), ("Lambda",), ("|value|",)),
    Rule("expression", "proc_expr", ('at_text(current, "proc")', "PPRIMARY004"), ("ProcExpr",), ("proc(value: int)")),
    Rule("expression", "unsafe_expr", ('at_text(current, "unsafe")',), ("UnsafeExpr",), ("unsafe ")),
    Rule("expression", "block_expr", ('at_text(current, "{")', "parse_block"), ("BlockExpr",), ("{")),
    Rule("expression", "builtin_expr", ("sizeof", "alignof", "typeof", "nameof"), ("Builtin",), ("sizeof(",)),
]


TYPE_RULES = [
    Rule("type", "named_type", ("parse_path(current)",), ("Named",), ("Config[int]",)),
    Rule("type", "primitive_type", ("parse_type_primary",), ("Primitive",), ("int",)),
    Rule("type", "qualified_type", ('at_text(current, "const")', 'at_text(current, "owned")'), ("Qualified",), ()),
    Rule("type", "reference_type", ('at_text(current, "&")',), ("Reference",), ("&mut",)),
    Rule("type", "pointer_type", ('at_text(current, "*")',), ("Pointer",), ("*&total",)),
    Rule("type", "optional_type", ('at_text(current, "?")',), ("Optional",), ("?&mut",)),
    Rule("type", "array_or_slice_type", ('at_text(current, "[")', "PTYPE001"), ("FixedArray", "Slice"), ("[T]",)),
    Rule("type", "tuple_type", ('at_text(current, "(")', "PTYPE002"), ("Tuple",), ("(T, T)")),
    Rule("type", "proc_type", ('at_text(current, "proc")', "PTYPE003"), ("Proc",), ("proc(value")),
    Rule("type", "dyn_type", ('at_text(current, "dyn")',), ("Dyn",), ("dyn ")),
    Rule("type", "impl_trait_type", ('at_text(current, "impl")',), ("ImplTrait",), ("impl ")),
    Rule("type", "generic_type", ("PTYPE005",), ("Generic",), ("Option[T]")),
    Rule("type", "union_type", ('while at_text(current, "|")',), ("Union",), ("T | int")),
    Rule("type", "lifetime_type", ('at_text(current, "\'")', "PTYPE006"), ("Lifetime",), ()),
]


PATTERN_RULES = [
    Rule("pattern", "bind_pattern", ("parse_path(current)",), ("Bind",), ("value")),
    Rule("pattern", "constructor_pattern", ("PPAT005",), ("Constructor",), ("Option.Some(value)")),
    Rule("pattern", "struct_pattern", ("PPAT007",), ("Struct",), ("Pair { left")),
    Rule("pattern", "tuple_pattern", ("PPAT003",), ("Tuple",), ()),
    Rule("pattern", "list_pattern", ("PPAT004",), ("List",), ()),
    Rule("pattern", "range_pattern", ("PPAT001",), ("Range",), ()),
    Rule("pattern", "or_pattern", ('while at_text(current, "|")',), ("Or",), ("1 | 2")),
    Rule("pattern", "mut_pattern", ('at_text(current, "mut")',), ("Mut",), ("let mut")),
    Rule("pattern", "ref_pattern", ('at_text(current, "ref")',), ("Ref",), ()),
    Rule("pattern", "wildcard_pattern", ('at_text(current, "_")',), ("Wildcard",), ()),
    Rule("pattern", "literal_pattern", ("is_literal_token",), ("Literal",), ("case 0")),
]


ALL_RULES = DECL_RULES + STMT_RULES + EXPR_RULES + TYPE_RULES + PATTERN_RULES


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def valid_fixture_text() -> str:
    chunks = []
    for path in sorted(VALID_FIXTURES.glob("*.vit")):
        chunks.append(path.read_text(encoding="utf-8"))
    return "\n".join(chunks)


def status_for(rule: Rule, parser_text: str, ast_text: str, fixture_text: str) -> str:
    parser_ok = all(term in parser_text for term in rule.parser_terms)
    ast_ok = all(term in ast_text for term in rule.ast_terms)
    fixture_ok = True if not rule.fixture_terms else any(term in fixture_text for term in rule.fixture_terms)
    if parser_ok and ast_ok and fixture_ok:
        return "Tested"
    if parser_ok and ast_ok:
        return "Complete"
    if parser_ok or ast_ok:
        return "Partial"
    return "NotStarted"


def build_report() -> list[dict[str, str]]:
    parser_text = read(PARSER)
    ast_text = read(IR_AST) + "\n" + "\n".join(read(path) for path in sorted(FRONTEND_AST.glob("*.vit")))
    fixture_text = valid_fixture_text()
    return [
        {
            "category": rule.category,
            "rule": rule.name,
            "status": status_for(rule, parser_text, ast_text, fixture_text),
        }
        for rule in ALL_RULES
    ]


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)

    report = build_report()
    failures = [item for item in report if item["status"] in {"NotStarted", "Partial"}]

    if args.json:
        print(json.dumps(report, indent=2))
    else:
        by_category: dict[str, list[dict[str, str]]] = {}
        for item in report:
            by_category.setdefault(item["category"], []).append(item)
        for category, items in by_category.items():
            tested = sum(1 for item in items if item["status"] == "Tested")
            complete = sum(1 for item in items if item["status"] in {"Tested", "Complete"})
            print(f"[grammar-alignment] {category}: {complete}/{len(items)} complete, {tested}/{len(items)} tested")
            for item in items:
                if item["status"] in {"NotStarted", "Partial"}:
                    print(f"  - {item['status']}: {item['rule']}")

    if failures:
        return 1
    print("[grammar-alignment] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
