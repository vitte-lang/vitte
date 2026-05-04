#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
CATALOG_VIT = ROOT / "src" / "vitte" / "packages" / "compiler/driver" / "internal" / "option_catalog.vit"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def unique(items: list[str]) -> list[str]:
    out: list[str] = []
    seen: set[str] = set()
    for item in items:
        if item in seen:
            continue
        seen.add(item)
        out.append(item)
    return out


def extract_vitte_flags(text: str) -> list[str]:
    flags = re.findall(r'"(--[A-Za-z0-9][A-Za-z0-9-]*|-o|-t|-h)"', text)
    return unique(sorted(flags))


def main() -> int:
    vitte_flags = extract_vitte_flags(read(CATALOG_VIT))

    vitte_set = set(vitte_flags)

    print("# Compiler Driver Surface Parity")
    print()
    print(f"- Vitte catalog flags: `{len(vitte_flags)}`")
    print(f"- Unique flags: `{len(vitte_set)}`")
    print("- Status: `Vitte catalog is the single source of truth`")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
