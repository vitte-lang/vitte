"""Canonical lexer/parser diagnostics for source-level preflight checks."""

from __future__ import annotations

import re
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "locales/en/diagnostics.ftl"


def catalog_messages() -> dict[str, str]:
    messages: dict[str, str] = {}
    pattern = re.compile(r"^([A-Z][A-Z0-9_]*)\s*=\s*(.+)$")
    for line in CATALOG.read_text(encoding="utf-8").splitlines():
        match = pattern.match(line)
        if match:
            messages[match.group(1)] = match.group(2).strip()
    return messages


MESSAGES = catalog_messages()


def position(source: str, offset: int) -> dict[str, int]:
    prefix = source[:offset]
    line = prefix.count("\n") + 1
    line_start = prefix.rfind("\n") + 1
    return {
        "line": line,
        "column": offset - line_start + 1,
        "byte": len(prefix.encode("utf-8")),
    }


def span(file: str, source: str, start: int, end: int) -> dict[str, Any]:
    return {"file": file, "start": position(source, start), "end": position(source, end)}


def diagnostic(
    code: str,
    phase: str,
    file: str,
    source: str,
    start: int,
    end: int,
    label: str,
    *,
    notes: list[str] | None = None,
    helps: list[str] | None = None,
    suggestions: list[dict[str, Any]] | None = None,
) -> dict[str, Any]:
    if code not in MESSAGES:
        raise ValueError(f"diagnostic code is missing from Fluent: {code}")
    primary_span = span(file, source, start, end)
    return {
        "schema": "vitte.diagnostic",
        "schema_version": "1.0.0",
        "code": code,
        "severity": "error",
        "phase": phase,
        "message_key": code,
        "message": MESSAGES[code],
        "primary_span": primary_span,
        "labels": [{"kind": "primary", "message": label, "span": primary_span}],
        "notes": notes or [],
        "helps": helps or [],
        "suggestions": suggestions or [],
    }


def automatic_lexer_suggestion(value: dict[str, Any], source: str) -> dict[str, Any]:
    code = value["code"]
    primary_span = value["primary_span"]
    start_byte = primary_span["start"]["byte"]
    end_byte = primary_span["end"]["byte"]
    encoded = source.encode("utf-8")
    fragment = encoded[start_byte:end_byte].decode("utf-8", errors="replace")
    replacements = {
        "LEX_E_INVALID_CHAR": ("remove the invalid character", "", "machine-applicable"),
        "LEX_E_UNTERMINATED_STRING": ("terminate the string literal", fragment + '"', "machine-applicable"),
        "LEX_E_INVALID_CHAR_LITERAL": ("replace with a one-character literal", "'a'", "maybe-incorrect"),
        "LEX_E_INVALID_ESCAPE": ("replace with a valid newline escape", "\\n", "maybe-incorrect"),
        "LEX_E_INVALID_UNICODE": ("replace with the Unicode replacement character", "\\uFFFD", "maybe-incorrect"),
        "LEX_E_INVALID_NUMBER": ("separate the numeric and identifier parts", re.sub(r"(?<=\d)(?=[A-Za-z])", " ", fragment, count=1), "machine-applicable"),
        "LEX_E_TOKEN_TOO_LARGE": ("shorten the token to the lexer limit", fragment[:128], "maybe-incorrect"),
        "LEX_E_UNTERMINATED_COMMENT": ("terminate the block comment", fragment + "*/", "machine-applicable"),
    }
    message, replacement, applicability = replacements[code]
    return {
        "message": message,
        "replacement": replacement,
        "span": primary_span,
        "applicability": applicability,
    }


def analyze_lexer(source: str, file: str) -> list[dict[str, Any]]:
    diagnostics: list[dict[str, Any]] = []
    in_string = False
    string_start = -1
    escaped = False
    index = 0
    while index < len(source):
        character = source[index]
        if in_string:
            if escaped:
                escaped = False
            elif character == "\\":
                next_character = source[index + 1] if index + 1 < len(source) else ""
                if next_character not in {'"', "'", "\\", "0", "b", "f", "n", "r", "t", "u"}:
                    diagnostics.append(diagnostic(
                        "LEX_E_INVALID_ESCAPE", "lexer", file, source, index, min(len(source), index + 2),
                        f"unknown escape sequence `\\{next_character}`",
                        helps=["replace it with a supported escape such as `\\n`, `\\t`, `\\\"`, or `\\\\`"],
                    ))
                elif next_character == "u":
                    digits = source[index + 2:index + 6]
                    if len(digits) != 4 or re.fullmatch(r"[0-9A-Fa-f]{4}", digits) is None:
                        diagnostics.append(diagnostic(
                            "LEX_E_INVALID_UNICODE", "lexer", file, source, index, min(len(source), index + 6),
                            "unicode escape requires four hexadecimal digits",
                            helps=["use the form `\\u0041`"],
                        ))
                escaped = True
            elif character == '"':
                in_string = False
                string_start = -1
            elif character == "\n":
                diagnostics.append(diagnostic(
                    "LEX_E_UNTERMINATED_STRING", "lexer", file, source, string_start, index,
                    "string literal is not terminated",
                    helps=["add the missing closing quote before the end of the line"],
                ))
                in_string = False
                string_start = -1
            index += 1
            continue
        if source.startswith("//", index):
            newline = source.find("\n", index + 2)
            index = len(source) if newline < 0 else newline
            continue
        if source.startswith("/*", index):
            comment_start = index
            depth = 1
            index += 2
            while index < len(source) and depth > 0:
                if source.startswith("/*", index):
                    depth += 1
                    index += 2
                elif source.startswith("*/", index):
                    depth -= 1
                    index += 2
                else:
                    index += 1
            if depth > 0:
                diagnostics.append(diagnostic(
                    "LEX_E_UNTERMINATED_COMMENT", "lexer", file, source, comment_start, len(source),
                    "block comment is not terminated",
                    helps=["add the missing `*/` terminator"],
                ))
            continue
        if character == '"':
            in_string = True
            string_start = index
        elif character == "'":
            line_end = source.find("\n", index + 1)
            if line_end < 0:
                line_end = len(source)
            closing = source.find("'", index + 1, line_end)
            literal_end = closing + 1 if closing >= 0 else line_end
            content = source[index + 1:closing] if closing >= 0 else ""
            valid = closing >= 0 and (len(content) == 1 or (len(content) == 2 and content.startswith("\\")))
            if not valid:
                diagnostics.append(diagnostic(
                    "LEX_E_INVALID_CHAR_LITERAL", "lexer", file, source, index, max(index + 1, literal_end),
                    "character literal must contain exactly one character",
                    helps=["use one character between single quotes or use a string literal"],
                ))
            index = max(index, literal_end - 1)
        elif character == "@":
            diagnostics.append(diagnostic(
                "LEX_E_INVALID_CHAR", "lexer", file, source, index, index + 1,
                "invalid character `@`",
                helps=["remove `@` or replace it with valid Vitte syntax"],
            ))
        index += 1
    if in_string:
        diagnostics.append(diagnostic(
            "LEX_E_UNTERMINATED_STRING", "lexer", file, source, string_start, len(source),
            "string literal is not terminated",
            helps=["add the missing closing quote at the end of the string"],
        ))
    number_forms = (
        re.compile(r"[0-9][0-9_]*"),
        re.compile(r"0x[0-9A-Fa-f][0-9A-Fa-f_]*"),
        re.compile(r"0o[0-7][0-7_]*"),
        re.compile(r"0b[01][01_]*"),
    )
    for match in re.finditer(r"\b[0-9][A-Za-z0-9_]*\b", source):
        if any(pattern.fullmatch(match.group(0)) for pattern in number_forms):
            continue
        diagnostics.append(diagnostic(
            "LEX_E_INVALID_NUMBER", "lexer", file, source, match.start(), match.end(),
            f"invalid number literal `{match.group(0)}`",
            helps=["separate the number from the identifier or correct the numeric literal"],
        ))
    for match in re.finditer(r"\b[A-Za-z_][A-Za-z0-9_]{128,}\b", source):
        diagnostics.append(diagnostic(
            "LEX_E_TOKEN_TOO_LARGE", "lexer", file, source, match.start(), match.end(),
            f"token length {len(match.group(0))} exceeds the 128-byte lexer limit",
            helps=["split or shorten the identifier"],
        ))
    for value in diagnostics:
        if not value["suggestions"]:
            value["suggestions"].append(automatic_lexer_suggestion(value, source))
    return diagnostics


def analyze(source: str, file: str) -> list[dict[str, Any]]:
    return analyze_lexer(source, file)
