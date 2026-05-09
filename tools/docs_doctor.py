#!/usr/bin/env python3
from pathlib import Path
import json
import re
import sys

STRICT_CSP = "default-src 'self'; img-src 'self' data:; style-src 'self'; script-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none';"

errors = []
warnings = []


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="ignore")


def check_exists(paths):
    for p in paths:
        if not Path(p).exists():
            errors.append(f"missing required file: {p}")


def check_json(paths):
    for p in paths:
        fp = Path(p)
        if not fp.exists():
            continue
        try:
            json.loads(read(fp))
        except Exception:
            errors.append(f"invalid json: {p}")


def check_csp(page):
    fp = Path(page)
    if not fp.exists():
        errors.append(f"missing {page}")
        return
    s = read(fp)
    csps = re.findall(r'<meta[^>]+http-equiv="Content-Security-Policy"[^>]*>', s, flags=re.I)
    if len(csps) != 1:
        errors.append(f"{page}: expected exactly 1 CSP meta, found {len(csps)}")
    if "unsafe-inline" in s:
        errors.append(f"{page}: contains unsafe-inline")
    m = re.search(r'Content-Security-Policy" content="([^"]+)"', s)
    if not m:
        errors.append(f"{page}: no CSP content")
        return
    if m.group(1).strip() != STRICT_CSP:
        warnings.append(f"{page}: CSP differs from strict baseline")


def check_language_policy():
    for lang in ["de", "es", "it", "pt", "zh"]:
        lp = Path("docs") / lang
        if lp.exists():
            errors.append(f"disallowed internal language directory present: {lp}")


def check_seo_pages():
    for page in ["docs/index.html", "docs/doc.html"]:
        fp = Path(page)
        if not fp.exists():
            continue
        s = read(fp)
        if 'rel="canonical"' not in s:
            warnings.append(f"{page}: missing canonical")
        if 'hreflang="en"' not in s:
            warnings.append(f"{page}: missing hreflang alternates")


check_exists([
    "docs/checksums.txt",
    "docs/ebnf.sha256",
    "docs/search-index.json",
    "docs/search-index.docs.json",
    "docs/search-index.book.json",
    "docs/search-index.grammar.json",
    "docs/sitemap.xml",
    "docs/robots.txt",
    "docs/build-manifest.json",
    "docs/status.json",
    "docs/grammar/grammar.sha256",
    "docs/grammar/rules.json",
    "docs/grammar/status.html",
    "docs/docs-architecture.html",
    "docs/RELEASE_NOTES_TEMPLATE.md",
])

check_json([
    "docs/build-manifest.json",
    "docs/search-index.json",
    "docs/search-index.docs.json",
    "docs/search-index.book.json",
    "docs/search-index.grammar.json",
    "docs/grammar/rules.json",
    "docs/status.json",
])

for p in ["docs/index.html", "docs/doc.html", "docs/grammar/toolbox.html"]:
    check_csp(p)

check_language_policy()
check_seo_pages()

if errors:
    print("DOCS DOCTOR FAILED")
    for e in errors:
        print(f"- {e}")
    if warnings:
        print("\nWarnings:")
        for w in warnings:
            print(f"- {w}")
    print("\nSuggested fix order:")
    print("1) python3 tools/build_docs_site.py")
    print("2) python3 tools/build_grammar_extras.py")
    print("3) python3 tools/sync_ebnf_memory_pages.py")
    print("4) python3 tools/build_static_extras.py")
    sys.exit(1)

print("DOCS DOCTOR OK")
if warnings:
    print("Warnings:")
    for w in warnings:
        print(f"- {w}")
