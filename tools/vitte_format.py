#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROOTS = [
    ROOT / "src" / "vitte",
    ROOT / "tests",
    ROOT / "toolchain",
]


def candidate_files() -> list[Path]:
    files: list[Path] = []
    for root in DEFAULT_ROOTS:
        if not root.exists():
            continue
        for suffix in ("*.vit", "*.vitl"):
            files.extend(root.rglob(suffix))
    return sorted({p for p in files if ".git" not in p.parts and "target" not in p.parts})


def changed_files() -> list[Path]:
    commands = [
        ["git", "diff", "--name-only", "--diff-filter=ACMR"],
        ["git", "ls-files", "--others", "--exclude-standard"],
    ]
    files: set[Path] = set()
    for cmd in commands:
        completed = subprocess.run(cmd, cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        if completed.returncode != 0:
            continue
        for line in completed.stdout.splitlines():
            path = ROOT / line
            if path.suffix in {".vit", ".vitl"} and "target" not in path.parts:
                files.add(path)
    return sorted(files)


def format_text(text: str) -> str:
    lines = text.splitlines()
    formatted = "\n".join(line.rstrip() for line in lines)
    return formatted + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description="Format Vitte source files deterministically")
    parser.add_argument("--check", action="store_true", help="fail if formatting would change a file")
    parser.add_argument("--changed", action="store_true", help="only format/check changed and untracked Vitte files")
    parser.add_argument("paths", nargs="*", help="specific files to format/check")
    args = parser.parse_args()

    if args.paths:
        paths = [ROOT / p for p in args.paths]
    elif args.changed:
        paths = changed_files()
    else:
        paths = candidate_files()
    changed: list[Path] = []
    for path in paths:
        if not path.exists() or path.suffix not in {".vit", ".vitl"}:
            continue
        original = path.read_text(encoding="utf-8")
        formatted = format_text(original)
        if formatted != original:
            changed.append(path)
            if not args.check:
                path.write_text(formatted, encoding="utf-8")

    if changed and args.check:
        for path in changed:
            print(f"[vitte-format][error] not formatted: {path.relative_to(ROOT)}")
        return 1

    mode = "check" if args.check else "write"
    print(f"[vitte-format] {mode} ok files={len(paths)} changed={len(changed)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
