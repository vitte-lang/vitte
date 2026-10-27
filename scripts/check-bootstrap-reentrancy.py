#!/usr/bin/env python3
"""Reject mutable file-scope state in the reentrant bootstrap API.

Function-local state is owned by a compilation call.  Immutable tables and
thread-local error/version caches are safe to share, so the check only flags
ordinary mutable objects at file scope in the API, driver and source manager.
This is intentionally a small lexical check: it catches the class of global
state that can make concurrent compilations interfere without pretending to be
a complete C parser.
"""

from __future__ import annotations

from pathlib import Path
import re
import sys


ROOTS = (
    Path("src/bootstrap/api"),
    Path("src/bootstrap/config"),
    Path("src/bootstrap/diagnostic"),
    Path("src/bootstrap/driver"),
    Path("src/bootstrap/import"),
    Path("src/bootstrap/module"),
    Path("src/bootstrap/source"),
    Path("src/bootstrap/unicode"),
)
STATIC_OBJECT = re.compile(r"\bstatic\b")


def strip_comments_and_strings(line: str, in_block_comment: bool) -> tuple[str, bool]:
    result: list[str] = []
    index = 0
    quote: str | None = None
    while index < len(line):
        if in_block_comment:
            end = line.find("*/", index)
            if end < 0:
                return "".join(result), True
            index = end + 2
            in_block_comment = False
            continue
        if quote is not None:
            if line[index] == "\\":
                index += 2
                continue
            if line[index] == quote:
                quote = None
            index += 1
            continue
        if line.startswith("/*", index):
            in_block_comment = True
            index += 2
            continue
        if line.startswith("//", index):
            break
        if line[index] in "\"'":
            quote = line[index]
            index += 1
            continue
        result.append(line[index])
        index += 1
    return "".join(result), in_block_comment


def scan(path: Path) -> list[str]:
    violations: list[str] = []
    depth = 0
    in_block_comment = False
    for number, original in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line, in_block_comment = strip_comments_and_strings(
            original, in_block_comment
        )
        if depth == 0 and STATIC_OBJECT.search(line):
            # Function definitions/prototypes contain a call delimiter.  A
            # thread-local object is deliberately isolated per caller thread.
            before_semicolon = line.split(";", 1)[0]
            if (
                "_Thread_local" not in before_semicolon
                and "const" not in before_semicolon.split("static", 1)[-1]
                and "(" not in before_semicolon
            ):
                if ";" in line or "=" in line:
                    violations.append(f"{path}:{number}: mutable static object")
        depth += line.count("{") - line.count("}")
        if depth < 0:
            depth = 0
    return violations


def main() -> int:
    violations: list[str] = []
    for root in ROOTS:
        for path in sorted(root.rglob("*.c")):
            violations.extend(scan(path))
    if violations:
        print("[reentrancy] mutable file-scope state found:", file=sys.stderr)
        print("\n".join(violations), file=sys.stderr)
        return 1
    print(
        "[reentrancy] OK: no mutable file-scope state in API/config/diagnostic/"
        "driver/import/module/source/unicode"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
