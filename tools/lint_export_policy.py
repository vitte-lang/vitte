#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


def major(version: str) -> str:
    v = version.strip()
    if not v:
        return "0"
    return v.split(".", 1)[0]


def read_removed(path: Path) -> list[str]:
    if not path.exists():
        return []
    out: list[str] = []
    for raw in path.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if line:
            out.append(line)
    return out


def main() -> int:
    p = argparse.ArgumentParser(
        description="Fail when stable export removals occur without major version bump"
    )
    p.add_argument("--module", required=True, help="module/package name")
    p.add_argument("--scope", required=True, help="exports scope (all/public/internal)")
    p.add_argument("--current-version", required=True)
    p.add_argument("--baseline-version", required=True)
    p.add_argument("--removed-file", required=True, help="path with removed symbols, one per line")
    p.add_argument("--allow-breaking", action="store_true")
    args = p.parse_args()

    removed = read_removed(Path(args.removed_file))
    print(
        "[export-policy-lint] "
        f"module={args.module} scope={args.scope} removed={len(removed)} "
        f"baseline={args.baseline_version} current={args.current_version}"
    )

    if not removed:
        print("[export-policy-lint] OK")
        return 0
    if args.allow_breaking:
        print("[export-policy-lint] OK (allow-breaking)")
        return 0

    if major(args.current_version) == major(args.baseline_version):
        print("[export-policy-lint][error] breaking export removal without major bump")
        for sym in removed:
            print(f"[export-policy-lint][error] removed: {sym}")
        return 1

    print("[export-policy-lint] OK (major bump detected)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
