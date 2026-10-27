#!/usr/bin/env python3
"""Fail when the canonical EBNF, lexer tables, and parser drift apart."""

from __future__ import annotations

import re
import sys
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
GRAMMAR = ROOT / "grammar" / "vitte.ebnf"
LEXER = ROOT / "src" / "bootstrap" / "lexer" / "lexer.c"
PARSER = ROOT / "src" / "bootstrap" / "parser" / "parser.c"
TREE_SITTER = ROOT / "editors" / "tree-sitter" / "grammar.js"
TREE_SITTER_HIGHLIGHTS = ROOT / "editors" / "tree-sitter" / "queries" / "highlights.scm"
HIGHLIGHT = ROOT / "editors" / "grammar" / "vitte_highlight_grammar.json"
GENERATED_EBNF = (
    ROOT / "docs" / "grammar" / "vitte.ebnf",
    ROOT / "docs" / "book" / "grammar" / "grammar-surface.ebnf",
    ROOT / "docs" / "book" / "grammar" / "vitte.ebnf",
)

# The lexer keeps tokens for these aliases so the parser can reject them with
# a useful diagnostic; the canonical EBNF accepts the word operators instead.
LEGACY_LOGICAL_OPERATOR_ALIASES = {"&&", "||", "!"}


def without_comments(text: str) -> str:
    return re.sub(r"\(\*.*?\*\)", "", text, flags=re.DOTALL)


def generated_body(text: str) -> str:
    """Remove the generated-file prologue before comparing grammar bodies."""
    if text.startswith("(* GENERATED FILE - DO NOT EDIT"):
        _, separator, body = text.partition("\n\n")
        return body if separator else text
    return text


def main() -> int:
    grammar = without_comments(GRAMMAR.read_text(encoding="utf-8"))
    lexer = LEXER.read_text(encoding="utf-8")
    parser = PARSER.read_text(encoding="utf-8")
    tree_sitter = TREE_SITTER.read_text(encoding="utf-8")
    tree_sitter_highlights = TREE_SITTER_HIGHLIGHTS.read_text(encoding="utf-8")
    highlight = json.loads(HIGHLIGHT.read_text(encoding="utf-8"))

    terminals = set(re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', grammar))
    reserved_production = re.search(
        r"reserved_keyword\s*::=(.*?);", grammar, flags=re.DOTALL
    )
    reserved_keywords = (
        set(
            re.findall(
                r'"([^"\\]*(?:\\.[^"\\]*)*)"',
                reserved_production.group(1),
            )
        )
        if reserved_production
        else set()
    )
    keywords = re.findall(r'VITTE_KW\("([^"]+)",\s*(VITTE_TOKEN_[A-Z0-9_]+)\)', lexer)
    operators = re.findall(r'VITTE_OP\("([^"\\]+)",\s*(VITTE_TOKEN_[A-Z0-9_]+)\)', lexer)
    productions = set(re.findall(r"^\s*([a-z_][a-z0-9_]*)\s*::=", grammar, re.MULTILINE))

    errors: list[str] = []
    canonical_body = generated_body(GRAMMAR.read_text(encoding="utf-8"))
    for generated in GENERATED_EBNF:
        if not generated.exists():
            errors.append(f"generated grammar is missing: {generated}")
        elif generated_body(generated.read_text(encoding="utf-8")) != canonical_body:
            errors.append(f"generated grammar is out of date: {generated}")
    keyword_texts = [text for text, _ in keywords]
    operator_texts = [text for text, _ in operators]
    for label, values in (("keyword", keyword_texts), ("operator", operator_texts)):
        duplicates = sorted({value for value in values if values.count(value) > 1})
        errors.extend(f"duplicate lexer {label}: {value}" for value in duplicates)

    errors.extend(
        f"lexer keyword missing from EBNF: {text}"
        for text in sorted(set(keyword_texts) - terminals)
    )
    errors.extend(
        f"reserved EBNF keyword missing from lexer: {text}"
        for text in sorted(reserved_keywords - set(keyword_texts))
    )
    errors.extend(
        f"lexer operator missing from EBNF: {text}"
        for text in sorted(
            set(operator_texts) - terminals - LEGACY_LOGICAL_OPERATOR_ALIASES
        )
    )

    for text, token_kind in keywords + operators:
        if (text in LEGACY_LOGICAL_OPERATOR_ALIASES or
                token_kind == "VITTE_TOKEN_KW_RESERVED"):
            continue
        if token_kind not in parser:
            errors.append(f"lexer token is not consumed by parser: {token_kind} ({text})")

    # These productions are the frontend features that historically drifted
    # first.  Keep their parser entry points and editor rules explicit so a
    # grammar change cannot silently leave one frontend behind.
    required_parser_features = {
        "generic_params": "vitte_parser_parse_generic_parameters",
        "where_clause": "vitte_parser_parse_where_clause",
        "trait_decl": "vitte_parser_parse_trait_declaration",
        "impl_decl": "vitte_parser_parse_impl_declaration",
        "associated_type_decl": "vitte_parser_parse_type_declaration",
        "pattern": "vitte_parser_parse_pattern",
        "attr_prefix": "vitte_parser_parse_attribute_prefix",
    }
    for production, symbol in required_parser_features.items():
        if production in productions and symbol not in parser:
            errors.append(f"grammar production has no parser entry point: {production} ({symbol})")

    required_editor_rules = {
        "generic_params": "generic_parameters",
        "where_clause": "where_clause",
        "trait_decl": "trait_declaration",
        "impl_decl": "implementation_declaration",
        "associated_type_decl": "associated_type_declaration",
        "pattern": "pattern",
        "attr_prefix": "attribute",
    }
    for production, rule in required_editor_rules.items():
        if production in productions and not re.search(
            rf"^\s*{re.escape(rule)}\s*:", tree_sitter, re.MULTILINE
        ):
            errors.append(f"grammar production has no tree-sitter rule: {production} ({rule})")

    for rule in ("let_statement", "for_statement"):
        if f"{rule} (pattern" not in tree_sitter_highlights:
            errors.append(
                f"tree-sitter highlights do not follow pattern bindings: {rule}"
            )

    highlighted_keywords = set(highlight.get("keywords", []))
    highlighted_keywords.update(highlight.get("deprecated_keywords", []))
    highlighted_keywords.update(highlight.get("contextual_keywords", []))
    lexer_keywords = set(keyword_texts)
    if highlighted_keywords != lexer_keywords:
        missing = sorted(lexer_keywords - highlighted_keywords)
        extra = sorted(highlighted_keywords - lexer_keywords)
        if missing:
            errors.append("editor highlight keywords missing from lexer: " + ", ".join(missing))
        if extra:
            errors.append("editor highlight keywords missing in lexer: " + ", ".join(extra))

    if errors:
        print("[ebnf-sync] FAILED")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        f"[ebnf-sync] OK: {len(set(keyword_texts))} keywords, "
        f"{len(set(operator_texts))} operators, parser token coverage verified"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
