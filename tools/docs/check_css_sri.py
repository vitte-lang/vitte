#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "docs"
def fail(msg: str) -> int:
    print(f"[docs-sri][error] {msg}", file=sys.stderr)
    return 1


def main() -> int:
    bad_attr_pat = re.compile(r'\s(?:integrity|crossorigin)="[^"]*"')

    for p in DOCS.rglob("*.html"):
        txt = p.read_text(encoding="utf-8")
        if bad_attr_pat.search(txt):
            return fail(f"unexpected SRI/CORS attribute in {p.relative_to(ROOT)}")

    print("[docs-sri] OK: no SRI/CORS attributes on local docs assets")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
