#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import sys


def collect_conflicts(root: Path) -> list[tuple[str, Path, Path]]:
    flat: dict[str, Path] = {}
    mod: dict[str, Path] = {}

    for path in sorted(root.rglob("*.vit")):
        rel = path.relative_to(root)
        if rel.name == "mod.vit":
            key = rel.parent.as_posix()
            mod[key] = path
        else:
            key = rel.with_suffix("").as_posix()
            flat[key] = path

    conflicts: list[tuple[str, Path, Path]] = []
    for key in sorted(set(flat) & set(mod)):
        conflicts.append((key, flat[key], mod[key]))
    return conflicts


def main() -> int:
    repo_root = Path(__file__).resolve().parents[1]
    target = repo_root / "src" / "vitte" / "packages"

    if not target.exists():
        print(f"[module-shape-policy][error] missing directory: {target}", file=sys.stderr)
        return 1

    conflicts = collect_conflicts(target)
    if not conflicts:
        print("[module-shape-policy] OK")
        return 0

    print("[module-shape-policy][error] mixed module shape detected under src/vitte/packages", file=sys.stderr)
    for key, flat_file, mod_file in conflicts:
        rel_flat = flat_file.relative_to(repo_root).as_posix()
        rel_mod = mod_file.relative_to(repo_root).as_posix()
        print(
            f"[module-shape-policy][error] {key}: both '{rel_flat}' and '{rel_mod}' exist",
            file=sys.stderr,
        )
        print(
            "[module-shape-policy][error] fix: keep only one shape (either '<name>.vit' or '<name>/mod.vit')",
            file=sys.stderr,
        )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
