#!/usr/bin/env python3
"""Generate the source-level export index for the Vitte core package."""

from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "src"
OUTPUT = ROOT / "API.md"


def matching_brace(source: str, opening: int) -> int:
    depth = 0
    quote: str | None = None
    escaped = False
    line_comment = False
    block_comment = False
    index = opening

    while index < len(source):
        char = source[index]
        next_char = source[index + 1] if index + 1 < len(source) else ""

        if line_comment:
            if char == "\n":
                line_comment = False
            index += 1
            continue

        if block_comment:
            if char == "*" and next_char == "/":
                block_comment = False
                index += 2
            else:
                index += 1
            continue

        if quote is not None:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = None
            index += 1
            continue

        if char == "/" and next_char == "/":
            line_comment = True
            index += 2
            continue
        if char == "/" and next_char == "*":
            block_comment = True
            index += 2
            continue
        if char in ("\"", "'"):
            quote = char
            index += 1
            continue
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return index + 1
        index += 1

    raise ValueError("unclosed exported declaration")


def declaration_end(source: str, start: int, kind: str) -> tuple[int, bool]:
    if kind in {"form", "pick", "trait"}:
        opening = source.find("{", start)
        if opening < 0:
            raise ValueError("exported block declaration has no body")
        return matching_brace(source, opening), False

    if kind == "proc":
        round_depth = 0
        square_depth = 0
        angle_depth = 0
        index = start
        while index < len(source):
            char = source[index]
            if char == "(":
                round_depth += 1
            elif char == ")":
                round_depth -= 1
            elif char == "[":
                square_depth += 1
            elif char == "]":
                square_depth -= 1
            elif char == "<":
                angle_depth += 1
            elif char == ">" and index > 0 and source[index - 1] != "-":
                angle_depth = max(0, angle_depth - 1)
            elif round_depth == 0 and square_depth == 0 and angle_depth == 0:
                if char == ";":
                    return index + 1, False
                if char == "{":
                    return index, True
            index += 1
        raise ValueError("exported procedure has no body or terminator")

    if kind in {"type", "const", "intrinsic"}:
        if kind == "type" and "=" in source[start : source.find("\n", start) if "\n" in source[start:] else len(source)]:
            # A fixed-array alias can contain a semicolon inside its brackets.
            line_end = source.find("\n", start)
            return (line_end if line_end >= 0 else len(source)), False
        semicolon = source.find(";", start)
        if semicolon >= 0:
            return semicolon + 1, False

    line_end = source.find("\n", start)
    return (line_end if line_end >= 0 else len(source)), False


def exported_declarations(source: str) -> list[str]:
    declarations: list[str] = []
    for match in re.finditer(r"(?m)^\s*export\s+(\w+)\b", source):
        kind = match.group(1)
        end, prototype = declaration_end(source, match.start(), kind)
        snippet = source[match.start() : end].strip()
        if prototype:
            snippet = snippet.rstrip() + ";"
        declarations.append(snippet)
    return declarations


def source_module(source: str) -> str:
    match = re.search(r"(?m)^\s*space\s+([A-Za-z_][\w:]*)", source)
    if not match:
        raise ValueError("source file has exports but no module declaration")
    return match.group(1)


def main() -> None:
    modules: list[tuple[str, Path, list[str]]] = []
    for path in sorted(SOURCE_ROOT.rglob("*")):
        if path.suffix not in {".vit", ".vitte"}:
            continue
        source = path.read_text(encoding="utf-8")
        declarations = exported_declarations(source)
        if declarations:
            modules.append((source_module(source), path.relative_to(ROOT), declarations))

    lines = [
        "# `core` public API reference",
        "",
        "This file is generated from exported declarations under `src/`.",
        "Run `python3 tools/generate_api_reference.py` from `modules/core` after",
        "changing an exported signature. Contracts and error semantics are described",
        "in [`README.md`](README.md). This is a source API index, not a stable ABI",
        "promise.",
        "",
        f"Source files indexed: {len(modules)}.",
        f"Exported declarations indexed: {sum(len(declarations) for _, _, declarations in modules)}.",
        "",
    ]

    for module, path, declarations in modules:
        lines.extend(
            [
                f"## `{module}`",
                "",
                f"Source: [`{path.as_posix()}`]({path.as_posix()}) · exports: {len(declarations)}",
                "",
                "```vit",
                *declarations,
                "```",
                "",
            ]
        )

    OUTPUT.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
