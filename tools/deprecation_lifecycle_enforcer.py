#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RX = re.compile(r"DEPRECATED[^\n]*removal target:\s*v(\d+)\.(\d+)\.(\d+)")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--current-version", default="3.0.0")
    args = ap.parse_args()
    cur = tuple(int(x) for x in args.current_version.split("."))

    errs = []
    for p in (ROOT / "src/vitte/packages").rglob("mod.vit"):
        txt = p.read_text(encoding="utf-8", errors="ignore")
        for m in RX.finditer(txt):
            tgt = (int(m.group(1)), int(m.group(2)), int(m.group(3)))
            if tgt <= cur:
                errs.append(f"{p}: deprecated wrapper expired at v{tgt[0]}.{tgt[1]}.{tgt[2]}")

    if errs:
        for e in errs:
            print(f"[deprecation-lifecycle][error] {e}")
        return 1
    print("[deprecation-lifecycle] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
