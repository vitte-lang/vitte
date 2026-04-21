#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

LINK_RE = re.compile(r"\[[^\]]+\]\(([^)]+)\)")
FENCE_RE = re.compile(r"```.*?```", re.S)
INLINE_CODE_RE = re.compile(r"`[^`]*`")


def iter_markdown(root: Path):
    for p in sorted(root.rglob("*.md")):
        yield p


def resolve_target(src: Path, target: str, book_root: Path, repo_root: Path) -> bool:
    if not target or target.startswith(("http://", "https://", "mailto:", "#")):
        return True
    target = target.split("#", 1)[0].strip()
    if not target:
        return True

    candidates = [
        (src.parent / target).resolve(),
        (book_root / target).resolve(),
        (repo_root / target).resolve(),
    ]
    return any(c.exists() for c in candidates)


def main() -> int:
    parser = argparse.ArgumentParser(description="Check markdown internal links under docs/book/")
    parser.add_argument("--book-root", default="docs/book", help="book root directory")
    parser.add_argument("--report", default="target/reports/book_links_report.txt", help="report output path")
    args = parser.parse_args()

    book_root = Path(args.book_root).resolve()
    repo_root = book_root.parents[1]
    report_path = Path(args.report)

    issues: list[str] = []
    files = list(iter_markdown(book_root))

    for md in files:
        text = md.read_text(encoding="utf-8", errors="ignore")
        text = FENCE_RE.sub("", text)
        lines = text.splitlines()
        for i, line in enumerate(lines, start=1):
            scan = INLINE_CODE_RE.sub("", line)
            for m in LINK_RE.finditer(scan):
                target = m.group(1).strip()
                if not resolve_target(md, target, book_root, repo_root):
                    issues.append(f"{md}:{i}: broken link -> {target}")

    report_path.parent.mkdir(parents=True, exist_ok=True)
    out = []
    out.append("[book-links] report")
    out.append(f"files={len(files)}")
    out.append(f"issues={len(issues)}")
    out.append("")
    out.extend(issues)
    report_path.write_text("\n".join(out) + "\n", encoding="utf-8")

    if issues:
        print(f"[book-links] FAIL issues={len(issues)} report={report_path}")
        return 1
    print(f"[book-links] OK files={len(files)} report={report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
