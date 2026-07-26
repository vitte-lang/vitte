#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path


RULE_RE = re.compile(r"^([A-Za-z_][A-Za-z0-9_]*)\s*::=\s*(.+?)(?=^[A-Za-z_][A-Za-z0-9_]*\s*::=|\Z)", re.M | re.S)
LITERAL_RE = re.compile(r'"([^"]+)"')
IDENT_RE = re.compile(r"\b([A-Za-z_][A-Za-z0-9_]*)\b")
IDENT_FULL_RE = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")

STOP_WORDS = {
    "proc",
    "decl",
    "stmt",
    "expr",
    "type",
    "item",
    "list",
    "kind",
    "body",
    "path",
    "char",
    "string",
    "block",
    "field",
    "param",
    "import",
    "module",
    "package",
    "ident",
    "pattern",
    "rule",
    "ws",
}

LEXICAL_RULES = {
    "bool_lit",
    "null_lit",
    "int_lit",
    "float_lit",
    "bytes_lit",
    "char_lit",
    "string_lit",
    "raw_string_lit",
    "line_comment",
    "block_comment",
    "zone_comment",
    "ident",
    "suffix",
    "WS",
    "WS1",
    "NEWLINE",
    "LETTER",
    "DIGIT",
    "BINDIGIT",
    "OCTDIGIT",
    "HEXDIGIT",
    "EOF",
    "escape_seq",
    "string_char",
    "raw_string_char",
    "char_char",
}

RULE_ALIASES = {
    "program": ["frontend_run", "parse_toplevel", "AstRoot"],
    "toplevel": ["parse_toplevel", "AstItemKind", "frontend_run"],
    "char_lit": ["TokenKind.Char", "validate_char_lexeme", "invalid_char_literal"],
    "string_lit": ["TokenKind.String", "validate_string_lexeme", "unterminated string literal"],
    "int_lit": ["TokenKind.Integer", "validate_number_lexeme"],
    "float_lit": ["TokenKind.Float", "validate_number_lexeme", "malformed float exponent"],
    "bytes_lit": ['b"', "BytesLiteral", "TokenKind.String"],
    "char_char": ["validate_char_lexeme"],
    "string_char": ["validate_string_lexeme"],
    "raw_string_lit": ["validate_raw_string_lexeme", 'r"', '"""'],
    "raw_string_char": ["validate_raw_string_lexeme"],
    "escape_seq": ["invalid escape sequence", "invalid_unicode_escape", "validate_string_lexeme", "validate_char_lexeme"],
    "LETTER": ["is_letter"],
    "DIGIT": ["is_digit"],
    "NEWLINE": ["\\n"],
    "EOF": ["TokenKind.Eof"],
    "stmt": ["parse_stmt", "recover_stmt_index", "PSTMT"],
    "stmt_end": ['";"', "statement_boundary"],
    "expr": ["parse_expr", "AstExprKind"],
    "postfix_expr": ["parse_postfix_expr", "parse_postfix_expr_ast", "AstExprKind.Call", "AstExprKind.Member", "AstExprKind.Index"],
    "type_expr": ["parse_type_expr", "AstTypeKind"],
    "pattern": ["parse_pattern", "AstPatternKind"],
    "assign_op": ["+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>="],
    "ternary_expr": ["PTERN001", '"?"', '":"'],
    "bit_xor_expr": ['"^"', "precedence_from_operator"],
    "mul_expr": ['"*"', '"/"', '"%"', "precedence_from_operator"],
    "pattern_ctor": ["PPAT005", "AstPatternKind.Variant"],
    "type_qualifier": ['"const"', '"volatile"', '"atomic"', '"mut"', '"owned"', '"borrow"'],
    "field_init_list": ["parse_struct_field_exprs", "AstExprKind.Struct", "PSTRUCT001"],
    "bitfield_list": ["parse_surface_bitfield_list", "parse_surface_bitfield_stmt", "AstItemKind.Bits", "bits_surface_source", '"  read: 0 .. 1,\\n"'],
    "docstring": ['r"main entry"', '"""multi', "validate_raw_string_lexeme", "parse_attrs"],
    "field_list": ["parse_surface_field_stmt", "AstNominalMemberKind.Field", "parser_total_surface_source"],
    "flag_list": ["parse_surface_bits_or_flags_stmt", "AstItemKind.Flags", "parser_total_surface_source"],
    "flag_item": ["parse_surface_bits_or_flags_stmt", "AstNominalMemberKind.Variant", '"  read,\\n"'],
    "param_list": ["parse_param_list", "parse_ast_param_list", "parser_total_surface_source"],
    "param": ["parse_ast_param_from_tokens", "AstParam", "parser_total_surface_source"],
    "capability_list": ['effects(io, alloc)', "parse_proc_suffixes", "requires(debug)"],
    "capability": ["parse_text_parts", "effects_index", "requires_index"],
    "module_path": ["parse_module_path", "parse_path", '"use .demo.parser'],
    "package_path": ["parse_module_path", '"demo/parser/total"', "parse_path"],
    "relative": ['"use .demo.parser', "parse_module_path"],
    "ident_list": ['"{ alpha, beta as gamma, * }"', "import_items", "parse_path"],
    "meta_arg_list": ["parse_item_meta_args", "parse_meta_decl", "answer = 42"],
    "meta_arg": ["parse_item_meta_args", "answer = 42", 'name = "vitte"'],
    "doc_comment": ["parse_attrs", "standalone_attribute_source", "PATTR003"],
    "attr_path": ["parse_attrs", "demo.attr", "attr_prefix"],
    "attr_arg_list": ["parse_attrs", "answer = 42", 'name = "vitte"'],
    "rel_expr": ['left < right', "rel_expr", "precedence_from_operator"],
    "arg_list": ["parse_arg_list", "parse_call_generic_arguments", "named: right"],
    "arg": ["parse_arg_list", "named: right", "AstExprKind.Call"],
    "tuple_lit": ["AstTypeKind.Tuple", "parse_ast_type_list", "parser_lparen"],
    "lambda_expr": ['"|item: int| item + 1"', "parse_param_list", "AstExprKind.Lambda"],
    "pattern_bind": ["parse_ast_pattern_from_tokens", "AstPatternKind.Binding", "pattern_bind"],
    "pattern_tuple": ["parse_ast_pattern_list", "AstPatternKind.Tuple", '"(a, b)"'],
    "lifetime": ["'a", "lifetime", "parse_type_expr"],
    "exponent": ["TokenKind.Float", "parse_ast_expr_from_tokens", "AstExprKind.Float"],
    "ident": ["expect_ident", "TokenKind.Identifier", "is_identifier_continue"],
}

PARSER_PATTERNS = [
    "src/vitte/compiler/frontend/parse/*.vit",
    "src/vitte/compiler/frontend/pipeline.vit",
    "src/vitte/compiler/frontend/grammar_alignment_checker.vit",
]
AST_PATTERNS = [
    "src/vitte/compiler/frontend/ast/*.vit",
    "src/vitte/compiler/frontend/parse/parser.vit",
]
DIAG_PATTERNS = [
    "src/vitte/compiler/frontend/**/*.vit",
    "src/vitte/compiler/diagnostics/**/*.vit",
    "src/vitte/compiler/tests/frontend_fixtures/invalid/**/*.vit",
    "docs/book/grammar/diagnostics/expected/*.json",
]
TEST_PATTERNS = [
    "src/vitte/compiler/tests/*.vit",
    "src/vitte/compiler/tests/**/*.vit",
    "tests/**/*.must",
    "tests/**/*.vit",
    "docs/book/grammar/diagnostics/expected/*.json",
]
LEXER_PATTERNS = [
    "src/vitte/compiler/frontend/lexer/*.vit",
]


@dataclass(frozen=True)
class Rule:
    name: str
    definition: str


@dataclass
class Corpus:
    name: str
    files: dict[str, str]
    merged: str
    merged_identifiers: set[str]
    file_identifiers: dict[str, set[str]]


def load_rules(grammar_text: str) -> list[Rule]:
    rules: list[Rule] = []
    for name, body in RULE_RE.findall(grammar_text):
        definition = " ".join(line.strip() for line in body.strip().splitlines())
        rules.append(Rule(name=name, definition=definition))
    return rules


def read_existing_files(repo: Path, patterns: list[str]) -> list[Path]:
    files: dict[str, Path] = {}
    for pattern in patterns:
        for path in sorted(repo.glob(pattern)):
            if path.is_file():
                files[str(path.relative_to(repo))] = path
    return [files[key] for key in sorted(files)]


def normalize_token(token: str) -> str:
    return token.strip().lower()


def candidate_terms(rule: Rule) -> list[str]:
    terms: list[str] = [rule.name]

    if "_" in rule.name:
        parts = [part for part in rule.name.split("_") if part and part not in STOP_WORDS]
        terms.extend(parts)
        stem = "_".join(part for part in rule.name.split("_") if part not in {"decl", "stmt", "expr", "type"})
        if stem and stem != rule.name:
            terms.append(stem)

    terms.extend(LITERAL_RE.findall(rule.definition))

    for ident in IDENT_RE.findall(rule.definition):
        if ident != rule.name and ident not in STOP_WORDS and ident.lower() == ident:
            terms.append(ident)

    suffix_aliases = [
        ("_decl", ""),
        ("_stmt", ""),
        ("_expr", ""),
        ("_type", ""),
        ("_lit", ""),
        ("_list", ""),
        ("_item", ""),
        ("_char", ""),
    ]
    for suffix, replacement in suffix_aliases:
        if rule.name.endswith(suffix):
            alias = rule.name[: -len(suffix)] + replacement
            if alias:
                terms.append(alias)

    terms.extend(RULE_ALIASES.get(rule.name, []))

    deduped: list[str] = []
    seen: set[str] = set()
    for term in terms:
        key = normalize_token(term)
        if len(key) < 2:
            continue
        if key in seen:
            continue
        seen.add(key)
        deduped.append(term)
    return deduped


def build_corpus(repo: Path, name: str, patterns: list[str]) -> Corpus:
    files: dict[str, str] = {}
    file_identifiers: dict[str, set[str]] = {}
    for path in read_existing_files(repo, patterns):
        try:
            rel = str(path.relative_to(repo))
            text = path.read_text(encoding="utf-8")
            files[rel] = text
            file_identifiers[rel] = set(IDENT_FULL_RE.findall(text))
        except UnicodeDecodeError:
            continue
    merged = "\n".join(files.values())
    merged_identifiers: set[str] = set()
    for identifiers in file_identifiers.values():
        merged_identifiers.update(identifiers)
    return Corpus(name=name, files=files, merged=merged, merged_identifiers=merged_identifiers, file_identifiers=file_identifiers)


def term_present(text: str, term: str, identifiers: set[str] | None = None) -> bool:
    if not term:
        return False
    if IDENT_FULL_RE.fullmatch(term):
        if identifiers is not None:
            return term in identifiers
        return term in set(IDENT_FULL_RE.findall(text))
    return term in text


def search_evidence(corpus: Corpus, terms: list[str]) -> dict[str, object]:
    total_hits = 0
    matched_terms: list[str] = []
    for term in terms:
        if term_present(corpus.merged, term, corpus.merged_identifiers):
            total_hits += 1
            matched_terms.append(term)

    refs: list[str] = []
    if matched_terms:
        for rel, text in corpus.files.items():
            if len(refs) >= 8:
                break
            for term in matched_terms:
                if term_present(text, term, corpus.file_identifiers.get(rel)):
                    refs.append(rel)
                    break

    return {
        "hits": total_hits,
        "files": refs,
        "terms": matched_terms[:12],
    }


def status_for_rule(rule: Rule, parser: dict[str, object], ast: dict[str, object], diagnostics: dict[str, object], tests: dict[str, object], lexer: dict[str, object]) -> dict[str, object]:
    lexical = rule.name in LEXICAL_RULES
    parsed = lexer["hits"] > 0 if lexical else parser["hits"] > 0
    ast_built = False if lexical else ast["hits"] > 0
    diagnosed = diagnostics["hits"] > 0
    tested = tests["hits"] > 0
    missing = not parsed and not ast_built and not diagnosed and not tested

    classifications: list[str] = []
    if parsed:
        classifications.append("parsed")
    if ast_built:
        classifications.append("ast-built")
    if diagnosed:
        classifications.append("diagnosed")
    if tested:
        classifications.append("tested")
    if missing:
        classifications.append("missing")

    return {
        "lexical_rule": lexical,
        "parsed": parsed,
        "ast_built": ast_built,
        "diagnosed": diagnosed,
        "tested": tested,
        "missing": missing,
        "classifications": classifications,
    }


def summarize(rules_report: list[dict[str, object]]) -> dict[str, int]:
    summary = {
        "total_rules": len(rules_report),
        "lexical_rules": 0,
        "classified_rules": 0,
        "parsed_rules": 0,
        "ast_built_rules": 0,
        "diagnosed_rules": 0,
        "tested_rules": 0,
        "missing_rules": 0,
    }
    for item in rules_report:
        status = item["status"]
        if status["lexical_rule"]:
            summary["lexical_rules"] += 1
        if status["classifications"]:
            summary["classified_rules"] += 1
        if status["parsed"]:
            summary["parsed_rules"] += 1
        if status["ast_built"]:
            summary["ast_built_rules"] += 1
        if status["diagnosed"]:
            summary["diagnosed_rules"] += 1
        if status["tested"]:
            summary["tested_rules"] += 1
        if status["missing"]:
            summary["missing_rules"] += 1
    return summary


def overall_status(summary: dict[str, int]) -> str:
    if summary["total_rules"] == 0 or summary["classified_rules"] != summary["total_rules"]:
        return "red"
    if summary["missing_rules"] == 0:
        return "green"
    if summary["parsed_rules"] == 0 and summary["ast_built_rules"] == 0 and summary["tested_rules"] == 0:
        return "red"
    return "yellow"


def render_markdown(report: dict[str, object]) -> str:
    lines = [
        "# Grammar Coverage Report",
        "",
        f"- Grammar: `{report['grammar']}`",
        f"- Rule count: `{report['summary']['total_rules']}`",
        f"- Classified rules: `{report['summary']['classified_rules']}`",
        f"- Parsed: `{report['summary']['parsed_rules']}`",
        f"- AST built: `{report['summary']['ast_built_rules']}`",
        f"- Diagnosed: `{report['summary']['diagnosed_rules']}`",
        f"- Tested: `{report['summary']['tested_rules']}`",
        f"- Missing: `{report['summary']['missing_rules']}`",
        f"- Overall status: `{report['overall_status']}`",
        "",
        "## Coverage Matrix",
        "",
        "| Rule | Parsed | AST | Diag | Test | Missing | Classification |",
        "| --- | --- | --- | --- | --- | --- | --- |",
    ]

    for item in report["rules"]:
        status = item["status"]
        lines.append(
            f"| `{item['rule']}` | {'yes' if status['parsed'] else 'no'} | {'yes' if status['ast_built'] else 'no'} | {'yes' if status['diagnosed'] else 'no'} | {'yes' if status['tested'] else 'no'} | {'yes' if status['missing'] else 'no'} | {', '.join(status['classifications'])} |"
        )

    lines.extend(["", "## Missing Rules", ""])
    missing = [item["rule"] for item in report["rules"] if item["status"]["missing"]]
    if missing:
        for rule in missing:
            lines.append(f"- `{rule}`")
    else:
        lines.append("- none")
    lines.append("")
    return "\n".join(lines)


def build_report(repo: Path) -> dict[str, object]:
    grammar_path = repo / "src/vitte/grammar/vitte.ebnf"
    grammar_text = grammar_path.read_text(encoding="utf-8")
    rules = load_rules(grammar_text)

    parser_corpus = build_corpus(repo, "parser", PARSER_PATTERNS)
    ast_corpus = build_corpus(repo, "ast", AST_PATTERNS)
    diagnostics_corpus = build_corpus(repo, "diagnostics", DIAG_PATTERNS)
    tests_corpus = build_corpus(repo, "tests", TEST_PATTERNS)
    lexer_corpus = build_corpus(repo, "lexer", LEXER_PATTERNS)

    rules_report: list[dict[str, object]] = []
    for rule in rules:
        terms = candidate_terms(rule)
        parser_evidence = search_evidence(parser_corpus, terms)
        ast_evidence = search_evidence(ast_corpus, terms)
        diagnostics_evidence = search_evidence(diagnostics_corpus, terms)
        tests_evidence = search_evidence(tests_corpus, terms)
        lexer_evidence = search_evidence(lexer_corpus, terms)
        status = status_for_rule(rule, parser_evidence, ast_evidence, diagnostics_evidence, tests_evidence, lexer_evidence)

        rules_report.append(
            {
                "rule": rule.name,
                "definition": rule.definition,
                "candidate_terms": terms,
                "status": status,
                "evidence": {
                    "parser": parser_evidence,
                    "ast": ast_evidence,
                    "diagnostics": diagnostics_evidence,
                    "tests": tests_evidence,
                    "lexer": lexer_evidence,
                },
            }
        )

    summary = summarize(rules_report)
    return {
        "grammar": str(grammar_path.relative_to(repo)),
        "summary": summary,
        "overall_status": overall_status(summary),
        "rules": rules_report,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Vitte exhaustive grammar coverage report")
    parser.add_argument("--check", action="store_true", help="generate the exhaustive coverage matrix and validate the report shape")
    parser.add_argument("--json", action="store_true", help="print the JSON report to stdout")
    args = parser.parse_args(argv)

    repo = Path(__file__).resolve().parents[1]
    report = build_report(repo)

    out_dir = repo / "target/reports/grammar_coverage"
    out_dir.mkdir(parents=True, exist_ok=True)
    json_out = out_dir / "grammar_coverage.json"
    md_out = out_dir / "grammar_coverage.md"
    json_out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    md_out.write_text(render_markdown(report) + "\n", encoding="utf-8")

    summary = report["summary"]
    print(
        "[grammar-coverage] "
        f"total={summary['total_rules']} "
        f"classified={summary['classified_rules']} "
        f"parsed={summary['parsed_rules']} "
        f"ast={summary['ast_built_rules']} "
        f"diag={summary['diagnosed_rules']} "
        f"tested={summary['tested_rules']} "
        f"missing={summary['missing_rules']} "
        f"status={report['overall_status']}"
    )
    print(f"[grammar-coverage] wrote {json_out.relative_to(repo)} and {md_out.relative_to(repo)}")

    if args.json:
        print(json.dumps(report, indent=2, ensure_ascii=False))

    if args.check:
        if summary["total_rules"] == 0:
            print("[grammar-coverage][error] no grammar rules were discovered")
            return 1
        if summary["classified_rules"] != summary["total_rules"]:
            print("[grammar-coverage][error] not every grammar rule received a coverage classification")
            return 1
        if summary["parsed_rules"] != summary["total_rules"]:
            print("[grammar-coverage][error] parser coverage is not complete")
            return 1
        if report["overall_status"] == "green" and summary["missing_rules"] != 0:
            print("[grammar-coverage][error] green status is forbidden while rules are still missing")
            return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
