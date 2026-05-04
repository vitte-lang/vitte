#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
CATALOG_VIT = ROOT / "src" / "vitte" / "packages" / "compiler/driver" / "internal" / "option_catalog.vit"


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def unique(items: list[str]) -> list[str]:
    seen: set[str] = set()
    out: list[str] = []
    for item in items:
        if item in seen:
            continue
        seen.add(item)
        out.append(item)
    return out


def extract_array(text: str, proc_name: str) -> list[str]:
    match = re.search(rf"proc {proc_name}\(\).*?give \[(.*?)\]", text, re.S)
    if not match:
        return []
    return unique(re.findall(r'"([^"]+)"', match.group(1)))


def main() -> int:
    catalog = read(CATALOG_VIT)

    commands = extract_array(catalog, "top_level_commands")
    mod_subcommands = extract_array(catalog, "mod_subcommands")
    grammar_subcommands = extract_array(catalog, "grammar_subcommands")
    flags = unique(sorted(extract_array(catalog, "boolean_flags") + extract_array(catalog, "value_flags")))

    print("# Compiler Driver Surface Audit")
    print()
    print(f"- Source: `{CATALOG_VIT.relative_to(ROOT)}`")
    print(f"- Top-level commands: `{len(commands)}`")
    print(f"- Flags and short options: `{len(flags)}`")
    print()
    print("## Commands")
    for command in commands:
        print(f"- `{command}`")
    print()
    print("## `grammar` Subcommands")
    for command in grammar_subcommands:
        print(f"- `{command}`")
    print()
    print("## `mod` Subcommands")
    for command in mod_subcommands:
        print(f"- `{command}`")
    print()
    print("## Flags")
    for flag in flags:
        print(f"- `{flag}`")
    print()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
