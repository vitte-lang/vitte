#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import re

REQUIRED_BUCKETS = ("public", "internal", "experimental")
OWNER_RE = re.compile(r"^@[a-z0-9_.-]+/[a-z0-9_.-]+$")
GLOB_IMPORT_RE = re.compile(r"^\s*(use|pull)\s+[^\s]*\.\*\s+as\s+\w+")


def non_comment_lines(path: Path) -> list[str]:
    out: list[str] = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        out.append(line)
    return out


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Lint module tree: enforce public/internal/experimental + ownership"
    )
    parser.add_argument(
        "--root",
        default="tests/modules/module_tree",
        help="module tree root to lint",
    )
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    root = (repo / args.root).resolve()

    if not root.exists():
        print(f"[module-tree-lint][error] missing root: {root}")
        return 1

    errors: list[str] = []

    buckets = sorted([p.name for p in root.iterdir() if p.is_dir()])
    for b in buckets:
        if b not in REQUIRED_BUCKETS:
            errors.append(f"{root / b}: unknown top-level bucket '{b}'")

    for b in REQUIRED_BUCKETS:
        bucket = root / b
        if not bucket.exists() or not bucket.is_dir():
            errors.append(f"{bucket}: missing required bucket")
            continue

        module_dirs = sorted([p for p in bucket.iterdir() if p.is_dir()])
        if not module_dirs:
            errors.append(f"{bucket}: must contain at least one module directory")
            continue

        for module_dir in module_dirs:
            mod_file = module_dir / "mod.vit"
            owners_file = module_dir / "OWNERS"
            if not mod_file.exists():
                errors.append(f"{module_dir}: missing mod.vit")
            if not owners_file.exists():
                errors.append(f"{module_dir}: missing OWNERS")
            else:
                owners = non_comment_lines(owners_file)
                if not owners:
                    errors.append(f"{owners_file}: no owner entry")
                for owner in owners:
                    if not OWNER_RE.match(owner):
                        errors.append(f"{owners_file}: invalid owner '{owner}' (expected @team/name)")

            if mod_file.exists() and b == "public":
                for i, line in enumerate(mod_file.read_text(encoding="utf-8").splitlines(), start=1):
                    if GLOB_IMPORT_RE.match(line):
                        errors.append(
                            f"{mod_file}:{i}: glob import is forbidden in public modules (allowed only in experimental)"
                        )

    print(f"[module-tree-lint] scanned root: {root.relative_to(repo)}")
    for err in errors:
        print(f"[module-tree-lint][error] {err}")

    if errors:
        print(f"[module-tree-lint] FAILED: {len(errors)} error(s)")
        return 1

    print("[module-tree-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
