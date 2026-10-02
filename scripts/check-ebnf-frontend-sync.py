#!/usr/bin/env python3
"""Fail when the canonical EBNF, lexer tables, and parser drift apart."""

from __future__ import annotations

import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
GRAMMAR = ROOT / "grammar" / "vitte.ebnf"
LEXER = ROOT / "src" / "src" / "lexer" / "lexer.c"
PARSER = ROOT / "src" / "src" / "parser" / "parser.c"


def without_comments(text: str) -> str:
    return re.sub(r"\(\*.*?\*\)", "", text, flags=re.DOTALL)


def main() -> int:
    grammar = without_comments(GRAMMAR.read_text(encoding="utf-8"))
    lexer = LEXER.read_text(encoding="utf-8")
    parser = PARSER.read_text(encoding="utf-8")

    terminals = set(re.findall(r'"([^"\\]*(?:\\.[^"\\]*)*)"', grammar))
    keywords = re.findall(r'VITTE_KW\("([^"]+)",\s*(VITTE_TOKEN_[A-Z0-9_]+)\)', lexer)
    operators = re.findall(r'VITTE_OP\("([^"\\]+)",\s*(VITTE_TOKEN_[A-Z0-9_]+)\)', lexer)

    errors: list[str] = []
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
        f"lexer operator missing from EBNF: {text}"
        for text in sorted(set(operator_texts) - terminals)
    )

    for text, token_kind in keywords + operators:
        if token_kind not in parser:
            errors.append(f"lexer token is not consumed by parser: {token_kind} ({text})")

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
