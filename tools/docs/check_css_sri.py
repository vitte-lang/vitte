#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path
import hashlib
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "docs"
SITE = DOCS / "css" / "site.css"
PRINT = DOCS / "css" / "print.css"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fail(msg: str) -> int:
    print(f"[docs-sri][error] {msg}", file=sys.stderr)
    return 1


def main() -> int:
    site_h = sha256(SITE)
    print_h = sha256(PRINT)

    site_pat = re.compile(r'integrity="sha256-([a-f0-9]{64})"')

    for p in DOCS.rglob("*.html"):
        txt = p.read_text(encoding="utf-8")
        for m in re.finditer(r'<link[^>]*href="[^\"]*site\.css[^\"]*"[^>]*>', txt):
            im = site_pat.search(m.group(0))
            if not im:
                return fail(f"missing site.css integrity in {p.relative_to(ROOT)}")
            if im.group(1) != site_h:
                return fail(f"invalid site.css integrity in {p.relative_to(ROOT)}")
        for m in re.finditer(r'<link[^>]*href="[^\"]*print\.css[^\"]*"[^>]*>', txt):
            im = site_pat.search(m.group(0))
            if not im:
                return fail(f"missing print.css integrity in {p.relative_to(ROOT)}")
            if im.group(1) != print_h:
                return fail(f"invalid print.css integrity in {p.relative_to(ROOT)}")

    print("[docs-sri] all docs CSS integrity values are valid")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
