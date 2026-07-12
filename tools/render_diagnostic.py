#!/usr/bin/env python3
"""Render a canonical Vitte diagnostic as stable terminal text."""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any

ANSI = {
    "error": "\x1b[1;31m",
    "warning": "\x1b[1;33m",
    "note": "\x1b[1;34m",
    "help": "\x1b[1;36m",
    "suggestion": "\x1b[1;32m",
    "reset": "\x1b[0m",
}


def detect_color(stream: Any, environment: dict[str, str]) -> bool:
    return bool(stream.isatty()) and "NO_COLOR" not in environment and environment.get("TERM") != "dumb"


def styled(text: str, role: str, color: bool) -> str:
    return f'{ANSI[role]}{text}{ANSI["reset"]}' if color else text


def source_line(source_root: Path, file: str, line: int) -> str | None:
    path = Path(file)
    if not path.is_absolute():
        path = source_root / path
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError):
        return None
    return lines[line - 1] if 1 <= line <= len(lines) else None


def render(diagnostic: dict[str, Any], source_root: Path, color: bool = False) -> str:
    span = diagnostic["primary_span"]
    start = span["start"]
    lines = [
        styled(f'{diagnostic["severity"]}[{diagnostic["code"]}]', diagnostic["severity"], color)
        + f' {diagnostic["phase"]}: {diagnostic["message"]}',
        f'  --> {span["file"]}:{start["line"]}:{start["column"]}',
    ]
    text = source_line(source_root, span["file"], start["line"])
    primary_label = next((label for label in diagnostic["labels"] if label["kind"] == "primary"), None)
    if text is not None:
        width = len(str(start["line"]))
        gutter = f'{"":>{width}} |'
        marker_width = 1
        if span["end"]["line"] == start["line"]:
            marker_width = max(1, span["end"]["column"] - start["column"])
        marker = " " * max(0, start["column"] - 1) + "^" * marker_width
        if primary_label is not None:
            marker += f' {primary_label["message"]}'
        lines.extend((gutter, f'{start["line"]:>{width}} | {text}', f"{gutter} {marker}"))
    for label in diagnostic["labels"]:
        if label["kind"] != "secondary":
            continue
        secondary = label["span"]
        secondary_start = secondary["start"]
        lines.append(f'  ::: {secondary["file"]}:{secondary_start["line"]}:{secondary_start["column"]}')
        secondary_text = source_line(source_root, secondary["file"], secondary_start["line"])
        if secondary_text is None:
            continue
        width = len(str(secondary_start["line"]))
        gutter = f'{"":>{width}} |'
        marker_width = max(1, secondary["end"]["column"] - secondary_start["column"])
        marker = " " * max(0, secondary_start["column"] - 1) + "-" * marker_width
        lines.extend((
            gutter,
            f'{secondary_start["line"]:>{width}} | {secondary_text}',
            f'{gutter} {marker} {label["message"]}',
        ))
    for note in diagnostic["notes"]:
        lines.append(f'  = {styled("note", "note", color)}: {note}')
    for help_message in diagnostic["helps"]:
        lines.append(f'  = {styled("help", "help", color)}: {help_message}')
    for suggestion in diagnostic["suggestions"]:
        prefix = styled("suggestion", "suggestion", color)
        lines.append(f'  = {prefix}[{suggestion["applicability"]}]: {suggestion["message"]}')
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("diagnostic", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path.cwd())
    parser.add_argument("--color", choices=("auto", "always", "never"), default="auto")
    args = parser.parse_args()
    with args.diagnostic.open(encoding="utf-8") as stream:
        diagnostic = json.load(stream)
    if not isinstance(diagnostic, dict):
        raise SystemExit("diagnostic must be a JSON object")
    color = args.color == "always" or (args.color == "auto" and detect_color(sys.stdout, dict(os.environ)))
    print(render(diagnostic, args.source_root, color), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
