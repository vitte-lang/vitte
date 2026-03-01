#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GRAMMAR = ROOT / "editors/grammar/vitte_highlight_grammar.json"
SNAP_DIR = ROOT / "tests/editors/highlights"


def load_text(editor: str) -> str:
    mapping = {
        "vim": ROOT / "editors/vim/syntax/vitte.vim",
        "emacs": ROOT / "editors/emacs/vitte-mode.el",
        "nano": ROOT / "editors/nano/vitte.nanorc",
    }
    p = mapping[editor]
    return p.read_text(encoding="utf-8", errors="ignore")


def build_snapshot(editor: str) -> str:
    g = json.loads(GRAMMAR.read_text(encoding="utf-8"))
    txt = load_text(editor)

    lines = [f"editor={editor}"]
    for group, words in [
        ("keywords", g["keywords"]),
        ("decl_keywords", g["decl_keywords"]),
        ("types", g["types"]),
        ("builtins", g["builtins"]),
    ]:
        hit = sum(1 for w in words if re.search(rf"\b{re.escape(w)}\b", txt))
        lines.append(f"{group}:{hit}/{len(words)}")

    strict = g["strict_patterns"]
    strict_checks = {
        "diag_code": "VITTE-" in txt,
        "alias_pkg": "_pkg" in txt,
        "contract_block": "ROLE-CONTRACT" in txt,
        "api_prefix_diagnostics": strict["api_prefixes"][0] in txt,
        "api_prefix_quickfix": strict["api_prefixes"][1] in txt,
        "api_prefix_doctor": strict["api_prefixes"][2] in txt,
    }
    for k, ok in strict_checks.items():
        lines.append(f"strict.{k}={'yes' if ok else 'no'}")

    lines.append(f"length={len(txt)}")
    return "\n".join(lines) + "\n"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--editor", choices=["vim", "emacs", "nano"], required=True)
    ap.add_argument("--update", action="store_true")
    args = ap.parse_args()

    SNAP_DIR.mkdir(parents=True, exist_ok=True)
    expected = SNAP_DIR / f"{args.editor}.highlight.must"
    current = build_snapshot(args.editor)

    if args.update or not expected.exists():
        expected.write_text(current, encoding="utf-8")
        print(f"[highlights-snapshot] updated {expected}")
        return 0

    old = expected.read_text(encoding="utf-8")
    if old != current:
        print(f"[highlights-snapshot][error] mismatch for {args.editor}: {expected}")
        print("[highlights-snapshot][error] run with --update to refresh")
        return 1

    out = ROOT / "target/reports" / f"highlight_{args.editor}.snapshot"
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(current, encoding="utf-8")
    print(f"[highlights-snapshot] OK {args.editor}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
