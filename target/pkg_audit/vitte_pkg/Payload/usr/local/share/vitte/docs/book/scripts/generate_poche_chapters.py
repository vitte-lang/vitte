#!/usr/bin/env python3
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CHAPTERS_DIR = ROOT / "chapters"
POCHE_DIR = ROOT / "poche"
POCHE_CHAPTERS_DIR = POCHE_DIR / "chapters"

H1_RE = re.compile(r"^#\s+(.+)$", re.M)
VIT_CODE_BLOCK_RE = re.compile(r"```vit\n(.*?)\n```", re.S)


def normalize_title(title: str) -> str:
    return " ".join(title.strip().split())


def pick_first_code(text: str) -> str:
    m = VIT_CODE_BLOCK_RE.search(text)
    if m:
        code = m.group(1).strip()
        if code:
            return code
    return "entry main at app/demo {\n  return 0\n}"


def chapter_number_and_topic(filename: str, title: str) -> tuple[str, str]:
    num = filename.split("-", 1)[0]
    topic = title
    topic = re.sub(r"^\d+[a-zA-Z]?\.\s*", "", topic)
    topic = topic.strip()
    return num, topic or filename


def simplify_topic(topic: str) -> str:
    topic = topic.replace("Vitte", "")
    topic = re.sub(r"\s+", " ", topic).strip(" -")
    topic = re.sub(r"\bavec\b\s*$", "", topic, flags=re.I).strip(" -")
    return topic or "ce sujet"


def build_poche_content(src_name: str, title: str, code: str) -> str:
    num, topic = chapter_number_and_topic(src_name, title)
    short_topic = simplify_topic(topic).lower()
    h1 = f"# {num}. {topic} (version poche)"

    lines = [
        h1,
        "",
        f"Prérequis: `docs/book/poche/SOMMAIRE.md`.",
        f"Voir aussi: `docs/book/chapters/{src_name}`.",
        "",
        "## Objectif",
        "",
        f"Comprendre {short_topic} de manière simple pour agir rapidement.",
        "",
        "## Idée clé en 3 points",
        "",
        "1. Identifier le besoin concret.",
        "2. Appliquer la forme minimale correcte.",
        "3. Vérifier le résultat avec un test court.",
        "",
        "## Quand l'utiliser",
        "",
        "1. Quand vous avez besoin d'une solution lisible immédiatement.",
        "2. Quand vous voulez valider une base avant d'aller plus loin.",
        "3. Quand vous devez expliquer rapidement le sujet à quelqu'un d'autre.",
        "",
        "## Exemple minimal",
        "",
        "```vit",
        code,
        "```",
        "",
        "## Erreurs fréquentes",
        "",
        "1. Vouloir couvrir tous les cas d'un coup.",
        "2. Mélanger plusieurs changements dans la même étape.",
        "3. Oublier de relire le message d'erreur exact.",
        "",
        "## Mini-exercice (5 minutes)",
        "",
        f"Modifiez l'exemple pour créer une variante de {short_topic}, puis vérifiez le résultat attendu.",
        "",
        "## Checklist rapide",
        "",
        "1. Le code compile.",
        "2. Le résultat est compréhensible.",
        "3. Le cas limite principal est testé.",
        "",
        "## Corrigé minimal",
        "",
        "Partir du cas nominal, faire une seule modification, recompiler, puis valider la sortie.",
        "",
        "## Aller plus loin",
        "",
        f"Version complète: `docs/book/chapters/{src_name}`.",
    ]
    return "\n".join(lines) + "\n"


def main() -> int:
    POCHE_CHAPTERS_DIR.mkdir(parents=True, exist_ok=True)
    chapters = sorted(CHAPTERS_DIR.glob("*.md"))

    generated = []
    for src in chapters:
        text = src.read_text(encoding="utf-8", errors="ignore")
        h1 = H1_RE.search(text)
        title = normalize_title(h1.group(1) if h1 else src.stem)
        code = pick_first_code(text)

        dst = POCHE_CHAPTERS_DIR / src.name
        dst.write_text(build_poche_content(src.name, title, code), encoding="utf-8")
        generated.append((src.name, title))

    summary_lines = [
        "# Sommaire Tout-En-Poche",
        "",
        "- [Accueil poche](README.md)",
        "- [Parcours express (10 étapes)](00-comment-lire.md)",
        "",
        "## Parcours express",
        "",
        "- [0. Comment utiliser ce mini-livre](00-comment-lire.md)",
        "- [1. Premier programme](01-premier-programme.md)",
        "- [2. Variables et types](02-variables-et-types.md)",
        "- [3. Conditions et boucles](03-conditions-et-boucles.md)",
        "- [4. Procédures](04-procedures.md)",
        "- [5. Structures de données](05-structures.md)",
        "- [6. Modules simplement](06-modules.md)",
        "- [7. Lire les erreurs](07-lire-les-erreurs.md)",
        "- [8. Tester sans se perdre](08-tests-essentiels.md)",
        "- [9. Mini-projet CLI](09-mini-projet-cli.md)",
        "- [10. Feuille de route personnelle](10-feuille-route.md)",
        "",
        "## Version poche de tous les chapitres",
        "",
    ]

    for src_name, title in generated:
        label = normalize_title(title)
        summary_lines.append(f"- [{label}](chapters/{src_name})")

    (POCHE_DIR / "SOMMAIRE.md").write_text("\n".join(summary_lines) + "\n", encoding="utf-8")

    print(f"generated={len(generated)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
