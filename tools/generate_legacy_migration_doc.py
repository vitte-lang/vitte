#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


def canonicalize(path: str) -> str:
    parts = [p for p in path.split("/") if p]
    if len(parts) >= 2 and parts[-1] == parts[-2]:
        return "/".join(parts[:-1])
    return path


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate old->new legacy import migration table")
    parser.add_argument("--allowlist", default="tools/legacy_import_path_allowlist.txt")
    parser.add_argument("--entry-allowlist", default="tools/package_entry_legacy_allowlist.txt")
    parser.add_argument("--name-allowlist", default="tools/package_name_legacy_allowlist.txt")
    parser.add_argument("--out", default="book/packages-migration-map.md")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allowlist = (repo / args.allowlist).resolve()
    entry_allowlist = (repo / args.entry_allowlist).resolve()
    name_allowlist = (repo / args.name_allowlist).resolve()
    out = (repo / args.out).resolve()

    rows: list[tuple[str, str, str]] = []
    if allowlist.exists():
        for raw in allowlist.read_text(encoding="utf-8").splitlines():
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            key = line.split("#", 1)[0].strip()
            if ":" not in key:
                continue
            file_path, line_no = key.rsplit(":", 1)
            file_abs = repo / file_path
            if not file_abs.exists():
                continue
            try:
                idx = int(line_no) - 1
            except ValueError:
                continue
            lines = file_abs.read_text(encoding="utf-8").splitlines()
            if idx < 0 or idx >= len(lines):
                continue
            text = lines[idx].strip()
            if not text.startswith("use ") and not text.startswith("pull "):
                continue
            parts = text.split()
            if len(parts) < 2:
                continue
            old = parts[1]
            new = canonicalize(old)
            rows.append((f"{file_path}:{line_no}", old, new))

    rows.sort()
    legacy_wrappers: list[str] = []
    if entry_allowlist.exists():
        for raw in entry_allowlist.read_text(encoding="utf-8").splitlines():
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            legacy_wrappers.append(line)
    legacy_wrappers.sort()

    underscore_names: list[tuple[str, str]] = []
    if name_allowlist.exists():
        for raw in name_allowlist.read_text(encoding="utf-8").splitlines():
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            underscore_names.append((line, line.replace("_", "/")))
    underscore_names.sort()

    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w", encoding="utf-8") as f:
        f.write("# Packages Migration Map (Auto)\n\n")
        f.write("Generated from legacy allowlists in `tools/`.\n\n")
        f.write("| Location | Old import | New import |\n")
        f.write("|---|---|---|\n")
        for loc, old, new in rows:
            f.write(f"| {loc} | `{old}` | `{new}` |\n")
        f.write("\n")
        f.write("## Legacy Wrappers (`<pkg>.vit` -> `mod.vit`)\n\n")
        f.write("| Package | Old entry | New entry |\n")
        f.write("|---|---|---|\n")
        for pkg in legacy_wrappers:
            f.write(f"| `{pkg}` | `{pkg}/{pkg}.vit` | `{pkg}/mod.vit` |\n")
        f.write("\n")
        f.write("## Legacy Names (`_` -> `/`)\n\n")
        f.write("| Legacy name | Target hierarchy |\n")
        f.write("|---|---|\n")
        for old_name, new_name in underscore_names:
            f.write(f"| `{old_name}` | `{new_name}` |\n")

    print(
        f"[migration-map] wrote {out.relative_to(repo)} "
        f"with imports={len(rows)} wrappers={len(legacy_wrappers)} names={len(underscore_names)}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
