#!/usr/bin/env python3
from __future__ import annotations
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CSV_PATH = ROOT / "data" / "ffi" / "abi" / "abi_profiles.csv"
OUT = ROOT / "data" / "ffi" / "abi" / "ABI_COVERAGE.md"


def main() -> int:
    rows = list(csv.DictReader(CSV_PATH.open(newline="", encoding="utf-8")))
    lines = ["# ABI Coverage", "", "| Target | Convention | Arg Registers | Return | Preserved | Ptr | Align(ptr/float) |", "|---|---|---|---|---|---:|---|" ]
    for r in rows:
        lines.append(
            f"| {r['target']} | {r['calling_convention']} | {r['arg_registers']} | {r['return_register']} | {r['preserve_registers']} | {r['pointer_size']} | {r['pointer_alignment']}/{r['float_alignment']} |"
        )
    lines.append("")
    lines.append(f"Total targets: {len(rows)}")
    OUT.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("[ffi-abi] generated ABI_COVERAGE.md")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
