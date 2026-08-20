#!/usr/bin/env python3
from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOKEN = ROOT / "src/vitte/compiler/frontend/lexer/token.vit"
SCANNER = ROOT / "src/vitte/compiler/frontend/lexer/scanner.vit"
LITERALS = ROOT / "src/vitte/compiler/frontend/lexer/literals.vit"
LEXER_TESTS = ROOT / "src/vitte/compiler/tests/lexer_tests.vit"
REPORT_DIR = ROOT / "target/reports"

EXPECTED_TOKEN_KINDS = [
    "Eof",
    "Ident",
    "Keyword",
    "String",
    "Integer",
    "Float",
    "Char",
    "Symbol",
    "Invalid",
]

EXPECTED_SYMBOLS = [
    "(", ")", "[", "]", "{", "}", ",", ";", ":", "::", ".", "..", "...", "..=", "->", "=>",
    "+", "+=", "-", "-=", "*", "*=", "/", "/=", "%", "%=",
    "&", "&=", "&&", "|", "|=", "||", "^", "^=", "!", "!=", "=",
    "==", "<", "<=", "<<", "<<=", ">", ">=", ">>", ">>=", "?", "??", "~",
]

EXPECTED_KEYWORDS = [
    "space", "use", "export", "const", "static", "global", "region", "type",
    "opaque", "extern", "form", "class", "union", "bits", "pick", "flags",
    "trait", "impl", "proc", "intrinsic", "compiler", "query", "pass",
    "backend", "diagnostic", "macro", "comptime", "static_assert", "test",
    "bench", "entry", "pub", "priv", "unsafe", "async", "inline", "noinline",
    "naked", "interrupt", "where", "effects", "requires", "operator", "as",
    "for", "at", "let", "set", "give", "try", "defer", "asm", "emit",
    "assert", "panic", "unreachable", "if", "elif", "else", "while", "loop",
    "in", "break", "continue", "select", "when", "match", "case", "with",
    "critical", "mut", "owned", "borrow", "move", "await", "not", "and",
    "or", "is", "self", "ref", "dyn", "true", "false", "null", "sizeof",
    "alignof", "offsetof", "typeof", "nameof", "map", "resource", "volatile",
    "atomic", "user", "kernel", "phys", "mmio", "dma", "noexcept",
]


def extract_pick_members(text: str, name: str) -> list[str]:
    match = re.search(rf"pick\s+{re.escape(name)}\s*\{{(?P<body>.*?)\}}", text, re.S)
    if not match:
        raise SystemExit(f"[lexer-token-stability][error] missing pick {name}")
    members: list[str] = []
    for raw in match.group("body").splitlines():
        item = raw.strip().rstrip(",")
        if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", item):
            members.append(item)
    return members


def function_body(text: str, signature: str) -> str:
    start = text.find(signature)
    if start < 0:
        raise SystemExit(f"[lexer-token-stability][error] missing {signature}")
    brace = text.find("{", start)
    if brace < 0:
        raise SystemExit(f"[lexer-token-stability][error] malformed {signature}")
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[brace + 1:index]
    raise SystemExit(f"[lexer-token-stability][error] unterminated {signature}")


def check_order(text: str, first: str, second: str, label: str) -> str | None:
    first_index = text.find(first)
    second_index = text.find(second)
    if first_index < 0 or second_index < 0:
        return f"{label}: missing required branch"
    if first_index > second_index:
        return f"{label}: `{first}` must appear before `{second}`"
    return None


def main() -> int:
    token_text = TOKEN.read_text(encoding="utf-8")
    scanner_text = SCANNER.read_text(encoding="utf-8")
    literals_text = LITERALS.read_text(encoding="utf-8")
    tests_text = LEXER_TESTS.read_text(encoding="utf-8")

    failures: list[str] = []

    token_kinds = extract_pick_members(token_text, "TokenKind")
    if token_kinds != EXPECTED_TOKEN_KINDS:
        failures.append(
            f"{TOKEN.relative_to(ROOT)}: TokenKind changed from canonical compact surface {EXPECTED_TOKEN_KINDS} to {token_kinds}"
        )

    literal_kinds = extract_pick_members(literals_text, "LiteralKind")
    for required in ("Integer", "Float", "String", "RawString", "Char", "Bool", "Null", "Identifier", "Keyword"):
        if required not in literal_kinds:
            failures.append(f"{LITERALS.relative_to(ROOT)}: LiteralKind.{required} missing")

    for required in (
        "proc token_span_for_file",
        "proc token_with_file_offset",
        "proc token_with_file",
        "proc token_kind_is_literal",
    ):
        if required not in token_text:
            failures.append(f"{TOKEN.relative_to(ROOT)}: missing `{required}`")

    for required in ("proc scan_tokens", "proc scan_tokens_from_file", "proc scanner_selftest"):
        if required not in scanner_text:
            failures.append(f"{SCANNER.relative_to(ROOT)}: missing `{required}`")

    scanner_bundle = scanner_text + "\n" + tests_text
    for symbol in EXPECTED_SYMBOLS:
        if symbol not in scanner_bundle:
            failures.append(f"{SCANNER.relative_to(ROOT)}: missing stable symbol token `{symbol}`")

    for keyword in EXPECTED_KEYWORDS:
        if f'text == "{keyword}"' not in literals_text:
            failures.append(f"{LITERALS.relative_to(ROOT)}: missing stable keyword `{keyword}`")

    keyword_body = function_body(literals_text, "proc keyword_kind")
    order_failure = check_order(keyword_body, 'text == "true"', "is_keyword(text)", "keyword_kind")
    if order_failure:
        failures.append(f"{LITERALS.relative_to(ROOT)}: {order_failure}")
    order_failure = check_order(keyword_body, 'text == "null"', "is_keyword(text)", "keyword_kind")
    if order_failure:
        failures.append(f"{LITERALS.relative_to(ROOT)}: {order_failure}")

    for assertion in (
        'keyword_kind("true") == LiteralKind.Bool',
        'keyword_kind("false") == LiteralKind.Bool',
        'keyword_kind("null") == LiteralKind.Null',
    ):
        if assertion not in literals_text:
            failures.append(f"{LITERALS.relative_to(ROOT)}: literals_selftest must assert `{assertion}`")

    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    manifest = {
        "expected_symbols": len(EXPECTED_SYMBOLS),
        "expected_keywords": len(EXPECTED_KEYWORDS),
        "failures": failures,
        "literal_kinds": literal_kinds,
        "status": "ok" if not failures else "error",
        "token_kinds": token_kinds,
    }
    (REPORT_DIR / "lexer_token_stability.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    (REPORT_DIR / "lexer_token_stability.md").write_text(
        "\n".join(
            [
                "# Lexer Token Stability",
                "",
                f"- token kinds: {len(token_kinds)}",
                f"- literal kinds: {len(literal_kinds)}",
                f"- expected symbols: {len(EXPECTED_SYMBOLS)}",
                f"- expected keywords: {len(EXPECTED_KEYWORDS)}",
                f"- status: {manifest['status']}",
                "",
            ]
        ),
        encoding="utf-8",
    )

    if failures:
        print(f"[lexer-token-stability][error] failures={len(failures)}")
        for failure in failures:
            print(f" - {failure}")
        return 1

    print(
        "[lexer-token-stability] "
        f"token_kinds={len(token_kinds)} symbols={len(EXPECTED_SYMBOLS)} keywords={len(EXPECTED_KEYWORDS)} status=ok"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
