#!/usr/bin/env python3
from __future__ import annotations

import ast
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
GRAMMAR = ROOT / "src/vitte/grammar/vitte.ebnf"
SCANNER = ROOT / "src/vitte/compiler/frontend/lexer/scanner.vit"
LITERALS = ROOT / "src/vitte/compiler/frontend/lexer/literals.vit"
COMMENTS = ROOT / "src/vitte/compiler/frontend/lexer/comments.vit"


IDENT_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
INT_FRAGMENT_RE = re.compile(r"^(0|1|7|9|0x|0b|0o)$")

SYMBOL_TERMINALS = {
    "!=",
    "#![",
    "#[",
    "%",
    "%=",
    "&",
    "&&",
    "&=",
    "(",
    ")",
    "*",
    "*=",
    "+",
    "+=",
    ",",
    "-",
    "-=",
    "->",
    ".",
    "..",
    "...",
    "..=",
    "/",
    "/=",
    ":",
    "::",
    ";",
    "<",
    "<<",
    "<<=",
    "<=",
    "=",
    "==",
    "=>",
    ">",
    ">=",
    ">>",
    ">>=",
    "?",
    "??",
    "[",
    "]",
    "^",
    "^=",
    "{",
    "|",
    "|=",
    "||",
    "}",
    "~",
}

COMMENT_TERMINALS = {
    "#",
    "//",
    "///",
    "/*",
    "*/",
    "<<<",
    ">>>",
}

LITERAL_INTERNALS = {
    "'",
    '"',
    '"""',
    "\\n",
    "\\r",
    "\\t",
    "\\0",
    '\\"',
    "\\'",
    "\\\\",
    "\\x",
    "\\u{",
    'r"',
    "a",
    "b",
    "e",
    "f",
    "z",
    "A",
    "C",
    "E",
    "F",
    "Z",
}

WHITESPACE_TERMINALS = {" ", "\t", "\n", "\r\n"}

COMPOSITE_EXAMPLES = {"()", "[]", "{ ... }"}


def quoted_terminals(text: str) -> set[str]:
    terms: set[str] = set()
    for match in re.finditer(r'"(?:\\.|[^"\\])*"', text):
        raw = match.group(0)
        try:
            terms.add(ast.literal_eval(raw))
        except Exception as exc:  # pragma: no cover - defensive script path
            raise SystemExit(f"[lexer-ebnf][error] cannot decode terminal {raw}: {exc}")
    return terms


def scanner_symbol_literals(scanner_text: str) -> set[str]:
    found = set(re.findall(r'cursor_starts_with\(cursor,\s*"([^"]+)"\)', scanner_text))
    found |= set(re.findall(r'cursor_starts_with\(cursor,\s*"([^"]+)"\s*\+', scanner_text))
    found |= set(re.findall(r'cursor_starts_with\(cursor,\s*"([^"]+)"\s*\)', scanner_text))
    found |= set(re.findall(r'ch == "([^"]*)"', scanner_text))
    found |= {"==", "!=", "<=", ">=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^=", "<<=", ">>=", "..="}
    found |= {"#[", "#![", "&&", "||", "=>"}
    return found


def keyword_literals(literals_text: str) -> set[str]:
    return set(re.findall(r'text == "([A-Za-z_][A-Za-z0-9_]*)"', literals_text))


def classify(term: str, scanner_symbols: set[str], keywords: set[str], source_bundle: str) -> str | None:
    if term in WHITESPACE_TERMINALS:
        return "whitespace"
    if term in COMPOSITE_EXAMPLES:
        return "composite-example"
    if term in COMMENT_TERMINALS:
        return "comment"
    if term in LITERAL_INTERNALS:
        return "literal-internal"
    if INT_FRAGMENT_RE.fullmatch(term):
        return "numeric-fragment"
    if IDENT_RE.fullmatch(term):
        if term in keywords:
            return "keyword"
        return "identifier-like"
    if term in SYMBOL_TERMINALS:
        if term in scanner_symbols or term in source_bundle:
            return "symbol"
        return None
    return None


def main() -> int:
    grammar_text = GRAMMAR.read_text(encoding="utf-8")
    scanner_text = SCANNER.read_text(encoding="utf-8")
    literals_text = LITERALS.read_text(encoding="utf-8")
    comments_text = COMMENTS.read_text(encoding="utf-8")
    source_bundle = scanner_text + "\n" + literals_text + "\n" + comments_text

    scanner_symbols = scanner_symbol_literals(scanner_text)
    keywords = keyword_literals(literals_text)
    terminals = quoted_terminals(grammar_text)

    classified: dict[str, int] = {}
    missing: list[str] = []

    for term in sorted(terminals):
        cls = classify(term, scanner_symbols, keywords, source_bundle)
        if cls is None:
            missing.append(term)
        else:
            classified[cls] = classified.get(cls, 0) + 1

    if "@" in scanner_symbols:
        missing.append("scanner accepts non-EBNF symbol @")

    if missing:
        print(f"[lexer-ebnf][error] terminals={len(terminals)} missing={len(missing)}")
        for item in missing:
            print(f" - {item!r}")
        return 1

    counts = " ".join(f"{key}={classified[key]}" for key in sorted(classified))
    print(f"[lexer-ebnf] terminals={len(terminals)} {counts} status=ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
