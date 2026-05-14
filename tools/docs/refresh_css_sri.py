#!/usr/bin/env python3
from __future__ import annotations
from pathlib import Path
import hashlib
import re

ROOT = Path(__file__).resolve().parents[2]
DOCS = ROOT / "docs"
SITE = DOCS / "css" / "site.css"
PRINT = DOCS / "css" / "print.css"


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def patch_tag(tag: str, h: str) -> str:
    tag = re.sub(r"\s+integrity=\"[^\"]*\"", "", tag)
    tag = re.sub(r"\s+crossorigin=\"[^\"]*\"", "", tag)
    return tag[:-1] + f' integrity="sha256-{h}" crossorigin="anonymous">'


def main() -> int:
    site_h = sha256(SITE)
    print_h = sha256(PRINT)

    for p in DOCS.rglob("*.html"):
        s = p.read_text(encoding="utf-8")
        s2 = re.sub(r'<link[^>]*rel="stylesheet"[^>]*href="[^\"]*site\\.css[^\"]*"[^>]*>', lambda m: patch_tag(m.group(0), site_h), s)
        s2 = re.sub(r'<link[^>]*rel="preload"[^>]*as="style"[^>]*href="[^\"]*site\\.css[^\"]*"[^>]*>', lambda m: patch_tag(m.group(0), site_h), s2)
        s2 = re.sub(r'<link[^>]*rel="stylesheet"[^>]*href="[^\"]*print\\.css[^\"]*"[^>]*>', lambda m: patch_tag(m.group(0), print_h), s2)
        if s2 != s:
            p.write_text(s2, encoding="utf-8")

    print(f"[docs-sri] refreshed CSS SRI: site.css={site_h} print.css={print_h}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
