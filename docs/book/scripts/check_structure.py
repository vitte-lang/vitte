#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

LEVEL_RE = re.compile(r"^Niveau:\s*(Débutant|Intermédiaire|Avancé)\.?\s*$", re.M)
H1_RE = re.compile(r"^#\s+.+$", re.M)
FENCE_RE = re.compile(r"```.*?```", re.S)

CHAPTER_REQUIRED = [
    "## Objectif",
    "## Exemple",
    "## Pourquoi",
    "## Test mental",
    "## À faire",
    "## Corrigé minimal",
]

KEYWORD_REQUIRED = [
    "## Définition",
    "## Syntaxe",
    "## Exemple nominal",
    "## Exemple invalide",
    "## Pièges",
    "## Voir aussi",
    "## Quand l’utiliser / Quand l’éviter",
    "## Erreurs compilateur fréquentes",
    "## Mot-clé voisin",
    "## Utilisé dans les chapitres",
]

KEYWORD_SKIP = {
    "README.md",
    "couverture.md",
    "parcours.md",
    "packs-apprentissage.md",
    "non-utilises.md",
    "erreurs-compilateur.md",
    "all.md",
}


def main() -> int:
    ap = argparse.ArgumentParser(description="Structure checks for book chapters/keywords")
    ap.add_argument("--book-root", default="book")
    ap.add_argument("--strict", action="store_true", help="fail on all warnings")
    args = ap.parse_args()

    root = Path(args.book_root).resolve()
    chapters_dir = root / "chapters"
    keywords_dir = chapters_dir / "keywords"

    errors: list[str] = []
    warnings: list[str] = []

    for md in sorted(chapters_dir.glob("*.md")):
        t = md.read_text(encoding="utf-8", errors="ignore")
        scan = FENCE_RE.sub("", t)
        h1 = len(H1_RE.findall(scan))
        if h1 != 1:
            errors.append(f"{md}: expected exactly one H1, got {h1}")
        if "Prérequis:" not in scan:
            warnings.append(f"{md}: missing 'Prérequis:'")
        if "Voir aussi:" not in scan:
            warnings.append(f"{md}: missing 'Voir aussi:'")
        for sec in CHAPTER_REQUIRED:
            if sec not in scan:
                warnings.append(f"{md}: missing chapter section {sec}")

    for md in sorted(keywords_dir.glob("*.md")):
        if md.name in KEYWORD_SKIP:
            continue
        t = md.read_text(encoding="utf-8", errors="ignore")
        scan = FENCE_RE.sub("", t)
        h1 = len(H1_RE.findall(scan))
        if h1 != 1:
            errors.append(f"{md}: expected exactly one H1, got {h1}")
        if not LEVEL_RE.search(scan):
            warnings.append(f"{md}: missing/invalid level banner")
        for sec in KEYWORD_REQUIRED:
            if sec not in scan:
                warnings.append(f"{md}: missing keyword section {sec}")
        if "## Différences proches" in scan:
            warnings.append(f"{md}: redundant section '## Différences proches' (use only '## Mot-clé voisin')")

    print(f"[book-structure] chapters={len(list(chapters_dir.glob('*.md')))} keywords={len(list(keywords_dir.glob('*.md')))}")
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
