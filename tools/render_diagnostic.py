#!/usr/bin/env python3
"""Render a canonical Vitte diagnostic as stable terminal text."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def source_line(source_root: Path, file: str, line: int) -> str | None:
    path = Path(file)
    if not path.is_absolute():
        path = source_root / path
    try:
        lines = path.read_text(encoding="utf-8").splitlines()
    except (OSError, UnicodeError):
        return None
    return lines[line - 1] if 1 <= line <= len(lines) else None


def render(diagnostic: dict[str, Any], source_root: Path) -> str:
    span = diagnostic["primary_span"]
    start = span["start"]
    lines = [
        f'{diagnostic["severity"]}[{diagnostic["code"]}] {diagnostic["phase"]}: {diagnostic["message"]}',
        f'  --> {span["file"]}:{start["line"]}:{start["column"]}',
    ]
    text = source_line(source_root, span["file"], start["line"])
    if text is not None:
        width = len(str(start["line"]))
        gutter = f'{"":>{width}} |'
        marker_width = 1
        if span["end"]["line"] == start["line"]:
            marker_width = max(1, span["end"]["column"] - start["column"])
        marker = " " * max(0, start["column"] - 1) + "^" * marker_width
        lines.extend((gutter, f'{start["line"]:>{width}} | {text}', f"{gutter} {marker}"))
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("diagnostic", type=Path)
    parser.add_argument("--source-root", type=Path, default=Path.cwd())
    args = parser.parse_args()
    with args.diagnostic.open(encoding="utf-8") as stream:
        diagnostic = json.load(stream)
    if not isinstance(diagnostic, dict):
        raise SystemExit("diagnostic must be a JSON object")
    print(render(diagnostic, args.source_root), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
