#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

DECL_RE = re.compile(r"^\s*(proc|type|form|pick|const|let|make)\s+([A-Za-z_][A-Za-z0-9_]*)\b")
ENTRY_RE = re.compile(r"^\s*entry\b")


def module_name_from_path(path: Path, std_root: Path) -> str:
    rel = path.relative_to(std_root)
    if rel.name == "mod.vit":
        rel = rel.parent
    else:
        rel = rel.with_suffix("")
    return "std/" + rel.as_posix()


def parse_public_symbols(path: Path) -> set[str]:
    symbols: set[str] = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        if ENTRY_RE.match(raw):
            continue
        m = DECL_RE.match(raw)
        if m:
            symbols.add(m.group(2))
    return symbols


def load_surface(surface_path: Path) -> list[tuple[str, str, str]]:
    rows: list[tuple[str, str, str]] = []
    for ln in surface_path.read_text(encoding="utf-8").splitlines():
        line = ln.strip()
        if not line or line.startswith("#"):
            continue
        parts = [p.strip() for p in line.split("|")]
        if len(parts) != 4:
            raise ValueError(f"invalid surface row: {line}")
        version, profile, module, symbol = parts
        if version != "v1":
            raise ValueError(f"unsupported ABI surface version: {version}")
        rows.append((profile, module, symbol))
    return rows


def main() -> int:
    p = argparse.ArgumentParser(description="Lint stdlib API surface against source files")
    p.add_argument("--std-root", default="src/vitte/packages")
    p.add_argument("--surface", default="docs/stdlib_abi_surface_v1.txt")
    args = p.parse_args()

    std_root = Path(args.std_root)
    surface = Path(args.surface)
    if not std_root.exists():
        print(f"[stdlib-api-lint][error] missing std root: {std_root}", file=sys.stderr)
        return 1
    if not surface.exists():
        print(f"[stdlib-api-lint][error] missing ABI surface file: {surface}", file=sys.stderr)
        return 1

    found: dict[str, set[str]] = {}
    for src in sorted(std_root.rglob("*.vit")):
        mod = module_name_from_path(src, std_root)
        found.setdefault(mod, set()).update(parse_public_symbols(src))

    ok = True
    try:
        rows = load_surface(surface)
    except ValueError as exc:
        print(f"[stdlib-api-lint][error] {exc}", file=sys.stderr)
        return 1

    for profile, module, symbol in rows:
        if module not in found:
            print(f"[stdlib-api-lint][error] {profile}: missing module '{module}'", file=sys.stderr)
            ok = False
            continue
        if symbol not in found[module]:
            print(
                f"[stdlib-api-lint][error] {profile}: missing symbol '{symbol}' in module '{module}'",
                file=sys.stderr,
            )
            ok = False

    if not ok:
        return 1

    print(f"[stdlib-api-lint] ok ({len(rows)} ABI entries checked)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
