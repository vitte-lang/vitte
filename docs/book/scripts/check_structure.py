#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

LEVEL_RE = re.compile(r"Level:\s*(Beginner|Intermediate|Advanced)\.?\s*$", re.M)
H1_RE = re.compile(r"<h1\b", re.I)
FENCE_RE = re.compile(r"```.*?```", re.S)
TAG_RE = re.compile(r"<[^>]+>")

CHAPTER_REQUIRED = [
    "Concrete Problem",
    "Red Thread (Single Project)",
    "For what",
    "What you are going to do",
    "Minimal example",
    "Step by step explanation",
    "Common pitfalls",
    "Short exercise",
    "Summary in 5 points",
]

KEYWORD_REQUIRED = [
    "Quick reading",
    "Why (job)",
    "Definition",
    "Syntax",
    "Algorithmic reading",
    "Nominal example",
    "Invalid example",
    "Quick refactor",
    "Pitfalls",
    "When to use it / When to avoid it",
    "Common compiler errors",
    "Neighbor keyword",
    "Used in chapters",
    "See also",
    "Completeness score",
]

KEYWORD_SKIP = {
    "index.html",
    "couverture.html",
    "parcours.html",
    "packs-apprentissage.html",
    "non-utilises.html",
    "erreurs-compilateur.html",
    "all.html",
}


def strip_html(text: str) -> str:
    return TAG_RE.sub(" ", text)


def has_heading(text: str, title: str) -> bool:
    pattern = rf"<h2\b[^>]*>\s*{re.escape(title)}\s*</h2>"
    return re.search(pattern, text, flags=re.I | re.S) is not None


def is_redirect_page(text: str) -> bool:
    return 'http-equiv="refresh"' in text.lower() and 'rel="canonical"' in text.lower()


def main() -> int:
    ap = argparse.ArgumentParser(description="Structure checks for rendered HTML book chapters/keywords")
    ap.add_argument("--book-root", default="book")
    ap.add_argument("--strict", action="store_true", help="fail on all warnings")
    args = ap.parse_args()

    root = Path(args.book_root).resolve()
    chapters_dir = root / "chapters"
    keywords_dir = chapters_dir / "keywords"

    errors: list[str] = []
    warnings: list[str] = []

    for md in sorted(chapters_dir.glob("*.html")):
        t = md.read_text(encoding="utf-8", errors="ignore")
        if is_redirect_page(t):
            continue
        scan = strip_html(FENCE_RE.sub("", t))
        h1 = len(H1_RE.findall(t))
        if h1 != 1:
            errors.append(f"{md}: expected exactly one H1, got {h1}")
        for sec in CHAPTER_REQUIRED:
            if not has_heading(t, sec):
                warnings.append(f"{md}: missing chapter section {sec}")
        if "Prerequisites:" not in scan:
            warnings.append(f"{md}: missing 'Prerequisites:'")
        if "See also:" not in scan:
            warnings.append(f"{md}: missing 'See also:'")

    for md in sorted(keywords_dir.glob("*.html")):
        if md.name in KEYWORD_SKIP:
            continue
        t = md.read_text(encoding="utf-8", errors="ignore")
        if is_redirect_page(t):
            continue
        scan = strip_html(FENCE_RE.sub("", t))
        h1 = len(H1_RE.findall(t))
        if h1 != 1:
            errors.append(f"{md}: expected exactly one H1, got {h1}")
        if not LEVEL_RE.search(scan):
            warnings.append(f"{md}: missing/invalid level banner")
        for sec in KEYWORD_REQUIRED:
            if not has_heading(t, sec):
                warnings.append(f"{md}: missing keyword section {sec}")
        if has_heading(t, "Nearby keyword") and has_heading(t, "Neighbor keyword"):
            warnings.append(f"{md}: redundant near-keyword naming")

    print(f"[book-structure] chapters={len(list(chapters_dir.glob('*.html')))} keywords={len(list(keywords_dir.glob('*.html')))}")
    print(f"[book-structure] errors={len(errors)} warnings={len(warnings)}")
    for e in errors:
        print(f"[error] {e}")
    for w in warnings[:200]:
        print(f"[warn] {w}")

    if errors:
        return 1
    if args.strict and warnings:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
