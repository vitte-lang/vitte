#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BIN = ROOT / "bin" / "vitte"
FREQUENT_CODES = [
    "E0001",
    "E0002",
    "E0005",
    "E1002",
    "E1005",
    "E1011",
    "E1012",
    "E1013",
    "E1016",
    "E1018",
    "E1022",
    "E1027",
    "E1029",
]


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--strict", action="store_true")
    args = ap.parse_args()

    if not BIN.exists():
        print(f"[diag-autofix][error] missing compiler binary: {BIN}")
        return 1

    failed = []
    for code in FREQUENT_CODES:
        proc = subprocess.run(
            [str(BIN), "explain", code],
            cwd=str(ROOT),
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        text = proc.stdout
        ok = proc.returncode == 0 and ("Fix:" in text) and ("Summary:" in text) and ("Example:" in text)
        print(f"[diag-autofix] {code}: {'ok' if ok else 'missing'}")
        if not ok:
            failed.append(code)

    if failed:
        print(f"[diag-autofix] missing fix coverage: {', '.join(failed)}")
        return 1 if args.strict else 0
    print("[diag-autofix] all frequent diagnostics expose fix guidance")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
