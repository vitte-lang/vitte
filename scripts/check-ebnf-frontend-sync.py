#!/usr/bin/env python3
"""Check canonical EBNF terminal coverage against lexer and parser tables."""

from __future__ import annotations

import ast
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
GRAMMAR = ROOT / "grammar" / "vitte.ebnf"
C_FRONTENDS = (
    (
        "src/src",
        ROOT / "src" / "src" / "lexer" / "lexer.c",
        ROOT / "src" / "src" / "parser" / "parser.c",
    ),
    (
        "src/bootstrap",
        ROOT / "src" / "bootstrap" / "lexer" / "lexer.c",
        ROOT / "src" / "bootstrap" / "parser" / "parser.c",
    ),
)
NATIVE_LEXER = ROOT / "src" / "compiler" / "lexer" / "lexer.vitte"
TOKEN_TEST = ROOT / "src" / "tests" / "frontend_token_test.c"


def without_comments(text: str) -> str:
    return re.sub(r"\(\*.*?\*\)", "", text, flags=re.DOTALL)


def c_lexer_entries(source: str, macro: str) -> list[tuple[str, str]]:
    return re.findall(
        rf'{macro}\("([^"\\]+)",\s*(VITTE_TOKEN_[A-Z0-9_]+)\)',
        source,
    )


def native_lexer_entries(source: str) -> tuple[list[str], list[str]]:
    keyword_function = re.search(
        r"(?ms)^proc lexer_keyword\(text: string\) -> TokenKind \{(.*?)^\}",
        source,
    )
    operators = re.search(
        r"(?s)LEXER_OPERATORS:\s*\[string\]\s*=\s*\[(.*?)\];",
        source,
    )
    if keyword_function is None or operators is None:
        raise ValueError("could not locate native lexer keyword or operator table")
    keywords = re.findall(r'text\s*==\s*"([^"]+)"', keyword_function.group(1))
    operator_texts = re.findall(r'"([^"]+)"', operators.group(1))
    return keywords, operator_texts


def grammar_keyword_entries(grammar: str) -> set[str]:
    production = re.search(
        r"(?ms)^vitte_keyword_token\s*::=\s*(.*?);",
        grammar,
    )
    if production is None:
        raise ValueError("could not locate vitte_keyword_token production")
    return set(re.findall(r'"([^"\\]+)"', production.group(1)))


def grammar_syntax_terminals(grammar: str, grammar_keywords: set[str]) -> set[str]:
    syntax = grammar.split("EOF ::=", 1)[0]
    terminals = set(re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', syntax))
    return terminals - grammar_keywords


def tested_keyword_entries(source: str) -> set[tuple[str, str]]:
    cases = re.search(
        r"(?s)static const keyword_case_t keywords\[\] = \{(.*?)\n    \};",
        source,
    )
    if cases is None:
        raise ValueError("could not locate complete lexer keyword test table")
    return set(
        re.findall(
            r'\{\s*"([^"\\]+)"\s*,\s*(VITTE_TOKEN_[A-Z0-9_]+)\s*\}',
            cases.group(1),
        )
    )


def tested_operator_spellings(source: str) -> set[str]:
    function = re.search(
        r"(?s)static void\s+test_maximal_munch_operators\(void\)\s*\{(.*?)\n\}",
        source,
    )
    if function is None:
        raise ValueError("could not locate maximal-munch lexer test")
    initializer = re.search(
        r'(?s)static const char source\[\]\s*=\s*((?:"(?:\\.|[^"\\])*"\s*)+);',
        function.group(1),
    )
    if initializer is None:
        raise ValueError("could not locate maximal-munch lexer input")
    literals = re.findall(r'"(?:\\.|[^"\\])*"', initializer.group(1))
    return set("".join(ast.literal_eval(literal) for literal in literals).split())


def main() -> int:
    grammar = without_comments(GRAMMAR.read_text(encoding="utf-8"))
    native_lexer = NATIVE_LEXER.read_text(encoding="utf-8")
    token_test = TOKEN_TEST.read_text(encoding="utf-8")
    grammar_keywords = grammar_keyword_entries(grammar)
    grammar_syntax_tokens = grammar_syntax_terminals(grammar, grammar_keywords)
    tested_keywords = tested_keyword_entries(token_test)
    tested_operators = tested_operator_spellings(token_test)

    terminals = set(re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', grammar))
    errors: list[str] = []
    all_keywords: set[str] = set()
    all_operators: set[str] = set()

    for label, lexer_path, parser_path in C_FRONTENDS:
        lexer = lexer_path.read_text(encoding="utf-8")
        parser = parser_path.read_text(encoding="utf-8")
        keywords = c_lexer_entries(lexer, "VITTE_KW")
        operators = c_lexer_entries(lexer, "VITTE_OP")

        for token_label, entries in (("keyword", keywords), ("operator", operators)):
            texts = [text for text, _ in entries]
            duplicates = sorted({text for text in texts if texts.count(text) > 1})
            errors.extend(
                f"duplicate {label} lexer {token_label}: {text}"
                for text in duplicates
            )

        all_keywords.update(text for text, _ in keywords)
        all_operators.update(text for text, _ in operators)

        for text, _ in keywords:
            if text not in terminals:
                errors.append(f"{label} lexer keyword missing from EBNF: {text}")
        for text, _ in operators:
            if text not in terminals:
                errors.append(f"{label} lexer operator missing from EBNF: {text}")

        for text, token_kind in keywords + operators:
            if token_kind not in parser:
                errors.append(
                    f"{label} lexer token is not referenced by parser: "
                    f"{token_kind} ({text})"
                )

        # src/bootstrap is the C compatibility frontend for the canonical
        # grammar. src/src is an older lexer with a smaller vocabulary.
        if label == "src/bootstrap":
            missing_keywords = sorted(grammar_keywords - {text for text, _ in keywords})
            errors.extend(
                f"{label} lexer is missing canonical grammar keyword: {text}"
                for text in missing_keywords
            )
            missing_operators = sorted(grammar_syntax_tokens - {text for text, _ in operators})
            errors.extend(
                f"{label} lexer is missing canonical grammar syntax token: {text}"
                for text in missing_operators
            )
            tested_keyword_map = dict(tested_keywords)
            lexer_keyword_map = dict(keywords)
            errors.extend(
                f"lexer token test is missing canonical keyword: {text}"
                for text in sorted(grammar_keywords - set(tested_keyword_map))
            )
            errors.extend(
                f"lexer token test expects the wrong kind for {text}: "
                f"expected {lexer_keyword_map.get(text)}, got {kind}"
                for text, kind in sorted(tested_keywords)
                if text in lexer_keyword_map and lexer_keyword_map[text] != kind
            )
            errors.extend(
                f"lexer token test is missing operator/delimiter: {text}"
                for text in sorted(
                    {text for text, _ in operators} - tested_operators
                )
            )

    native_keywords, native_operators = native_lexer_entries(native_lexer)
    for label, values in (
        ("keyword", native_keywords),
        ("operator", native_operators),
    ):
        duplicates = sorted({value for value in values if values.count(value) > 1})
        errors.extend(f"duplicate native lexer {label}: {value}" for value in duplicates)
        missing = sorted(set(values) - terminals)
        errors.extend(f"native lexer {label} missing from EBNF: {value}" for value in missing)

    all_keywords.update(native_keywords)
    all_operators.update(native_operators)

    errors.extend(
        f"native lexer is missing canonical grammar keyword: {text}"
        for text in sorted(grammar_keywords - set(native_keywords))
    )
    errors.extend(
        f"native lexer is missing canonical grammar syntax token: {text}"
        for text in sorted(grammar_syntax_tokens - set(native_operators))
    )

    if errors:
        print("[ebnf-sync] FAILED")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        f"[ebnf-sync] OK: {len(grammar_keywords)} canonical keywords and "
        f"{len(grammar_syntax_tokens)} syntax tokens are covered by native/bootstrap; "
        f"all {len(grammar_keywords)} keyword-token mappings have lexer tests; "
        f"{len(all_operators)} operator/delimiter spellings checked across 3 lexers; "
        "legacy C parser token references verified"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
