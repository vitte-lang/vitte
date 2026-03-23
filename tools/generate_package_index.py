#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PKG_ROOT = ROOT / "src" / "vitte" / "packages"
DOCS = ROOT / "docs"
OUT = DOCS / "PACKAGE_INDEX.md"


DOC_MAP = {
    "std": "docs/std/README.md",
    "fs": "docs/fs/README.md",
    "db": "docs/db/README.md",
    "http": "docs/http/README.md",
    "http_client": "docs/http_client/README.md",
    "process": "docs/process/README.md",
    "log": "docs/log/README.md",
}

API_MAP = {
    "std": "docs/std/API_INDEX.md",
    "fs": "docs/fs/API_INDEX.md",
    "db": "docs/db/API_INDEX.md",
    "http": "docs/http/API_INDEX.md",
    "http_client": "docs/http_client/API_INDEX.md",
    "process": "docs/process/API_INDEX.md",
    "log": "docs/log/API_INDEX.md",
    "json": "docs/json/API_INDEX.md",
    "yaml": "docs/yaml/API_INDEX.md",
    "lint": "docs/lint/API_INDEX.md",
    "test": "docs/test/API_INDEX.md",
}

DOCUMENTED_EXPERIMENTAL = {
    "std", "fs", "db", "http", "http_client", "process", "log", "json", "yaml", "lint", "test"
}


def rel(path: Path) -> str:
    return path.relative_to(ROOT).as_posix()


def package_rows() -> list[str]:
    rows: list[str] = []
    for pkg in sorted(p.name for p in PKG_ROOT.iterdir() if p.is_dir()):
        base = PKG_ROOT / pkg
        facade = rel(base / "mod.vit") if (base / "mod.vit").exists() else "-"
        info = rel(base / "info.vit") if (base / "info.vit").exists() else "-"
        owners = rel(base / "OWNERS") if (base / "OWNERS").exists() else "-"
        doc = DOC_MAP.get(pkg, "-")
        api = API_MAP.get(pkg, "-")
        maturity = "documented-experimental" if pkg in DOCUMENTED_EXPERIMENTAL else "internal-by-default"
        rows.append(f"| `{pkg}` | `{maturity}` | `{facade}` | `{info}` | `{owners}` | `{doc}` | `{api}` |")
    return rows


def main() -> int:
    lines = [
        "# Package Index",
        "",
        "This file maps package families to their main repository anchors.",
        "",
        "Use `docs/PACKAGE_MATURITY.md` for the maturity policy.",
        "Use this file when you need the practical index of facades, metadata, docs, and API pages.",
        "",
        "| Package | Default maturity | Facade | Metadata | Owners | Docs | API index |",
        "| --- | --- | --- | --- | --- | --- | --- |",
    ]
    lines.extend(package_rows())
    lines.extend([
        "",
        "## Reading Rule",
        "",
        "- `documented-experimental` means visible public surface, but still expected to change unless explicitly promoted",
        "- `internal-by-default` means the package exists in the repository, but should not be treated as a public promise just because it is present",
    ])

    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"[generate-package-index] wrote {OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
