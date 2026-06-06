#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

LEVEL_RE = re.compile(r"Level:\s*(Beginner|Intermediate|Advanced)\.?\s*$", re.M)
H1_RE = re.compile(r"<h1\b", re.I)
FENCE_RE = re.compile(r"```.*?```", re.S)
TAG_RE = re.compile(r"<[^>]+>")

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
    "commented-examples.html",
}


def strip_html(text: str) -> str:
    return TAG_RE.sub(" ", text)


def has_heading(text: str, title: str) -> bool:
    pattern = rf"<h2\b[^>]*>\s*{re.escape(title)}\s*</h2>"
    return re.search(pattern, text, flags=re.I | re.S) is not None


def has_any_heading(text: str, titles: list[str]) -> bool:
    return any(has_heading(text, title) for title in titles)


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
        if not has_heading(t, "Concrete Problem"):
            warnings.append(f"{md}: missing chapter section Concrete Problem")
        if not has_heading(t, "Red Thread (Single Project)"):
            warnings.append(f"{md}: missing chapter section Red Thread (Single Project)")
        if not has_heading(t, "For what"):
            warnings.append(f"{md}: missing chapter section For what")
        if not has_heading(t, "What you are going to do"):
            warnings.append(f"{md}: missing chapter section What you are going to do")
        if not has_any_heading(t, ["Minimal example", "Coherent example"]):
            warnings.append(f"{md}: missing chapter section Minimal example or Coherent example")
        if not has_heading(t, "Global explanation"):
            warnings.append(f"{md}: missing chapter section Global explanation")
        if not has_heading(t, "Common pitfalls"):
            warnings.append(f"{md}: missing chapter section Common pitfalls")
        if not has_heading(t, "Short exercise"):
            warnings.append(f"{md}: missing chapter section Short exercise")
        if not has_heading(t, "Summary in 5 points"):
            warnings.append(f"{md}: missing chapter section Summary in 5 points")
        lower_scan = scan.lower()
        if "reading line by line" in lower_scan or "<h2>line by line" in lower_scan or "<h3>line by line" in lower_scan:
            warnings.append(f"{md}: avoid line-by-line explanations; prefer global reading")
        if "step-by-step explanation" in lower_scan:
            warnings.append(f"{md}: replace step-by-step explanation with global explanation")
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
        lower_scan = scan.lower()
        if "reading line by line" in lower_scan or "<h2>line by line" in lower_scan or "<h3>line by line" in lower_scan:
            warnings.append(f"{md}: avoid line-by-line commentary in keyword pages")
        if "example in c" not in lower_scan and "comparison with c" not in lower_scan:
            warnings.append(f"{md}: add a short C comparison for keyword context")
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
