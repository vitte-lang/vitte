#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import re

OWNER_RE = re.compile(r"^@[a-z0-9_.-]+/[a-z0-9_.-]+$")


def non_comment_lines(path: Path) -> list[str]:
    out: list[str] = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        out.append(line)
    return out


def find_experimental_modules(root: Path) -> list[Path]:
    mods: list[Path] = []
    if not root.exists():
        return mods
    for mod in root.rglob("mod.vit"):
        rel = mod.parent.relative_to(root).as_posix()
        if "/experimental/" in f"/{rel}/" or rel.startswith("experimental/"):
            mods.append(mod.parent)
    return sorted(mods)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Require owner + justification for experimental modules"
    )
    parser.add_argument(
        "--roots",
        nargs="+",
        default=["tests/modules/module_tree", "src/vitte/packages"],
        help="roots to scan",
    )
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    errors: list[str] = []
    scanned = 0

    for root_rel in args.roots:
        root = (repo / root_rel).resolve()
        for module_dir in find_experimental_modules(root):
            scanned += 1
            owners = module_dir / "OWNERS"
            just = module_dir / "JUSTIFICATION.md"
            if not owners.exists():
                errors.append(f"{module_dir.relative_to(repo)}: missing OWNERS")
            else:
                lines = non_comment_lines(owners)
                if not lines:
                    errors.append(f"{owners.relative_to(repo)}: empty OWNERS")
                for line in lines:
                    if not OWNER_RE.match(line):
                        errors.append(
                            f"{owners.relative_to(repo)}: invalid owner '{line}' (expected @team/name)"
                        )
            if not just.exists():
                errors.append(f"{module_dir.relative_to(repo)}: missing JUSTIFICATION.md")
            else:
                lines = non_comment_lines(just)
                if not lines:
                    errors.append(f"{just.relative_to(repo)}: empty justification")

    print(f"[experimental-modules-lint] scanned experimental modules: {scanned}")
    for e in errors:
        print(f"[experimental-modules-lint][error] {e}")
    if errors:
        print(f"[experimental-modules-lint] FAILED: {len(errors)} error(s)")
        return 1
    print("[experimental-modules-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
