#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


def extract_switch_tokens(text: str, fn_name: str) -> set[str]:
    fn_re = re.compile(rf"static bool {re.escape(fn_name)}\(TokenKind kind\)\s*\{{(.*?)\n\}}", re.S)
    m = fn_re.search(text)
    if not m:
        return set()
    body = m.group(1)
    return set(re.findall(r"case\s+TokenKind::([A-Za-z0-9_]+)\s*:", body))


def extract_parse_stmt_dispatch(text: str) -> set[str]:
    fn_re = re.compile(r"StmtId Parser::parse_stmt\(\)\s*\{(.*?)\n\}", re.S)
    m = fn_re.search(text)
    if not m:
        return set()
    body = m.group(1)
    return set(re.findall(r"case\s+TokenKind::([A-Za-z0-9_]+)\s*:\s*return parse_[a-z_]+\(.*?\);", body))


def extract_toplevel_dispatch(text: str) -> set[str]:
    fn_re = re.compile(r"DeclId Parser::parse_toplevel\(\)\s*\{(.*?)\n\}", re.S)
    m = fn_re.search(text)
    if not m:
        return set()
    body = m.group(1)
    return set(re.findall(r"current_\.kind\s*==\s*TokenKind::([A-Za-z0-9_]+)", body))


def main() -> int:
    ap = argparse.ArgumentParser(description="parser sync token coverage report")
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    repo = Path(__file__).resolve().parents[1]
    parser_expr = (repo / "src/compiler/frontend/parser_expr.inc").read_text(encoding="utf-8")
    parser_stmt = (repo / "src/compiler/frontend/parser_stmt.inc").read_text(encoding="utf-8")
    parser_toplevel = (repo / "src/compiler/frontend/parser_toplevel.inc").read_text(encoding="utf-8")

    stmt_sync = extract_switch_tokens(parser_expr, "is_stmt_sync_kind")
    top_sync = extract_switch_tokens(parser_expr, "is_toplevel_sync_kind")
    arm_sync = extract_switch_tokens(parser_expr, "is_match_arm_sync_kind")

    stmt_dispatch = extract_parse_stmt_dispatch(parser_stmt)
    top_dispatch = extract_toplevel_dispatch(parser_toplevel)

    missing_stmt = sorted(stmt_dispatch - stmt_sync)
    missing_top = sorted(top_dispatch - top_sync)

    report = {
        "stmt_dispatch": sorted(stmt_dispatch),
        "stmt_sync": sorted(stmt_sync),
        "missing_stmt_sync_tokens": missing_stmt,
        "toplevel_dispatch": sorted(top_dispatch),
        "toplevel_sync": sorted(top_sync),
        "missing_toplevel_sync_tokens": missing_top,
        "match_arm_sync": sorted(arm_sync),
    }

    out = repo / "target/reports/parser_sync_coverage.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    if missing_stmt or missing_top:
        print("[parser-sync-coverage] FAILED")
        if missing_stmt:
            print(f"- missing stmt sync tokens: {', '.join(missing_stmt)}")
        if missing_top:
            print(f"- missing toplevel sync tokens: {', '.join(missing_top)}")
        return 1

    print(f"[parser-sync-coverage] OK ({out})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
