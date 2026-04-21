#!/usr/bin/env python3
"""Sync/check precedence table artifact generated from compiler source."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


ROW_RE = re.compile(
    r"^VITTE_PRECEDENCE_OP\(\s*([^,]+)\s*,\s*\"([^\"]+)\"\s*,\s*([^,]+)\s*,\s*([0-9]+)\s*,\s*(true|false)\s*\)\s*$"
)


def parse_rows(def_path: Path) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for raw in def_path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("//"):
            continue
        m = ROW_RE.match(line)
        if not m:
            raise ValueError(f"invalid precedence row: {line}")
        token, lexeme, op, prec, core = m.groups()
        rows.append(
            {
                "token": token,
                "lexeme": lexeme,
                "op": op,
                "precedence": int(prec),
                "core": core == "true",
                "assoc": "right" if token == "TokenKind::Equal" else "left",
            }
        )
    return rows


def render_payload(rows: list[dict[str, object]]) -> str:
    payload = {
        "source": "src/compiler/frontend/precedence_table.def",
        "rows": rows,
    }
    return json.dumps(payload, ensure_ascii=False, indent=2) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser(description="sync/check precedence table artifact")
    ap.add_argument("--check", action="store_true", help="fail if artifact is out of date")
    args = ap.parse_args()

    repo = Path(__file__).resolve().parents[4]
    source = repo / "src/compiler/frontend/precedence_table.def"
    target = repo / "docs/book/grammar/precedence_table.json"

    rows = parse_rows(source)
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
