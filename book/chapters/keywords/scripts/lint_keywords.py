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
    "## Lecture rapide (30s)",
    "## Pourquoi (métier)",
    "## Définition",
    "## Syntaxe",
    "## Exemple nominal",
    "## Exemple invalide",
    "## Différences proches",
    "## Refactor rapide",
    "## Pièges",
    "## Quand l’utiliser / Quand l’éviter",
    "## Erreurs compilateur fréquentes",
    "## Mot-clé voisin",
    "## Utilisé dans les chapitres",
    "## Voir aussi",
    "## Score de complétude",
]

CH_LINK_RE = re.compile(r"`(docs/book/chapters/[0-9a-z\-]+\.md)`")
KW_LINK_RE = re.compile(r"`(docs/book/keywords/[a-z0-9\-]+\.md)`")


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

        quick = section_body(t, "## Lecture rapide (30s)")
        if quick.count("- ") < 3:
            errors.append(f"{p}: Lecture rapide must contain 3 bullets")

        inval = section_body(t, "## Exemple invalide")
        if "VITTE-" not in inval and "E000" not in inval:
            errors.append(f"{p}: Exemple invalide missing diagnostic code (VITTE-XXXX or E000X)")
        if "ligne" not in inval.lower() or "colonne" not in inval.lower():
            errors.append(f"{p}: Exemple invalide missing expected line/column")

        used = section_body(t, "## Utilisé dans les chapitres")
        links = CH_LINK_RE.findall(used)
        if len(links) < 3 or len(links) > 5:
            errors.append(f"{p}: Utilisé dans les chapitres must contain 3..5 chapter links (found {len(links)})")

        see = section_body(t, "## Voir aussi")
        kw_links = KW_LINK_RE.findall(see)
        if not kw_links:
            errors.append(f"{p}: Voir aussi should reference at least one keyword file")

        score = section_body(t, "## Score de complétude")
        if not re.search(r"coverage:\s*syntaxe/exemples/invalides/diagnostics/liens\s*=\s*[0-5]/5", score):
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
