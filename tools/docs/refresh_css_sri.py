#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "docs"
def main() -> int:
    changed = 0
    for p in DOCS.rglob("*.html"):
        s = p.read_text(encoding="utf-8")
        s2 = re.sub(r"\s+integrity=\"[^\"]*\"", "", s)
        s2 = re.sub(r"\s+crossorigin=\"[^\"]*\"", "", s2)
        if s2 != s:
            p.write_text(s2, encoding="utf-8")
            changed += 1
    print(f"[docs-sri] stripped SRI/CORS attributes in {changed} docs pages")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
