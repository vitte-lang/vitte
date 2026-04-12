#!/usr/bin/env python3
"""Add `_pkg` suffix to simple `use ... as alias` statements when missing.

This codemod is intentionally conservative:
- rewrites only one-line `use ... as <alias>` forms
- skips aliases already ending in `_pkg`
- does not touch `pull`/`share`
"""

from __future__ import annotations

import argparse
import pathlib
import re

USE_ALIAS_RE = re.compile(r"^(\s*use\s+[^\n#]+?\s+as\s+)([A-Za-z_][A-Za-z0-9_]*)(\s*(?:#.*)?)$")


def rewrite_text(text: str) -> tuple[str, int]:
    rewrites = 0
    out_lines: list[str] = []
    for line in text.splitlines(keepends=True):
        raw = line[:-1] if line.endswith("\n") else line
        m = USE_ALIAS_RE.match(raw)
        if not m:
            out_lines.append(line)
            continue
        head, alias, tail = m.groups()
        if alias.endswith("_pkg"):
            out_lines.append(line)
            continue
        rewrites += 1
        nl = "\n" if line.endswith("\n") else ""
        out_lines.append(f"{head}{alias}_pkg{tail}{nl}")
    return "".join(out_lines), rewrites


def iter_files(root: pathlib.Path) -> list[pathlib.Path]:
    return sorted(root.rglob("*.vit"))


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("root", nargs="?", default="src/vitte", help="root directory to scan")
    ap.add_argument("--apply", action="store_true", help="write rewrites")
    args = ap.parse_args()

    root = pathlib.Path(args.root)
    changed_files = 0
    total = 0

    for path in iter_files(root):
        src = path.read_text(encoding="utf-8")
        dst, n = rewrite_text(src)
        if n == 0:
            continue
        changed_files += 1
        total += n
        if args.apply:
            path.write_text(dst, encoding="utf-8")
        print(f"{path}: {n}")

    mode = "applied" if args.apply else "dry-run"
    print(f"{mode}: rewrites={total} files={changed_files}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
