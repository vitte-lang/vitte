#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

TARGETS = [
    Path('docs/book/chapters/01-demarrer.md'),
    Path('docs/book/chapters/02-philosophie.md'),
    Path('docs/book/chapters/03-projet.md'),
    Path('docs/book/chapters/04-syntaxe.md'),
    Path('docs/book/chapters/05-types.md'),
    Path('docs/book/chapters/06-procedures.md'),
    Path('docs/book/chapters/07-controle.md'),
    Path('docs/book/chapters/08-structures.md'),
    Path('docs/book/chapters/09-modules.md'),
    Path('docs/book/chapters/10-diagnostics.md'),
]

AUTO_REP_RE = re.compile(
    r"\n?<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->.*?<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->\n?",
    re.S,
)

GENERIC_TAIL_RE = re.compile(
    r"\n## Objectif\n.*?(?=\n## Checkpoint synthèse|\n\Z)",
    re.S,
)


def clean(text: str) -> str:
    t = text

    # Remove generated repetitive block and generic tail section.
    t = AUTO_REP_RE.sub("\n", t)
    t = GENERIC_TAIL_RE.sub("\n", t)

    # Humanize headings and repeated markers.
    t = t.replace("## Ce que vous allez faire", "## Ce que vous allez réellement faire")
    t = t.replace("## Explication pas à pas", "## Méthode de lecture")
    t = t.replace("Lecture ligne par ligne (débutant):", "Lecture simple du code:")
    t = t.replace("Entrée -> sortie (à vérifier):", "Ce qu'on vérifie en pratique:")

    # Humanize recurring QA line.
    t = t.replace("Test mental: que se passe-t-il si l'entrée est invalide ?", "Question utile: que se passe-t-il si l'entrée est invalide ?")
    t = t.replace("Réponse attendue:", "Repère:")

    # Reduce over-formal phrasing.
    t = t.replace("Dans une lecture de production, ce choix réduit le coût mental:", "En pratique, ce choix simplifie la lecture:")
    t = t.replace("Erreurs fréquentes à éviter:", "Erreurs classiques à éviter:")

    # Trim duplicate blank lines.
    t = re.sub(r"\n{3,}", "\n\n", t)
    return t.rstrip() + "\n"


def main() -> int:
    changed = 0
    for p in TARGETS:
        src = p.read_text(encoding='utf-8', errors='ignore')
        dst = clean(src)
        if dst != src:
            p.write_text(dst, encoding='utf-8')
            changed += 1
    print(f"rewritten={changed}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
