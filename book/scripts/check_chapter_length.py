#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

WORD_RE = re.compile(r"[A-Za-zÀ-ÖØ-öø-ÿ0-9_'-]+")


def count_words(text: str) -> int:
    return len(WORD_RE.findall(text))


def words_to_pages(words: int, words_per_page: int) -> float:
    return words / float(words_per_page)


def main() -> int:
    ap = argparse.ArgumentParser(description="Check chapter length in paper-page equivalent")
    ap.add_argument("--chapters-dir", default="book/chapters")
    ap.add_argument("--min-pages", type=float, default=10.0)
    ap.add_argument("--max-pages", type=float, default=30.0)
    ap.add_argument("--words-per-page", type=int, default=350)
    ap.add_argument("--report", default="target/reports/chapter_length_report.txt")
    ap.add_argument("--strict", action="store_true", help="return non-zero if any chapter is out of range")
    args = ap.parse_args()

    chapters_dir = Path(args.chapters_dir)
    report_path = Path(args.report)
    report_path.parent.mkdir(parents=True, exist_ok=True)

    min_words = int(args.min_pages * args.words_per_page)
    max_words = int(args.max_pages * args.words_per_page)

    rows = []
    violations = []

    for md in sorted(chapters_dir.glob("*.md")):
        text = md.read_text(encoding="utf-8", errors="ignore")
        words = count_words(text)
        pages = words_to_pages(words, args.words_per_page)
        status = "OK"
        delta_words = 0

        if words < min_words:
            status = "TOO_SHORT"
            delta_words = min_words - words
            violations.append((md, status, words, pages, delta_words))
        elif words > max_words:
            status = "TOO_LONG"
            delta_words = words - max_words
            violations.append((md, status, words, pages, delta_words))

        rows.append((md, status, words, pages, delta_words))

    with report_path.open("w", encoding="utf-8") as f:
        f.write("[chapter-length] report\n")
        f.write(f"chapters={len(rows)}\n")
        f.write(
            f"rule=min_pages:{args.min_pages} max_pages:{args.max_pages} words_per_page:{args.words_per_page} "
            f"(min_words:{min_words} max_words:{max_words})\n"
        )
        f.write(f"violations={len(violations)}\n\n")
        f.write("status\twords\tpages\tdelta_words\tpath\n")
        for md, status, words, pages, delta_words in rows:
            f.write(f"{status}\t{words}\t{pages:.2f}\t{delta_words}\t{md}\n")

    print(
        f"[chapter-length] chapters={len(rows)} violations={len(violations)} "
        f"report={report_path}"
    )

    if args.strict and violations:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
