#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


def load_snapshot_commands(snapshots_dir: Path) -> str:
    chunks: list[str] = []
    if not snapshots_dir.exists():
        return ""
    for cmd in sorted(snapshots_dir.glob("*.cmd")):
        chunks.append(cmd.read_text(encoding="utf-8"))
    return "\n".join(chunks)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Ensure each public module has at least one modules snapshot"
    )
    parser.add_argument(
        "--public-root",
        default="tests/modules/module_tree/public",
        help="public modules root",
    )
    parser.add_argument(
        "--snapshots-dir",
        default="tests/modules/snapshots",
        help="snapshot cmd dir",
    )
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    public_root = (repo / args.public_root).resolve()
    snapshots_dir = (repo / args.snapshots_dir).resolve()

    if not public_root.exists():
        print(f"[public-snapshots-lint] missing public root: {public_root}")
        return 1

    cmd_blob = load_snapshot_commands(snapshots_dir)
    errors: list[str] = []
    scanned = 0

    for mod in sorted(public_root.rglob("mod.vit")):
        scanned += 1
        rel = mod.relative_to(repo).as_posix()
        if rel not in cmd_blob:
            errors.append(f"{rel}: no snapshot cmd references this public module")

    print(f"[public-snapshots-lint] scanned public modules: {scanned}")
    for e in errors:
        print(f"[public-snapshots-lint][error] {e}")
    if errors:
        print(f"[public-snapshots-lint] FAILED: {len(errors)} error(s)")
        return 1
    print("[public-snapshots-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
