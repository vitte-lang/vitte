#!/usr/bin/env python3
from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))

from parser_sync_coverage_report import build_report  # noqa: E402


def main() -> int:
    report = build_report(ROOT)
    rules = report["rules"]
    parsed = [
        item
        for item in rules
        if item["status"]["parsed"] and not item["status"]["lexical_rule"]
    ]
    gaps = [
        item
        for item in parsed
        if not item["status"]["ast_built"]
    ]

    if gaps:
        print("[ast-coverage][error] parsed non-lexical grammar rules without AST evidence:")
        for item in gaps:
            print(f"  - {item['rule']}")
        return 1

    lexical_skipped = [
        item
        for item in rules
        if item["status"]["lexical_rule"] and item["status"]["parsed"] and not item["status"]["ast_built"]
    ]
    print(
        "[ast-coverage] "
        f"parsed_nonlex={len(parsed)} "
        f"parsed_without_ast=0 "
        f"lexical_skipped={len(lexical_skipped)} "
        "status=green"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
