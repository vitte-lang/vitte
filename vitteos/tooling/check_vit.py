#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


def run() -> int:
    parser = argparse.ArgumentParser(description="Check a .vit file with local Vitte compiler")
    parser.add_argument("source", nargs="?", default="vitteos/scripts/vitteos_tooling.vit")
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[2]
    source = (root / args.source).resolve()
    compiler = root / "bin" / "vitte"

    if not source.exists():
        print(f"[check_vit] source not found: {source}")
        return 2

    if not compiler.exists():
        print(f"[check_vit] compiler not found: {compiler}")
        print("[check_vit] run: make build")
        return 3

    cmd = [str(compiler), "check", str(source)]
    print(f"[check_vit] running: {' '.join(cmd)}")
    return subprocess.call(cmd)


if __name__ == "__main__":
    raise SystemExit(run())
