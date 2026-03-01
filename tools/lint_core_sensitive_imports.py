#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path


DENY = (
    "vitte/ffi",
    "vitte/process",
    "vitte/net",
    "vitte/http_client",
)


def main() -> int:
    repo = Path(__file__).resolve().parents[1]
    root = repo / "src/vitte/packages/core"
    errs: list[str] = []

    for p in root.rglob("*.vit"):
        for i, raw in enumerate(p.read_text(encoding="utf-8").splitlines(), start=1):
            line = raw.strip()
            if not line.startswith("use "):
                continue
            for denied in DENY:
                if f"use {denied} " in line or line == f"use {denied}":
                    errs.append(f"{p}:{i}: sensitive import denied in core: {denied}")

    if errs:
        for e in errs:
            print(f"[core-sensitive-imports][error] {e}")
        return 1
    print("[core-sensitive-imports] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
