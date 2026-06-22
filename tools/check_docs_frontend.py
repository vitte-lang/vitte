#!/usr/bin/env python3
from __future__ import annotations

from pathlib import Path
import re

DOCS = Path("docs")
SCRIPT_SRC_RE = re.compile(r'<script[^>]+src="([^"]+)"[^>]*>', re.I)
INTEGRITY_ATTR_RE = re.compile(r'<[^>]+\bintegrity="[^"]+"[^>]*>', re.I)
CORS_ATTR_RE = re.compile(r'<[^>]+\bcrossorigin="anonymous"[^>]*>', re.I)

CRITICAL_PAGES = [
    Path("index.html"),
    Path("doc.html"),
    Path("news.html"),
    Path("community.html"),
    Path("search.html"),
    Path("status.html"),
    Path("broken-links.html"),
    Path("book/index.html"),
    Path("book/chapters/20a-architecture-globale.html"),
    Path("compiler/architecture.html"),
]


def must_contain(text: str, needle: str) -> bool:
    return needle in text


def main() -> int:
    errors: list[str] = []

    for rel in CRITICAL_PAGES:
        page = DOCS / rel
        if not page.exists():
            errors.append(f"{page}: missing page")
            continue
        text = page.read_text(encoding="utf-8", errors="ignore")
        path = str(page).replace("\\", "/")

        if "css/site.css" not in text:
            errors.append(f"{path}: missing site.css")
        if "css/print.css" not in text:
            errors.append(f"{path}: missing print.css")
        if not must_contain(text, '<header class="site-header">'):
            errors.append(f"{path}: missing site header")
        if not must_contain(text, "js/main.js"):
            errors.append(f"{path}: missing main.js module script")

    for page in sorted(DOCS.rglob("*.html")):
        text = page.read_text(encoding="utf-8", errors="ignore")
        path = str(page).replace("\\", "/")

        if INTEGRITY_ATTR_RE.search(text):
            errors.append(f"{path}: forbidden integrity attribute")
        if CORS_ATTR_RE.search(text):
            errors.append(f"{path}: forbidden crossorigin attribute")

        script_sources = SCRIPT_SRC_RE.findall(text)
        if any("site-interactions.js" in src or "main.min.js" in src for src in script_sources):
            errors.append(f"{path}: unexpected secondary docs script reference")

    if errors:
        print("docs frontend check failed")
        print("\n".join(errors[:300]))
        return 1

    print("docs frontend check ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
