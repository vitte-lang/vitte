#!/usr/bin/env python3

from __future__ import annotations

import json
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / "src/vitte/stdlib/stdlib_modules.json"
OUTPUT = ROOT / "docs/compiler/stdlib_api.generated.md"
JSON_OUTPUT = ROOT / "docs/compiler/stdlib_api.generated.json"
LSP_OUTPUT = ROOT / "docs/compiler/stdlib_lsp_index.generated.json"
PROC_RE = re.compile(r"^\s*proc\s+([A-Za-z_][A-Za-z0-9_]*)(.*)")
FORM_RE = re.compile(r"^\s*form\s+([A-Za-z_][A-Za-z0-9_]*)(.*)")
PICK_RE = re.compile(r"^\s*pick\s+([A-Za-z_][A-Za-z0-9_]*)(.*)")
CONST_RE = re.compile(r"^\s*const\s+([A-Za-z_][A-Za-z0-9_]*)(.*)")


def module_stability(entry: str) -> str:
    parts = Path(entry).parts
    if "experimental" in parts:
        return "experimental"
    if any(part in parts for part in ("tests", "benchmarks", "tools", "generated")):
        return "internal"
    if any(part in parts for part in ("core", "alloc", "std", "platform")):
        return "stable"
    return "stable"


def usage_example(module: str, name: str, kind: str) -> str:
    if kind == "proc":
        return f"{module}.{name}(...)"
    return f"{module}.{name}"


def collect_symbols(path: Path, entry: str) -> list[dict[str, str]]:
    symbols: list[dict[str, str]] = []
    module_name = path.stem
    stability = module_stability(entry)
    for line_number, line in enumerate(path.read_text(encoding="utf-8", errors="ignore").splitlines(), start=1):
        for regex, kind in ((PROC_RE, "proc"), (FORM_RE, "form"), (PICK_RE, "pick"), (CONST_RE, "const")):
            match = regex.match(line)
            if match:
                signature = line.strip()
                name = match.group(1)
                symbol_id = f"{entry}::{name}"
                symbols.append({
                    "id": symbol_id,
                    "name": name,
                    "kind": kind,
                    "signature": signature,
                    "module": entry,
                    "line": line_number,
                    "stability": stability,
                    "visibility": "public",
                    "example": usage_example(module_name, name, kind),
                })
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
    api_entries = []
    for entry in manifest["official_entrypoints"]:
        path = ROOT / entry
        if not path.exists() or path.suffix not in {".vit", ".vitl"}:
            continue
        symbols = collect_symbols(path, entry)
        if not symbols:
            continue
        stability = module_stability(entry)
        lines.append(f"## `{entry}`")
        lines.append("")
        lines.append(f"Stability: `{stability}`")
        lines.append("")
        lines.extend(
            f"- `{symbol['kind']} {symbol['name']}` signature `{symbol['signature']}` example `{symbol['example']}` stability `{symbol['stability']}`"
            for symbol in symbols
        )
        api_entries.append({
            "module": entry,
            "path": entry,
            "stability": stability,
            "symbols": symbols,
        })
        lines.append("")
    OUTPUT.write_text("\n".join(lines), encoding="utf-8")
    JSON_OUTPUT.write_text(json.dumps({
        "schema": "vitte.stdlib.api",
        "schema_version": "1.1.0",
        "source_manifest": str(MANIFEST.relative_to(ROOT)),
        "modules": api_entries,
    }, indent=2), encoding="utf-8")
    LSP_OUTPUT.write_text(json.dumps({
        "schema": "vitte.stdlib.lsp-index",
        "schema_version": "1.1.0",
        "symbols": [
            {
                "id": symbol["id"],
                "name": symbol["name"],
                "kind": symbol["kind"],
                "signature": symbol["signature"],
                "module": entry["module"],
                "path": entry["path"],
                "line": symbol["line"],
                "example": symbol["example"],
                "stability": symbol["stability"],
                "visibility": symbol["visibility"],
            }
            for entry in api_entries
            for symbol in entry["symbols"]
        ],
    }, indent=2), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
