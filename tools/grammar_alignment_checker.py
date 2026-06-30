#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from parser_sync_coverage_report import main as coverage_main


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Compatibility wrapper for exhaustive grammar coverage reporting")
    parser.add_argument("--json", action="store_true", help="print the exhaustive coverage report as JSON")
    args = parser.parse_args(argv)

    forwarded = ["--check"]
    if args.json:
        forwarded.append("--json")

    print("[grammar-alignment] deprecated wrapper: using tools/parser_sync_coverage_report.py")
    return coverage_main(forwarded)


if __name__ == "__main__":
    raise SystemExit(main())
