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


KEYWORD_PRIORITY = (
    "proc", "const", "form", "pick", "use", "import", "export", "share",
    "space", "intrinsic", "query", "diagnostic", "compiler", "pass", "backend",
)
TOP_LEVEL_KEYWORDS = {
    "space", "proc", "const", "form", "pick", "use", "import", "export",
    "share", "intrinsic", "query", "diagnostic", "compiler", "pass", "backend",
}


def levenshtein(left: str, right: str) -> int:
    previous = list(range(len(right) + 1))
    for left_index, left_character in enumerate(left, 1):
        current = [left_index]
        for right_index, right_character in enumerate(right, 1):
            current.append(min(
                current[-1] + 1,
                previous[right_index] + 1,
                previous[right_index - 1] + (left_character != right_character),
            ))
        previous = current
    return previous[-1]


def nearest_keyword(token: str) -> str | None:
    candidates = sorted(
        (levenshtein(token, keyword), KEYWORD_PRIORITY.index(keyword), keyword)
        for keyword in TOP_LEVEL_KEYWORDS
    )
    distance, _, keyword = candidates[0]
    return keyword if distance <= 2 else None


def mask_non_code(source: str) -> str:
    output = list(source)
    index = 0
    state = "code"
    while index < len(source):
        if state == "code" and source.startswith("//", index):
            state = "line-comment"
        elif state == "code" and source.startswith("/*", index):
            state = "block-comment"
            output[index:index + 2] = "  "
            index += 2
            continue
        elif state == "code" and source[index] == '"':
            state = "string"
            output[index] = "S"
            index += 1
            continue
        if state == "line-comment":
            if source[index] == "\n":
                state = "code"
            else:
                output[index] = " "
        elif state == "block-comment":
            if source.startswith("*/", index):
                output[index:index + 2] = "  "
                index += 2
                state = "code"
                continue
            if source[index] != "\n":
                output[index] = " "
        elif state == "string":
            if source[index] == "\\" and index + 1 < len(source):
                output[index:index + 2] = "  "
                index += 2
                continue
            if source[index] == '"':
                state = "code"
            if source[index] != "\n":
                output[index] = " "
        index += 1
    return "".join(output)


def analyze_parser(source: str, file: str) -> list[dict[str, Any]]:
    diagnostics: list[dict[str, Any]] = []
    brace_depth = 0
    offset = 0
    for raw_line in source.splitlines(keepends=True):
        line = raw_line.strip()
        if line and not line.startswith(("//", "/*", "*", "*/")) and brace_depth == 0:
            proc_name = re.match(r"proc[ \t]+([^ \t(]+)", line)
            if proc_name and re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", proc_name.group(1)) is None:
                invalid_name = proc_name.group(1)
                start = offset + raw_line.index(invalid_name)
                diagnostics.append(diagnostic(
                    "PARSE_E_EXPECTED_TOKEN", "parser", file, source, start, start + len(invalid_name),
                    "procedure name must be an identifier",
                    helps=["use an identifier such as `main` after `proc`"],
                ))
            missing_return = re.match(r"proc[ \t]+[A-Za-z_][A-Za-z0-9_]*\([^)]*\)[ \t]*(\{)", line)
            if missing_return:
                brace_in_raw = raw_line.index("{", raw_line.index("proc"))
                insertion = offset + brace_in_raw
                diagnostics.append(diagnostic(
                    "PARSE_E_TYPE_EXPECTED", "parser", file, source, insertion, insertion + 1,
                    "procedure return type is missing",
                    helps=["add `->` followed by the procedure return type"],
                    suggestions=[{
                        "message": "insert an integer return type",
                        "replacement": "-> int ",
                        "span": span(file, source, insertion, insertion),
                        "applicability": "maybe-incorrect",
                    }],
                ))
            match = re.match(r"([A-Za-z_][A-Za-z0-9_]*)", line)
            if match and match.group(1) not in TOP_LEVEL_KEYWORDS:
                token = match.group(1)
                start = offset + raw_line.index(token)
                suggestions: list[dict[str, Any]] = []
                helps = ["start a valid top-level declaration"]
                replacement = nearest_keyword(token)
                if replacement is not None:
                    suggestions.append({
                        "message": f"replace `{token}` with `{replacement}`",
                        "replacement": replacement,
                        "span": span(file, source, start, start + len(token)),
                        "applicability": "machine-applicable",
                    })
                    helps = [f"did you mean `{replacement}`?"]
                diagnostics.append(diagnostic(
                    "PARSE_E_TOPLEVEL_DECL_EXPECTED", "parser", file, source, start, start + len(token),
                    f"unexpected top-level token `{token}`",
                    notes=["the bootstrap backend limitation must never mask this parser error"],
                    helps=helps,
                    suggestions=suggestions,
                ))
        code_line = raw_line.split("//", 1)[0]
        brace_depth = max(0, brace_depth + code_line.count("{") - code_line.count("}"))
        offset += len(raw_line)
    masked = mask_non_code(source)
    paren_stack: list[int] = []
    for index, character in enumerate(masked):
        if character == "(":
            paren_stack.append(index)
        elif character == ")" and paren_stack:
            paren_stack.pop()
    for opening in paren_stack:
        line_end = masked.find("\n", opening)
        if line_end < 0:
            line_end = len(masked)
        arrow = masked.find("->", opening, line_end)
        insertion = arrow if arrow >= 0 else line_end
        diagnostics.append(diagnostic(
            "PARSE_E_MISSING_RPAREN", "parser", file, source, opening, opening + 1,
            "opening parenthesis is not closed",
            helps=["add the missing closing parenthesis"],
            suggestions=[{
                "message": "insert `)`",
                "replacement": ")",
                "span": span(file, source, insertion, insertion),
                "applicability": "machine-applicable",
            }],
        ))
    brace_stack: list[int] = []
    for index, character in enumerate(masked):
        if character == "{":
            brace_stack.append(index)
        elif character == "}" and brace_stack:
            brace_stack.pop()
    for opening in brace_stack:
        diagnostics.append(diagnostic(
            "PARSE_E_UNCLOSED_BLOCK", "parser", file, source, opening, opening + 1,
            "block opened here is not closed",
            helps=["add the missing closing brace"],
            suggestions=[{
                "message": "insert `}` at end of input",
                "replacement": "}",
                "span": span(file, source, len(source), len(source)),
                "applicability": "machine-applicable",
            }],
        ))
    for match in re.finditer(r":[ \t]*[A-Za-z_][A-Za-z0-9_<>\[\]]*[ \t]+([A-Za-z_][A-Za-z0-9_]*)[ \t]*:", masked):
        parameter_start = match.start(1)
        diagnostics.append(diagnostic(
            "PARSE_E_MISSING_COMMA", "parser", file, source, parameter_start, match.end(1),
            "missing comma before this parameter",
            helps=["separate adjacent parameters with a comma"],
            suggestions=[{
                "message": "insert a comma before the parameter",
                "replacement": ", ",
                "span": span(file, source, parameter_start, parameter_start),
                "applicability": "machine-applicable",
            }],
        ))
    for match in re.finditer(r";[ \t]*;", masked):
        semicolon = match.end() - 1
        diagnostics.append(diagnostic(
            "PARSE_E_UNEXPECTED_TOKEN", "parser", file, source, semicolon, semicolon + 1,
            "unexpected extra semicolon",
            helps=["remove the duplicate semicolon"],
            suggestions=[{
                "message": "remove this semicolon",
                "replacement": "",
                "span": span(file, source, semicolon, semicolon + 1),
                "applicability": "machine-applicable",
            }],
        ))
    for match in re.finditer(r"(=|[+\-*/])[ \t]*(?=;)", masked):
        operator = match.start(1)
        insertion = match.end()
        diagnostics.append(diagnostic(
            "PARSE_E_INCOMPLETE_EXPR", "parser", file, source, operator, operator + 1,
            "operator has no right-hand operand",
            helps=["complete the expression with the missing operand"],
            suggestions=[{
                "message": "insert an expression placeholder",
                "replacement": "0",
                "span": span(file, source, insertion, insertion),
                "applicability": "has-placeholders",
            }],
        ))
    return diagnostics


def analyze(source: str, file: str) -> list[dict[str, Any]]:
    lexer_diagnostics = analyze_lexer(source, file)
    if any(value["code"] in {"LEX_E_UNTERMINATED_STRING", "LEX_E_UNTERMINATED_COMMENT"} for value in lexer_diagnostics):
        return lexer_diagnostics
    return lexer_diagnostics + analyze_parser(source, file)
