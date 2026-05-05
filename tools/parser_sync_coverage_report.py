#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


TOPLEVEL_RULES = {
    "space_decl": "space ",
    "use_decl": "use ",
    "export_decl": "export ",
    "const_decl": "const ",
    "static_decl": "static ",
    "global_decl": "global ",
    "proc_decl": "proc ",
}

STMT_RULES = {
    "give_stmt": "give ",
    "let_stmt": "let ",
    "set_stmt": "set ",
    "emit_stmt": "emit ",
}


def grammar_rules(text: str) -> set[str]:
    return set(re.findall(r"^([A-Za-z_][A-Za-z0-9_]*)\s*::=", text, re.M))


def parser_prefixes(text: str) -> set[str]:
    prefixes = set(re.findall(r'core\.string_starts_with\([^,]+,\s*"([^"]+)"\)', text))
    prefixes.update(re.findall(r'_starts_with\([^,]+,\s*"([^"]+)"\)', text))
    prefixes.update(re.findall(r'_is_keyword_decl\([^,]+,\s*"([^"]+)"\)', text))
    return prefixes


def main() -> int:
    parser = argparse.ArgumentParser(description="Vitte parser sync token coverage report")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    grammar_text = (repo / "src/vitte/grammar/vitte.ebnf").read_text(encoding="utf-8")
    parser_text = (
        (repo / "src/vitte/compiler/frontend/parser.vit").read_text(encoding="utf-8")
        + "\n"
        + (repo / "src/vitte/compiler/ir/ast.vit").read_text(encoding="utf-8")
    )

    rules = grammar_rules(grammar_text)
    prefixes = parser_prefixes(parser_text)

    covered_top = {
        rule: prefix
        for rule, prefix in TOPLEVEL_RULES.items()
        if rule in rules and prefix in prefixes
    }
    covered_stmt = {
        rule: prefix
        for rule, prefix in STMT_RULES.items()
        if rule in rules and prefix in prefixes
    }

    missing_top = sorted(rule for rule, prefix in TOPLEVEL_RULES.items() if rule in rules and prefix not in prefixes)
    missing_stmt = sorted(rule for rule, prefix in STMT_RULES.items() if rule in rules and prefix not in prefixes)

    report = {
        "source": "src/vitte/compiler/frontend/parser.vit",
        "grammar": "src/vitte/grammar/vitte.ebnf",
        "covered_toplevel_rules": covered_top,
        "covered_stmt_rules": covered_stmt,
        "missing_toplevel_rules": missing_top,
        "missing_stmt_rules": missing_stmt,
    }

    out = repo / "target/reports/parser_sync_coverage.json"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    if missing_top or missing_stmt:
        print("[parser-sync-coverage] FAILED")
        if missing_top:
            print(f"- missing top-level parser prefixes: {', '.join(missing_top)}")
        if missing_stmt:
            print(f"- missing statement parser prefixes: {', '.join(missing_stmt)}")
        return 1

    print(f"[parser-sync-coverage] OK ({out})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
