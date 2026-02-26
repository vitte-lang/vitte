#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

IMPORT_RE = re.compile(r"^\s*(use|pull)\s+([^\s]+)")


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
    parser = argparse.ArgumentParser(
        description="Forbid std/* imports in selected .vit roots"
    )
    parser.add_argument(
        "--roots",
        nargs="+",
        default=["src/vitte/packages"],
        help="roots to scan for .vit files",
    )
    parser.add_argument(
        "--allowlist",
        default="tools/std_import_legacy_allowlist.txt",
        help="optional allowlist entries path:line",
    )
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[1]
    allow = load_allowlist((repo / args.allowlist).resolve())
    errors: list[str] = []
    scanned = 0

    for rel_root in args.roots:
        root = (repo / rel_root).resolve()
        if not root.exists():
            continue
        for file in sorted(root.rglob("*.vit")):
            scanned += 1
            rel = file.relative_to(repo).as_posix()
            for i, line in enumerate(file.read_text(encoding="utf-8").splitlines(), start=1):
                m = IMPORT_RE.match(line)
                if not m:
                    continue
                import_path = m.group(2)
                if not import_path.startswith("std/"):
                    continue
                key = f"{rel}:{i}"
                if key in allow:
                    continue
                errors.append(
                    f"{key}: std import forbidden '{import_path}' (migrate to vitte/*)"
                )

    print(f"[no-std-lint] scanned files: {scanned}")
    print(f"[no-std-lint] allowlist entries: {len(allow)}")
    for err in errors:
        print(f"[no-std-lint][error] {err}")
    if errors:
        print(f"[no-std-lint] FAILED: {len(errors)} error(s)")
        return 1
    print("[no-std-lint] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
