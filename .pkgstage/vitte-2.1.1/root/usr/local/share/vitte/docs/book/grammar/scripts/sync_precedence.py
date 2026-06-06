#!/usr/bin/env python3
"""Sync/check precedence table artifact generated from Vitte grammar metadata."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


ROWS: list[dict[str, object]] = [
    {"token": "Equal", "lexeme": "=", "op": "Assign", "precedence": 1, "core": True, "assoc": "right"},
    {"token": "PlusEqual", "lexeme": "+=", "op": "Assign", "precedence": 1, "core": True, "assoc": "right"},
    {"token": "MinusEqual", "lexeme": "-=", "op": "Assign", "precedence": 1, "core": True, "assoc": "right"},
    {"token": "StarEqual", "lexeme": "*=", "op": "Assign", "precedence": 1, "core": True, "assoc": "right"},
    {"token": "SlashEqual", "lexeme": "/=", "op": "Assign", "precedence": 1, "core": True, "assoc": "right"},
    {"token": "PercentEqual", "lexeme": "%=", "op": "Assign", "precedence": 1, "core": True, "assoc": "right"},
    {"token": "KwOr", "lexeme": "or", "op": "Or", "precedence": 2, "core": True, "assoc": "left"},
    {"token": "PipePipe", "lexeme": "||", "op": "Or", "precedence": 2, "core": True, "assoc": "left"},
    {"token": "KwAnd", "lexeme": "and", "op": "And", "precedence": 3, "core": True, "assoc": "left"},
    {"token": "AmpAmp", "lexeme": "&&", "op": "And", "precedence": 3, "core": True, "assoc": "left"},
    {"token": "Pipe", "lexeme": "|", "op": "BitOr", "precedence": 4, "core": True, "assoc": "left"},
    {"token": "Caret", "lexeme": "^", "op": "BitXor", "precedence": 5, "core": True, "assoc": "left"},
    {"token": "Amp", "lexeme": "&", "op": "BitAnd", "precedence": 6, "core": True, "assoc": "left"},
    {"token": "EqEq", "lexeme": "==", "op": "Eq", "precedence": 7, "core": True, "assoc": "left"},
    {"token": "NotEq", "lexeme": "!=", "op": "Ne", "precedence": 7, "core": True, "assoc": "left"},
    {"token": "Lt", "lexeme": "<", "op": "Lt", "precedence": 8, "core": True, "assoc": "left"},
    {"token": "Le", "lexeme": "<=", "op": "Le", "precedence": 8, "core": True, "assoc": "left"},
    {"token": "Gt", "lexeme": ">", "op": "Gt", "precedence": 8, "core": True, "assoc": "left"},
    {"token": "Ge", "lexeme": ">=", "op": "Ge", "precedence": 8, "core": True, "assoc": "left"},
    {"token": "Shl", "lexeme": "<<", "op": "Shl", "precedence": 9, "core": True, "assoc": "left"},
    {"token": "Shr", "lexeme": ">>", "op": "Shr", "precedence": 9, "core": True, "assoc": "left"},
    {"token": "Plus", "lexeme": "+", "op": "Add", "precedence": 10, "core": True, "assoc": "left"},
    {"token": "Minus", "lexeme": "-", "op": "Sub", "precedence": 10, "core": True, "assoc": "left"},
    {"token": "Star", "lexeme": "*", "op": "Mul", "precedence": 11, "core": True, "assoc": "left"},
    {"token": "Slash", "lexeme": "/", "op": "Div", "precedence": 11, "core": True, "assoc": "left"},
    {"token": "Percent", "lexeme": "%", "op": "Mod", "precedence": 11, "core": True, "assoc": "left"},
]


def render_payload(rows: list[dict[str, object]]) -> str:
    payload = {
        "source": "docs/book/grammar/scripts/sync_precedence.py",
        "rows": rows,
    }
    return json.dumps(payload, ensure_ascii=False, indent=2) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="sync/check precedence table artifact")
    ap.add_argument("--check", action="store_true", help="fail if artifact is out of date")
    args = ap.parse_args()

    repo = Path(__file__).resolve().parents[4]
    target = repo / "docs/book/grammar/precedence_table.json"

    rows = ROWS
    payload = render_payload(rows)
    if args.check:
        current = target.read_text(encoding="utf-8") if target.exists() else ""
        if current != payload:
            print(f"[precedence] out of sync: {target}")
            return 1
        print("[precedence] OK")
        return 0

    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(payload, encoding="utf-8")
    print(f"[precedence] wrote {target}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
