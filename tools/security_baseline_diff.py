#!/usr/bin/env python3
from __future__ import annotations
import argparse
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / "tests/modules/contracts/security/security_gates_baseline.report"
CUR = ROOT / "target/reports/security_gates.report"


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--update", action="store_true")
    args = ap.parse_args()

    if args.update:
        if not CUR.exists():
            print(f"[security-baseline][error] missing current report: {CUR}")
            return 1
        BASE.parent.mkdir(parents=True, exist_ok=True)
        BASE.write_text(CUR.read_text(encoding="utf-8"), encoding="utf-8")
        print(f"[security-baseline] updated {BASE}")
        return 0

    if not BASE.exists():
        print(f"[security-baseline][error] missing baseline: {BASE}")
        return 1
    if not CUR.exists():
        print(f"[security-baseline][error] missing current report: {CUR}")
        return 1

    b = BASE.read_text(encoding="utf-8")
    c = CUR.read_text(encoding="utf-8")
    if b != c:
        out = ROOT / "target/reports/security_gates.diff"
        out.write_text("baseline/current differ\n", encoding="utf-8")
        print(f"[security-baseline][error] diff detected; see {out}")
        return 1
    print("[security-baseline] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
