#!/usr/bin/env python3
"""Emit canonical frontend diagnostics for a Vitte source file."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from diagnostic_frontend import analyze


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    args = parser.parse_args()
    source = args.source.read_text(encoding="utf-8")
    print(json.dumps(analyze(source, str(args.source)), indent=2, ensure_ascii=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
