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
    "char_lit": ["TokenKind.Char", "validate_char_lexeme", "invalid_char_literal", "unterminated char literal"],
    "string_lit": ["TokenKind.String", "validate_string_lexeme", "unterminated string literal"],
    "int_lit": ["TokenKind.Integer", "validate_number_lexeme"],
    "float_lit": ["TokenKind.Float", "validate_number_lexeme"],
    "bytes_lit": ['at_text(current, "b")', "TokenKind.String"],
    "char_char": ["validate_char_lexeme"],
    "string_char": ["validate_string_lexeme"],
    "escape_seq": ["invalid escape sequence", "truncated escape sequence", "validate_string_lexeme", "validate_char_lexeme"],
    "LETTER": ["is_letter"],
    "DIGIT": ["is_digit"],
    "BINDIGIT": ["is_binary_digit"],
    "OCTDIGIT": ["is_octal_digit"],
    "HEXDIGIT": ["is_hex_digit"],
    "NEWLINE": ["\\n"],
    "EOF": ["TokenKind.Eof"],
    "ternary_expr": ["PTERN001", '"?"', '":"'],
    "bit_xor_expr": ['"^"', "precedence_from_operator", "assign_op_xor"],
    "mul_expr": ['"*"', '"/"', '"%"', "precedence_from_operator", "assign_op_mul", "assign_op_div", "assign_op_mod"],
    "pattern_ctor": ["PPAT005", "AstPatternKind.Variant", "ast_pattern_variant"],
    "type_qualifier": ['"const"', '"volatile"', '"atomic"', '"mut"', '"owned"', '"borrow"', '"user"', '"kernel"', '"phys"', '"mmio"', '"dma"'],
    "field_init_list": ["parse_struct_field_exprs", "AstExprKind.Struct", "PSTRUCT001"],
    "exponent": ["malformed float exponent", "1.5e10", "2E-3", "6.02e+23"],
    "docstring": ['r"main entry"', '"""multi', "validate_raw_string_lexeme", "unterminated raw string literal"],
    "bitfield_list": ['"bits"', "parse_container_decl", "AstItemKind.Bits", "bits_surface_source", '"read: 0 .. 1"'],
}


@dataclass
class Rule:
    name: str
    definition: str


def load_rules(grammar_text: str) -> list[Rule]:
    rules: list[Rule] = []
    for name, body in RULE_RE.findall(grammar_text):
        definition = " ".join(line.strip() for line in body.strip().splitlines())
        rules.append(Rule(name=name, definition=definition))
    return rules


def read_existing_files(repo: Path, patterns: list[str]) -> list[Path]:
    files: list[Path] = []
    for pattern in patterns:
        files.extend(sorted(repo.glob(pattern)))
    return [path for path in files if path.is_file()]


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


def count_term_matches(text: str, term: str) -> int:
    escaped = re.escape(term)
    if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", term):
        pattern = rf"\b{escaped}\b"
    else:
        pattern = escaped
    return len(re.findall(pattern, text))


def collect_corpus(repo: Path, patterns: list[str]) -> tuple[str, dict[str, str]]:
    files = read_existing_files(repo, patterns)
    by_file: dict[str, str] = {}
    for path in files:
        try:
            by_file[str(path.relative_to(repo))] = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
    merged = "\n".join(by_file.values())
    return merged, by_file


def search_evidence(corpus_files: dict[str, str], terms: list[str]) -> tuple[int, list[str], list[str]]:
    total = 0
    files: list[str] = []
    matched_terms: list[str] = []
    for rel, text in corpus_files.items():
        file_hits = 0
        local_matches: list[str] = []
        for term in terms:
            hits = count_term_matches(text, term)
            if hits > 0:
                file_hits += hits
                local_matches.append(term)
        if file_hits > 0:
            total += file_hits
            files.append(rel)
            matched_terms.extend(local_matches)
    # preserve order while deduping
    uniq_terms: list[str] = []
    seen_terms: set[str] = set()
    for term in matched_terms:
        key = normalize_token(term)
        if key in seen_terms:
            continue
        seen_terms.add(key)
        uniq_terms.append(term)
    return total, files[:8], uniq_terms[:12]


def status_for_rule(rule: Rule, parser_hits: int, ast_hits: int, diag_hits: int, test_hits: int, lexer_hits: int) -> dict[str, bool]:
    lexical = rule.name in LEXICAL_RULES
    parsed = lexer_hits > 0 if lexical else parser_hits > 0
    ast_built = False if lexical else ast_hits > 0
    diagnosed = diag_hits > 0
    tested = test_hits > 0
    return {
        "lexical_rule": lexical,
        "parsed": parsed,
        "ast_built": ast_built,
        "diagnosed": diagnosed,
        "tested": tested,
        "missing": not parsed and not ast_built and not diagnosed and not tested,
    }


def summarize(rules_report: list[dict]) -> dict[str, int]:
    summary = {
        "total_rules": len(rules_report),
        "lexical_rules": 0,
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


def render_markdown(report: dict) -> str:
    lines = [
        "# Parser Sync Coverage",
        "",
        f"- Grammar: `{report['grammar']}`",
        f"- Parser corpus: `{report['parser_corpus_root']}`",
        f"- Total rules: `{report['summary']['total_rules']}`",
        f"- Parsed evidence: `{report['summary']['parsed_rules']}`",
        f"- AST evidence: `{report['summary']['ast_built_rules']}`",
        f"- Diagnostic evidence: `{report['summary']['diagnosed_rules']}`",
        f"- Test evidence: `{report['summary']['tested_rules']}`",
        f"- Missing evidence: `{report['summary']['missing_rules']}`",
        "",
        "## Missing rules",
        "",
    ]

    missing = [item["rule"] for item in report["rules"] if item["status"]["missing"]]
    if missing:
        for rule in missing:
            lines.append(f"- `{rule}`")
    else:
        lines.append("- none")

    lines.extend(["", "## Sample matrix", "", "| Rule | Parsed | AST | Diag | Test |", "| --- | --- | --- | --- | --- |"])
    for item in report["rules"][:40]:
        status = item["status"]
        lines.append(
            f"| `{item['rule']}` | {'yes' if status['parsed'] else 'no'} | {'yes' if status['ast_built'] else 'no'} | {'yes' if status['diagnosed'] else 'no'} | {'yes' if status['tested'] else 'no'} |"
        )
    lines.append("")
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(description="Vitte parser sync coverage report")
    parser.add_argument("--check", action="store_true", help="generate the exhaustive coverage matrix and require non-empty coverage signals")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    grammar_path = repo / "src/vitte/grammar/vitte.ebnf"
    grammar_text = grammar_path.read_text(encoding="utf-8")
    rules = load_rules(grammar_text)

    parser_text, parser_files = collect_corpus(
        repo,
        [
            "src/vitte/compiler/frontend/parse/*.vit",
            "src/vitte/compiler/frontend/grammar_alignment_checker.vit",
        ],
    )
    ast_text, ast_files = collect_corpus(repo, ["src/vitte/compiler/frontend/ast/*.vit"])
    diag_text, diag_files = collect_corpus(
        repo,
        [
            "src/vitte/compiler/frontend/**/*.vit",
            "src/vitte/compiler/diagnostics/**/*.vit",
            "docs/book/grammar/diagnostics/expected/*.json",
        ],
    )
    test_text, test_files = collect_corpus(
        repo,
        [
            "src/vitte/compiler/tests/*.vit",
            "tests/**/*.must",
            "tests/**/*.vit",
            "docs/book/grammar/diagnostics/expected/*.json",
        ],
    )
    lexer_text, lexer_files = collect_corpus(repo, ["src/vitte/compiler/frontend/lexer/*.vit"])

    rules_report: list[dict] = []
    for rule in rules:
        terms = candidate_terms(rule)
        parser_hits, parser_refs, parser_terms = search_evidence(parser_files, terms)
        ast_hits, ast_refs, ast_terms = search_evidence(ast_files, terms)
        diag_hits, diag_refs, diag_terms = search_evidence(diag_files, terms)
        test_hits, test_refs, test_terms = search_evidence(test_files, terms)
        lexer_hits, lexer_refs, lexer_terms = search_evidence(lexer_files, terms)

        rules_report.append(
            {
                "rule": rule.name,
                "definition": rule.definition,
                "candidate_terms": terms,
                "status": status_for_rule(rule, parser_hits, ast_hits, diag_hits, test_hits, lexer_hits),
                "evidence": {
                    "parser": {
                        "hits": parser_hits,
                        "files": parser_refs,
                        "terms": parser_terms,
                    },
                    "ast": {
                        "hits": ast_hits,
                        "files": ast_refs,
                        "terms": ast_terms,
                    },
                    "diagnostics": {
                        "hits": diag_hits,
                        "files": diag_refs,
                        "terms": diag_terms,
                    },
                    "tests": {
                        "hits": test_hits,
                        "files": test_refs,
                        "terms": test_terms,
                    },
                    "lexer": {
                        "hits": lexer_hits,
                        "files": lexer_refs,
                        "terms": lexer_terms,
                    },
                },
            }
        )

    report = {
        "grammar": str(grammar_path.relative_to(repo)),
        "parser_corpus_root": "src/vitte/compiler/frontend/parse",
        "summary": summarize(rules_report),
        "rules": rules_report,
    }

    out_dir = repo / "target/reports"
    out_dir.mkdir(parents=True, exist_ok=True)
    json_out = out_dir / "parser_sync_coverage.json"
    md_out = out_dir / "parser_sync_coverage.md"
    json_out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    md_out.write_text(render_markdown(report) + "\n", encoding="utf-8")

    summary = report["summary"]
    print(
        "[parser-sync-coverage] "
        f"rules={summary['total_rules']} "
        f"parsed={summary['parsed_rules']} "
        f"ast={summary['ast_built_rules']} "
        f"diag={summary['diagnosed_rules']} "
        f"tested={summary['tested_rules']} "
        f"missing={summary['missing_rules']}"
    )
    print(f"[parser-sync-coverage] wrote {json_out.relative_to(repo)} and {md_out.relative_to(repo)}")

    if args.check:
        if summary["total_rules"] == 0 or summary["parsed_rules"] == 0:
            print("[parser-sync-coverage][error] coverage report is empty or failed to detect parser evidence")
            return 1
        return 0

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
