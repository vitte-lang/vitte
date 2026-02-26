#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


def load_allowlist(path: Path) -> set[str]:
    if not path.exists():
        return set()
    out: set[str] = set()
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        out.add(line)
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description="Lint module path naming (_ forbidden unless allowlisted)")
    parser.add_argument("--root", default="src/vitte/packages", help="root directory to scan")
    parser.add_argument(
        "--allowlist",
        default="tools/module_path_name_legacy_allowlist.txt",
        help="allowlist of legacy directory names with '_'",
    )
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    root = (repo / args.root).resolve()
    allow = load_allowlist((repo / args.allowlist).resolve())

    if not root.exists():
        print(f"[module-naming-lint][error] missing root: {root}")
        return 1

    errors: list[str] = []
    scanned = 0
    for mod in sorted(root.rglob("mod.vit")):
        scanned += 1
        rel_dir = mod.parent.relative_to(root)
        for seg in rel_dir.parts:
            if "_" in seg and seg not in allow:
                errors.append(
                    f"{mod.parent.relative_to(repo)}: segment '{seg}' contains '_' (use '/' hierarchy or add allowlist)"
                )

    print(f"[module-naming-lint] scanned modules: {scanned}")
    for e in errors:
        print(f"[module-naming-lint][error] {e}")
    if errors:
        print(f"[module-naming-lint] FAILED: {len(errors)} error(s)")
        return 1
    print("[module-naming-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
