#!/usr/bin/env python3
"""Render a canonical Vitte diagnostic as stable terminal text."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


def render(diagnostic: dict[str, Any]) -> str:
    span = diagnostic["primary_span"]
    start = span["start"]
    return "\n".join((
        f'{diagnostic["severity"]}[{diagnostic["code"]}] {diagnostic["phase"]}: {diagnostic["message"]}',
        f'  --> {span["file"]}:{start["line"]}:{start["column"]}',
    )) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("diagnostic", type=Path)
    args = parser.parse_args()
    with args.diagnostic.open(encoding="utf-8") as stream:
        diagnostic = json.load(stream)
    if not isinstance(diagnostic, dict):
        raise SystemExit("diagnostic must be a JSON object")
    print(render(diagnostic), end="")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
