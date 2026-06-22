#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

H1_RE = re.compile(r"^#\s+(.+)$", re.M)
VIT_CODE_RE = re.compile(r"```vit\n(.*?)\n```", re.S)

START = "<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->"
END = "<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->"


def clean_topic(title: str) -> str:
    return re.sub(r"^\d+[a-zA-Z]?\.\s*", "", title).strip() or "Sujet"


def extract_first_vit_code(text: str) -> str:
    m = VIT_CODE_RE.search(text)
    if m:
        code = m.group(1).strip()
        if code:
            return code
    return "entry main at app/demo {\n  return 0\n}"


def global_reading_summary(code: str) -> str:
    lines = [ln.strip() for ln in code.splitlines() if ln.strip()]
    if not lines:
        return (
            "Le code doit être lu comme un scénario complet: entrée, transformation, "
            "sortie vérifiable, puis cas limite explicite."
        )

    has_entry = any(ln.startswith("entry ") for ln in lines)
    has_proc = any(ln.startswith("proc ") for ln in lines)
    has_guard = any(ln.startswith(("if ", "if(")) for ln in lines)
    has_return = any(ln.startswith(("return ", "give ")) for ln in lines)

    parts = []
    if has_entry:
        parts.append("un point d'entrée exécutable")
    if has_proc:
        parts.append("des procédures au contrat explicite")
    if has_guard:
        parts.append("des gardes qui séparent nominal et cas limite")
    if has_return:
        parts.append("une sortie observable")

    if not parts:
        parts.append("une structure minimale à compléter")

    joined = ", ".join(parts[:-1]) + (" et " + parts[-1] if len(parts) > 1 else parts[0])
    return (
        "Le lecteur ne doit pas commenter chaque ligne isolément. "
        f"Il doit d'abord identifier {joined}, puis vérifier comment ces blocs coopèrent "
        "pour produire un comportement stable."
    )


def strip_old_block(text: str) -> str:
    if START in text and END in text:
        i = text.index(START)
        j = text.index(END) + len(END)
        before = text[:i].rstrip()
        after = text[j:].lstrip()
        if before and after:
            return before + "\n\n" + after
        return (before or after).rstrip() + "\n"
    return text


def build_examples_block(topic: str, base_code: str) -> str:
    topic_l = topic.lower()
    global_summary = global_reading_summary(base_code)

    return f"""{START}

## Exemples représentatifs basés sur le code du chapitre

Thème: **{topic_l}**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: code complet du chapitre

```vit
{base_code}
```

### Explication globale

{global_summary}

### Ce que ce code doit prouver

- Le scénario principal reste compréhensible sans paraphrase ligne par ligne.
- Le lecteur peut nommer l'entrée, la transformation et la sortie.
- Le cas limite a une place visible dans le flux.

### Exemple B: variante cas limite (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant une garde explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter une garde avant cette ligne.
3. Vérifier la nouvelle sortie sur une entrée limite.

### Exemple C: bug reproductible puis correction locale

Procédure:
1. Introduire une incompatibilité de type sur un appel.
2. Compiler et lire le premier diagnostic.
3. Corriger une seule ligne (pas de refactor global).
4. Recompiler et vérifier le retour nominal.

### Résultat attendu

- Le lecteur comprend ce que fait le code sans abstraction inutile.
- Chaque exemple est relié à une action concrète.
- La correction est reproductible et testable.
- Le chapitre explique un flux complet, pas une succession de micro-commentaires.

{END}
"""


def main() -> int:
    ap = argparse.ArgumentParser(description="Add concrete representative examples based on chapter code")
    ap.add_argument("--chapters-dir", default="docs/book/chapters")
    args = ap.parse_args()

    changed = 0
    for md in sorted(Path(args.chapters_dir).glob("*.md")):
        text = md.read_text(encoding="utf-8", errors="ignore")
        text = strip_old_block(text).rstrip() + "\n"

        h1 = H1_RE.search(text)
        title = h1.group(1).strip() if h1 else md.stem
        topic = clean_topic(title)
        base_code = extract_first_vit_code(text)

        out = text.rstrip() + "\n\n" + build_examples_block(topic, base_code).rstrip() + "\n"
        md.write_text(out, encoding="utf-8")
        changed += 1

    print(f"examples_added={changed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
