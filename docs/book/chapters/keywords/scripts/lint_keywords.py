#!/usr/bin/env python3
"""Lint keyword documentation quality gates."""

from __future__ import annotations

from pathlib import Path
import re
import sys

SKIP = {
    "README.md",
    "couverture.md",
    "parcours.md",
    "packs-apprentissage.md",
    "non-utilises.md",
    "erreurs-compilateur.md",
}

REQUIRED = [
    "## Quick read (30s)",
    "## Why it matters",
    "## Definition",
    "## Syntax",
    "## Nominal example",
    "## Invalid example",
    "## Nearby differences",
    "## Quick refactor",
    "## Pitfalls",
    "## When to use it / When to avoid it",
    "## Common compiler errors",
    "## Neighbor keyword",
    "## Used in chapters",
    "## See also",
    "## Completeness score",
]

CH_LINK_RE = re.compile(r"`(docs/book/chapters/[0-9a-z\-]+\.md)`")
KW_LINK_RE = re.compile(r"`(docs/book/chapters/keywords/[a-z0-9\-]+\.md)`")


def section_body(text: str, heading: str) -> str:
    pat = re.compile(rf"{re.escape(heading)}\n\n(.*?)(?=\n## |\Z)", re.DOTALL)
    m = pat.search(text)
    return m.group(1).strip() if m else ""


def main() -> int:
    repo = Path(__file__).resolve().parents[3]
    root = repo / "book" / "keywords"
    errors: list[str] = []

    files = [p for p in sorted(root.glob("*.md")) if p.name not in SKIP]
    for p in files:
        t = p.read_text(encoding="utf-8")
        for heading in REQUIRED:
            if heading not in t:
                errors.append(f"{p}: missing section {heading}")

        quick = section_body(t, "## Quick read (30s)")
        if quick.count("- ") < 3:
            errors.append(f"{p}: Quick read must contain 3 bullets")

        inval = section_body(t, "## Invalid example")
        if "VITTE-" not in inval and "E000" not in inval:
            errors.append(f"{p}: Invalid example missing diagnostic code (VITTE-XXXX or E000X)")
        if "line" not in inval.lower() or "column" not in inval.lower():
            errors.append(f"{p}: Invalid example missing expected line/column")

        used = section_body(t, "## Used in chapters")
        links = CH_LINK_RE.findall(used)
        if len(links) < 3 or len(links) > 5:
            errors.append(f"{p}: Used in chapters must contain 3..5 chapter links (found {len(links)})")

        see = section_body(t, "## See also")
        kw_links = KW_LINK_RE.findall(see)
        if not kw_links:
            errors.append(f"{p}: See also should reference at least one keyword file")

        score = section_body(t, "## Completeness score")
        if not re.search(r"coverage:\s*syntax/examples/invalid/diagnostics/links\s*=\s*[0-5]/5", score):
            errors.append(f"{p}: invalid score format")

    if errors:
        print("[keywords-lint] FAILED")
        for e in errors:
            print(f"- {e}")
        return 1

    print(f"[keywords-lint] OK files={len(files)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
