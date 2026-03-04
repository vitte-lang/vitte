#!/usr/bin/env python3
"""Normalize docs/book/keywords to a strict editorial template."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import argparse
import re

SKIP = {
    "README.md",
    "couverture.md",
    "parcours.md",
    "packs-apprentissage.md",
    "non-utilises.md",
    "erreurs-compilateur.md",
}

ORDER = [
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

SECTION_RE = re.compile(r"^## .*$", re.MULTILINE)


def split_sections(text: str) -> tuple[str, str, dict[str, str]]:
    lines = text.splitlines()
    title = lines[0] if lines else ""
    niveau = ""
    for line in lines[1:6]:
        if line.startswith("Niveau:"):
            niveau = line
            break

    parts = SECTION_RE.split(text)
    heads = SECTION_RE.findall(text)
    sections: dict[str, str] = {}
    for i, head in enumerate(heads):
        body = parts[i + 1].strip("\n")
        sections[head.strip()] = body.strip()

    return title.strip(), niveau.strip(), sections


def ensure_section(sections: dict[str, str], heading: str, keyword: str) -> str:
    if heading in sections and sections[heading].strip():
        return sections[heading].strip()

    base = keyword.strip("`")
    if heading == "## Lecture rapide (30s)":
        return (
            "- Ce que c’est: ce mot-clé exprime une intention précise dans le flux Vitte.\n"
            "- Quand l’utiliser: quand il rend la lecture du contrat plus directe.\n"
            "- Erreur classique: l’utiliser au mauvais niveau (top-level vs bloc)."
        )
    if heading == "## Pourquoi (métier)":
        return (
            f"`{base}` réduit l’ambiguïté dans le code de production.\n\n"
            "Vous l’utilisez pour rendre la règle métier explicite dès la lecture.\n"
            "Cela simplifie les revues et accélère le diagnostic en cas d’erreur.\n"
            "Le but est un comportement stable, lisible et testable."
        )
    if heading == "## Différences proches":
        return (
            "| Mot-clé proche | Différence clé |\n"
            "| --- | --- |\n"
            "| `À compléter` | Positionner la différence opérationnelle en une phrase. |"
        )
    if heading == "## Refactor rapide":
        return (
            "Avant:\n"
            "```vit\n"
            "# usage fragile à corriger\n"
            "```\n\n"
            "Après:\n"
            "```vit\n"
            "# usage clair et testable\n"
            "```"
        )
    if heading == "## Exemple nominal":
        return (
            "Entrée:\n"
            "- Cas nominal simple.\n\n"
            "```vit\n"
            f"# exemple minimal avec `{base}`\n"
            "```\n\n"
            "Sortie observable:\n"
            "- Résultat attendu stable et vérifiable."
        )
    if heading == "## Exemple invalide":
        return (
            "Entrée:\n"
            "- Cas hors contrat minimal.\n\n"
            "```vit\n"
            f"# contre-exemple avec `{base}`\n"
            "```\n\n"
            "Diagnostic attendu:\n"
            "- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).\n"
            "- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).\n"
            "- Message: motif stable orienté correction."
        )
    if heading == "## Score de complétude":
        return "coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5"
    if heading == "## Utilisé dans les chapitres":
        return "- `docs/book/chapters/27-grammaire.md`.\n- `docs/book/chapters/31-erreurs-build.md`.\n- `docs/book/chapters/07-controle.md`."
    if heading == "## Voir aussi":
        return "- `docs/book/keywords/erreurs-compilateur.md`.\n- `book/glossaire.md`."
    if heading == "## Définition":
        return f"`{base}` est un mot-clé Vitte. Cette fiche vous donne sa forme, son usage et ses erreurs courantes."
    if heading == "## Syntaxe":
        return "Forme canonique: `à préciser`."
    if heading == "## Pièges":
        return "- L’utiliser hors contexte.\n- Masquer l’intention métier.\n- Oublier un contre-exemple testable."
    if heading == "## Quand l’utiliser / Quand l’éviter":
        return "- Quand l’utiliser: si ce mot-clé clarifie le contrat.\n- Quand l’éviter: si une forme plus simple existe."
    if heading == "## Erreurs compilateur fréquentes":
        return "| Message type | Cause | Correction |\n| --- | --- | --- |\n| `VITTE-XXXX` | Cause fréquente à préciser. | Correction rapide à appliquer. |"
    if heading == "## Mot-clé voisin":
        return "| Mot-clé | Différence opérationnelle |\n| --- | --- |\n| `À compléter` | Différence à préciser. |"
    return "À compléter."


def enrich_quick_read(body: str) -> str:
    lines = [l for l in body.splitlines() if l.strip()]
    bullets = [l for l in lines if l.lstrip().startswith("- ")]
    if len(bullets) >= 3:
        return body.strip()
    extra = [
        "- Ce que c’est: ce mot-clé exprime une intention précise dans le flux Vitte.",
        "- Quand l’utiliser: quand il rend la lecture du contrat plus directe.",
        "- Erreur classique: l’utiliser au mauvais niveau (top-level vs bloc).",
    ]
    merged = bullets + [e for e in extra if e not in bullets]
    return "\n".join(merged[:3])


def enrich_invalid_example(body: str, keyword: str) -> str:
    text = body.strip()
    if "Diagnostic attendu:" not in text:
        text += (
            "\n\nDiagnostic attendu:\n"
            "- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).\n"
            "- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).\n"
            "- Message: motif stable orienté correction."
        )
    if "VITTE-" not in text and "E000" not in text:
        text += "\n- Code: `VITTE-XXXX`."
    low = text.lower()
    if "ligne" not in low or "colonne" not in low:
        text += "\n- Position attendue: `ligne 1, colonne 1`."
    return text


def normalize_chapter_links(body: str) -> str:
    links: list[str] = []
    for raw in body.splitlines():
        m = re.search(r"`((?:docs/)?book/chapters/[0-9a-z\\-]+\\.md)`", raw)
        if not m:
            continue
        path = m.group(1)
        if path.startswith("book/chapters/"):
            path = "docs/" + path
        if path not in links:
            links.append(path)
    defaults = [
        "docs/book/chapters/07-controle.md",
        "docs/book/chapters/27-grammaire.md",
        "docs/book/chapters/31-erreurs-build.md",
    ]
    for d in defaults:
        if len(links) >= 3:
            break
        if d not in links:
            links.append(d)
    links = links[:5]
    return "\n".join(f"- `{p}`." for p in links)


def normalize_see_also(body: str) -> str:
    lines = [l.strip() for l in body.splitlines() if l.strip().startswith("- ")]
    if not any("docs/book/keywords/" in l for l in lines):
        lines.insert(0, "- `docs/book/keywords/erreurs-compilateur.md`.")
    if not lines:
        lines = [
            "- `docs/book/keywords/erreurs-compilateur.md`.",
            "- `book/glossaire.md`.",
        ]
    return "\n".join(lines)


def compute_score(sections: dict[str, str]) -> str:
    checks = []
    checks.append("## Syntaxe" in sections and "Forme canonique" in sections["## Syntaxe"])
    checks.append("## Exemple nominal" in sections and "```vit" in sections["## Exemple nominal"])
    checks.append("## Exemple invalide" in sections and "```vit" in sections["## Exemple invalide"])
    inv = sections.get("## Exemple invalide", "")
    checks.append("VITTE-" in inv or "E000" in inv)
    used = sections.get("## Utilisé dans les chapitres", "")
    checks.append(used.count("- ") >= 3)
    score = sum(1 for c in checks if c)
    return f"coverage: syntaxe/exemples/invalides/diagnostics/liens = {score}/5"


def normalize_file(path: Path) -> bool:
    text = path.read_text(encoding="utf-8")
    title, niveau, sections = split_sections(text)

    if not title:
        title = f"# Mot-clé `{path.stem}`"
    if not niveau:
        niveau = "Niveau: Intermédiaire."

    # Reuse existing "Mot-clé voisin" section as fallback for "Différences proches".
    if "## Différences proches" not in sections and "## Mot-clé voisin" in sections:
        sections["## Différences proches"] = sections["## Mot-clé voisin"]

    out = [title, "", niveau, ""]
    for heading in ORDER:
        body = ensure_section(sections, heading, path.stem)
        if heading == "## Lecture rapide (30s)":
            body = enrich_quick_read(body)
        elif heading == "## Exemple invalide":
            body = enrich_invalid_example(body, path.stem)
        elif heading == "## Utilisé dans les chapitres":
            body = normalize_chapter_links(body)
        elif heading == "## Voir aussi":
            body = normalize_see_also(body)
        if heading == "## Score de complétude":
            body = compute_score({**sections, heading: body})
        out.append(heading)
        out.append("")
        out.append(body.strip())
        out.append("")

    new_text = "\n".join(out).rstrip() + "\n"
    if new_text != text:
        path.write_text(new_text, encoding="utf-8")
        return True
    return False


def main() -> int:
    parser = argparse.ArgumentParser(description="Normalize keyword docs to strict template")
    args = parser.parse_args()

    repo = Path(__file__).resolve().parents[3]
    root = repo / "book" / "keywords"
    changed = 0
    for path in sorted(root.glob("*.md")):
        if path.name in SKIP:
            continue
        if normalize_file(path):
            changed += 1
    print(f"[keywords-normalize] changed={changed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
