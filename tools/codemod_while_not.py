#!/usr/bin/env python3
"""Rewrite common `while !(...)` guards into positive equivalents.

Only rewrites simple comparison forms:
- !(a >= b) -> a < b
- !(a > b)  -> a <= b
- !(a <= b) -> a > b
- !(a < b)  -> a >= b
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys

PATTERNS = [
    (re.compile(r"while\s*!\(\s*([^()\n]+?)\s*>=\s*([^()\n]+?)\s*\)\s*\{"), "while {lhs} < {rhs} {{"),
    (re.compile(r"while\s*!\(\s*([^()\n]+?)\s*>\s*([^()\n]+?)\s*\)\s*\{"), "while {lhs} <= {rhs} {{"),
    (re.compile(r"while\s*!\(\s*([^()\n]+?)\s*<=\s*([^()\n]+?)\s*\)\s*\{"), "while {lhs} > {rhs} {{"),
    (re.compile(r"while\s*!\(\s*([^()\n]+?)\s*<\s*([^()\n]+?)\s*\)\s*\{"), "while {lhs} >= {rhs} {{"),
]


def rewrite_text(text: str) -> tuple[str, int]:
    total = 0
    out = text
    for regex, template in PATTERNS:
        def repl(m: re.Match[str]) -> str:
            nonlocal total
            total += 1
            lhs = m.group(1).strip()
            rhs = m.group(2).strip()
            return template.format(lhs=lhs, rhs=rhs)

        out = regex.sub(repl, out)
    return out, total


def iter_files(root: pathlib.Path) -> list[pathlib.Path]:
    return sorted(root.rglob("*.vit"))


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("root", nargs="?", default="src/vitte", help="root directory to rewrite")
    p.add_argument("--apply", action="store_true", help="write changes to disk")
    args = p.parse_args()

    root = pathlib.Path(args.root)
    if not root.exists():
        print(f"error: path not found: {root}", file=sys.stderr)
        return 2

    changed_files = 0
    total_rewrites = 0

    for file in iter_files(root):
        src = file.read_text(encoding="utf-8")
        dst, n = rewrite_text(src)
        if n == 0:
            continue
        changed_files += 1
        total_rewrites += n
        if args.apply:
            file.write_text(dst, encoding="utf-8")
        print(f"{file}: {n}")

    mode = "applied" if args.apply else "dry-run"
    print(f"{mode}: rewrites={total_rewrites} files={changed_files}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
