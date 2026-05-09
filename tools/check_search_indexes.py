#!/usr/bin/env python3
from pathlib import Path
import json
import sys

DOCS = Path("docs")
FILES = [
    DOCS / "search-index.json",
    DOCS / "search-index.docs.json",
    DOCS / "search-index.grammar.json",
    DOCS / "search-index.book.json",
]

errors = []

for fp in FILES:
    if not fp.exists():
        errors.append(f"missing: {fp}")
        continue
    try:
        data = json.loads(fp.read_text(encoding="utf-8"))
    except Exception:
        errors.append(f"invalid json: {fp}")
        continue
    pages = data.get("pages", [])
    if not pages:
        errors.append(f"empty pages: {fp}")
        continue
    for i, item in enumerate(pages[:50]):
        for key in ("title", "path", "content", "lang", "section"):
            if key not in item:
                errors.append(f"{fp}: missing '{key}' in item {i}")
        if item.get("lang") not in ("en", "fr"):
            errors.append(f"{fp}: invalid lang '{item.get('lang')}' in item {i}")
        if item.get("section") not in ("docs", "grammar", "book"):
            errors.append(f"{fp}: invalid section '{item.get('section')}' in item {i}")

if (DOCS / "search-index.json").exists():
    data = json.loads((DOCS / "search-index.json").read_text(encoding="utf-8"))
    total = len(data.get("pages", []))
    if total < 20:
        errors.append(f"global index coverage too low: {total} pages (min 20)")

if errors:
    print("search index check failed")
    for e in errors:
        print("-", e)
    sys.exit(1)

print("search index check ok")
