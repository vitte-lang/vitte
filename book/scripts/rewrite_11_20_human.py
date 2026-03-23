#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

TARGETS = [
    Path('book/chapters/11-collections.md'),
    Path('book/chapters/12-pointeurs.md'),
    Path('book/chapters/13-generiques.md'),
    Path('book/chapters/14-macros.md'),
    Path('book/chapters/15-pipeline.md'),
    Path('book/chapters/16-interop.md'),
    Path('book/chapters/16a-liaison-native.md'),
    Path('book/chapters/17-stdlib.md'),
    Path('book/chapters/18-tests.md'),
    Path('book/chapters/19-performance.md'),
    Path('book/chapters/20-repro.md'),
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
    t = AUTO_REP_RE.sub("\n", t)
    t = GENERIC_TAIL_RE.sub("\n", t)

    t = t.replace("## Ce que vous allez faire", "## Ce que vous allez réellement faire")
    t = t.replace("## Explication pas à pas", "## Méthode de lecture")
    t = t.replace("Lecture ligne par ligne (débutant):", "Lecture simple du code:")
    t = t.replace("Entrée -> sortie (à vérifier):", "Ce qu'on vérifie en pratique:")
    t = t.replace("Test mental: que se passe-t-il si l'entrée est invalide ?", "Question utile: que se passe-t-il si l'entrée est invalide ?")
    t = t.replace("Réponse attendue:", "Repère:")
    t = t.replace("Dans une lecture de production, ce choix réduit le coût mental:", "En pratique, ce choix simplifie la lecture:")
    t = t.replace("Erreurs fréquentes à éviter:", "Erreurs classiques à éviter:")

    # Keep required section if missing after cleanup.
    if "\n## Objectif\n" not in t and "\n## Pourquoi\n" in t:
        t = t.replace(
            "\n## Pourquoi\n",
            "\n## Objectif\n\nComprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.\n\n## Pourquoi\n",
            1,
        )

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
