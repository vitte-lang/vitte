#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

PAT = re.compile(r"^\s*(pick|form|proc)\s+([A-Za-z_][A-Za-z0-9_]*)")


def build_index(root: Path, out: Path) -> None:
    rows: list[dict] = []
    for p in sorted((root / "src/vitte/packages/std").rglob("*.vit")):
        rel = p.relative_to(root).as_posix()
        for i, line in enumerate(p.read_text(encoding="utf-8").splitlines(), start=1):
            m = PAT.match(line)
            if not m:
                continue
            rows.append({"kind": m.group(1), "symbol": m.group(2), "file": rel, "line": i})
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps({"version": 1, "rows": rows}, indent=2) + "\n", encoding="utf-8")


def query_index(index: Path, q: str) -> list[dict]:
    payload = json.loads(index.read_text(encoding="utf-8"))
    rows = payload.get("rows", [])
    terms = {"sym": "", "kind": "", "module": ""}
    for tok in q.split():
        if ":" in tok:
            k, v = tok.split(":", 1)
            if k in terms:
                terms[k] = v.lower()
        else:
            terms["sym"] = tok.lower()
    out: list[dict] = []
    for r in rows:
        sym = str(r.get("symbol", "")).lower()
        kind = str(r.get("kind", "")).lower()
        mod = str(r.get("file", "")).lower()
        if terms["sym"] and terms["sym"] not in sym:
            continue
        if terms["kind"] and terms["kind"] != kind:
            continue
        if terms["module"] and terms["module"] not in mod:
            continue
        out.append(r)
    return out


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--index", action="store_true")
    ap.add_argument("--query", default="")
    ap.add_argument("--repo", default=".")
    args = ap.parse_args()

    repo = Path(args.repo).resolve()
    idx = repo / ".vitte-cache/vitte-ide-gtk/std.symbol.index.json"

    if args.index:
        build_index(repo, idx)
        print(f"[std-symbol-search] indexed -> {idx}")
        return 0

    if not idx.exists():
        build_index(repo, idx)

    if not args.query.strip():
        print("[std-symbol-search] empty query")
        return 0

    rows = query_index(idx, args.query)
    for r in rows[:200]:
        print(f"{r['file']}:{r['line']}:1:info:{r['kind']} {r['symbol']}")
    print(f"[std-symbol-search] hits={len(rows)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
