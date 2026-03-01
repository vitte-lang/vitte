#!/usr/bin/env python3
from __future__ import annotations
import argparse, re
from pathlib import Path

USE = re.compile(r"^\s*use\s+([^\s]+)\s+as\s+([A-Za-z_][A-Za-z0-9_]*)")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--roots", required=True, help="comma separated roots")
    ap.add_argument("--tag", default="alias-pkg-generic")
    args = ap.parse_args()

    repo = Path(__file__).resolve().parents[1]
    errs: list[str] = []
    for root_s in [r.strip() for r in args.roots.split(",") if r.strip()]:
        root = repo / root_s
        if not root.exists():
            continue
        for p in root.rglob("*.vit"):
            for i, line in enumerate(p.read_text(encoding="utf-8").splitlines(), 1):
                m = USE.match(line)
                if m and not m.group(2).endswith("_pkg"):
                    errs.append(f"{p}:{i}: alias must end with _pkg")
    if errs:
        for e in errs:
            print(f"[{args.tag}][error] {e}")
        return 1
    print(f"[{args.tag}] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
