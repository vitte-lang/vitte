#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

CHAPTERS = Path('docs/book/chapters')

AUTO_EXPAND_RE = re.compile(
    r"\n?<!-- AUTO_EXPANSION_V1 START -->.*?<!-- AUTO_EXPANSION_V1 END -->\n?",
    re.S,
)

LINE_RE = re.compile(
    r"^(\d+)\.\s*`([^`]+)`\s*->\s*Comportement:\s*(.*?)\s*->\s*Preuve:\s*(.*?)\s*$"
)


def clean_text(t: str) -> str:
    # Remove verbose auto-expansion blocks.
    t = AUTO_EXPAND_RE.sub("\n", t)

    out = []
    lines = t.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]

        m = LINE_RE.match(line)
        if m:
            n, code, comportement, preuve = m.groups()
            comportement = comportement.strip().rstrip('.')
            preuve = preuve.strip().rstrip('.')
            out.append(f"{n}. `{code}` : {comportement}. Vérification: {preuve}.")
            i += 1
            continue

        # Replace repetitive block intro with concise phrasing.
        if line.strip() == "Mini tableau Entrée -> Sortie (exemples):":
            out.append("Entrée -> sortie (à vérifier):")
            i += 1
            continue

        # Trim one highly repetitive sentence.
        if line.strip().startswith("Ce déroulé concret sert de preuve locale"):
            i += 1
            continue

        out.append(line)
        i += 1

    cleaned = "\n".join(out)

    # Collapse excessive blank lines.
    cleaned = re.sub(r"\n{3,}", "\n\n", cleaned)
    return cleaned.rstrip() + "\n"


def main() -> int:
    changed = 0
    for md in sorted(CHAPTERS.glob('*.md')):
        original = md.read_text(encoding='utf-8', errors='ignore')
        new = clean_text(original)
        if new != original:
            md.write_text(new, encoding='utf-8')
            changed += 1
    print(f"humanized={changed}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
