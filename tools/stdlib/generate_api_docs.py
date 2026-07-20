#!/usr/bin/env python3

from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "src/vitte/stdlib/stdlib_modules.json"
OUTPUT = ROOT / "docs/compiler/stdlib_api.generated.md"
PROC_RE = re.compile(r"^\s*proc\s+([A-Za-z_][A-Za-z0-9_]*)(.*)")
FORM_RE = re.compile(r"^\s*form\s+([A-Za-z_][A-Za-z0-9_]*)(.*)")
PICK_RE = re.compile(r"^\s*pick\s+([A-Za-z_][A-Za-z0-9_]*)(.*)")
CONST_RE = re.compile(r"^\s*const\s+([A-Za-z_][A-Za-z0-9_]*)(.*)")


def collect_symbols(path: Path) -> list[str]:
    symbols: list[str] = []
    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        for regex, kind in ((PROC_RE, "proc"), (FORM_RE, "form"), (PICK_RE, "pick"), (CONST_RE, "const")):
            match = regex.match(line)
            if match:
                signature = line.strip()
                name = match.group(1)
                symbols.append(f"- `{kind} {name}` signature `{signature}` example `{name}`")
                break
    return symbols


def main() -> int:
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    lines = [
        "# Vitte Stdlib API",
        "",
        "Generated from `src/vitte/stdlib/stdlib_modules.json`.",
        "Each entry is suitable for LSP symbol indexing and documentation lookup.",
        "",
    ]
    for entry in manifest["official_entrypoints"]:
        path = ROOT / entry
        if not path.exists() or path.suffix not in {".vit", ".vitl"}:
            continue
        symbols = collect_symbols(path)
        if not symbols:
            continue
        lines.append(f"## `{entry}`")
        lines.append("")
        lines.extend(symbols)
        lines.append("")
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
