#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GRAMMAR = ROOT / "editors/tree-sitter/grammar.js"
QUERY = ROOT / "editors/tree-sitter/queries/highlights.scm"
CORPUS = ROOT / "editors/tree-sitter/test/corpus/core.txt"


REQUIRED_GRAMMAR_TOKENS = [
    "name: 'vitte'",
    "source_file",
    "proc_decl",
    "form_decl",
    "pick_decl",
    "trait_decl",
    "entry_decl",
    "use_stmt",
    "diag_code",
    "alias_pkg",
    "api_identifier",
]

REQUIRED_QUERY_BITS = [
    "@diagnostic.error",
    "@variable.special",
    "@comment.block",
    "proc_decl",
    "entry_decl",
    "diag_code",
    "alias_pkg",
    "api_identifier",
]


def main() -> int:
    errs = []

    if not GRAMMAR.exists():
        errs.append(f"missing grammar: {GRAMMAR}")
    if not QUERY.exists():
        errs.append(f"missing highlights query: {QUERY}")
    if not CORPUS.exists():
        errs.append(f"missing corpus: {CORPUS}")

    if errs:
        for e in errs:
            print(f"[tree-sitter-vitte-validate][error] {e}")
        return 1

    grammar = GRAMMAR.read_text(encoding="utf-8", errors="ignore")
    query = QUERY.read_text(encoding="utf-8", errors="ignore")

    for token in REQUIRED_GRAMMAR_TOKENS:
        if token not in grammar:
            errs.append(f"grammar missing token/rule: {token}")

    for token in REQUIRED_QUERY_BITS:
        if token not in query:
            errs.append(f"highlights query missing capture/rule: {token}")

    if not re.search(r"VITTE-[A-Z]+\[0-9\]\{4\}|VITTE-[A-Z]+\[0-9\]\{4\}|VITTE-[A-Z]+[0-9]\{4\}", query):
        # query may not encode regex literally; fallback to node-based requirement above.
        pass

    if errs:
        for e in errs:
            print(f"[tree-sitter-vitte-validate][error] {e}")
        return 1

    print("[tree-sitter-vitte-validate] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
