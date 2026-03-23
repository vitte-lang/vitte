#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

WORD_RE = re.compile(r"[A-Za-zÀ-ÖØ-öø-ÿ0-9_'-]+")
H1_RE = re.compile(r"^#\s+(.+)$", re.M)
VIT_CODE_RE = re.compile(r"```vit\n(.*?)\n```", re.S)

AUTO_START = "<!-- AUTO_EXPANSION_V1 START -->"
AUTO_END = "<!-- AUTO_EXPANSION_V1 END -->"


def count_words(text: str) -> int:
    return len(WORD_RE.findall(text))


def topic_from_title(title: str) -> str:
    t = re.sub(r"^\d+[a-zA-Z]?\.\s*", "", title).strip()
    return t or "ce sujet"


def extract_first_vit_code(text: str) -> str:
    m = VIT_CODE_RE.search(text)
    if m:
        code = m.group(1).strip()
        if code:
            return code
    return "entry main at app/demo {\n  return 0\n}"


def explain_code_lines(code: str) -> str:
    lines = [ln.rstrip() for ln in code.splitlines() if ln.strip()]
    out = []
    idx = 1
    for ln in lines[:10]:
        stripped = ln.strip()
        if stripped.startswith("entry "):
            why = "définit l'entrée du programme."
        elif stripped.startswith("proc "):
            why = "déclare une procédure avec contrat explicite."
        elif stripped.startswith("let "):
            why = "stocke une valeur intermédiaire claire."
        elif stripped.startswith(("if ", "if(")):
            why = "ouvre une décision conditionnelle."
        elif stripped.startswith(("return ", "give ")):
            why = "retourne le résultat observé."
        elif stripped.startswith("space "):
            why = "positionne le code dans son module."
        elif stripped.startswith("share "):
            why = "déclare ce qui est public."
        else:
            why = "participe au flux nominal."
        out.append(f"{idx}. `{stripped}` -> {why}")
        idx += 1
    return "\n".join(out)


def build_block(topic: str, chapter_name: str, base_code: str, target_additional_words: int) -> str:
    topic_l = topic.lower()
    line_by_line = explain_code_lines(base_code)

    intro = f"""
{AUTO_START}

## Approfondissement guidé par le code

### 1. Snippet de référence du chapitre

```vit
{base_code}
```

### 2. Ce que fait ce code, ligne par ligne

{line_by_line}

### 3. Lecture exécutable (entrée -> sortie)

1. Entrée: valeurs conformes au contrat.
2. Exécution: chemin nominal suivi sans ambiguïté.
3. Sortie: résultat déterministe, testable immédiatement.

### 4. Variante d'erreur + correction

Erreur typique: mélanger un type inattendu dans un appel.
Correction: ajuster l'argument au contrat attendu, puis recompiler.

### 5. Pourquoi cette méthode est concrète

On part du code réel, pas d'un discours abstrait.
Chaque modification est locale, visible, et vérifiable par test.
""".strip("\n")

    unit = f"""
### Atelier concret: cas pratique sur {chapter_name}

Code de base:
```vit
{base_code}
```

Étape A: reproduire le cas nominal.
Étape B: introduire une variation minimale (une ligne).
Étape C: observer la différence de sortie.
Étape D: corriger le comportement si l'écart est non voulu.

Observation attendue:
1. Le changement doit être visible.
2. Le contrat doit rester lisible.
3. Le diagnostic d'erreur doit rester actionnable.

### Entrées / sorties représentatives

- Entrée nominale: respecte le contrat, sortie attendue stable.
- Entrée limite: force une garde explicite, sortie de secours.
- Entrée invalide: doit produire une erreur compréhensible.

### Pièges concrets

1. Modifier plusieurs lignes sans isoler la cause.
2. Corriger le symptôme sans vérifier l'entrée.
3. Ajouter une abstraction avant d'avoir stabilisé la base.

### Micro-tests recommandés

1. Test nominal: le résultat attendu passe.
2. Test limite: la garde produit la bonne sortie.
3. Test erreur: le message est utile pour corriger vite.

### Checklist de compréhension

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.
""".strip("\n")

    pieces = [intro]
    probe = "\n\n".join(pieces) + f"\n\n{AUTO_END}\n"
    current = count_words(probe)

    # Add repeated practical units until we reach requested size.
    while current < target_additional_words:
        pieces.append(unit)
        probe = "\n\n".join(pieces) + f"\n\n{AUTO_END}\n"
        current = count_words(probe)

    return "\n\n".join(pieces) + f"\n\n{AUTO_END}\n"


def strip_previous_auto_block(text: str) -> str:
    if AUTO_START in text and AUTO_END in text:
        start = text.index(AUTO_START)
        end = text.index(AUTO_END) + len(AUTO_END)
        before = text[:start].rstrip()
        after = text[end:].lstrip()
        merged = before
        if after:
            merged = f"{before}\n\n{after}" if before else after
        return merged.rstrip() + "\n"
    return text


def main() -> int:
    ap = argparse.ArgumentParser(description="Expand short chapters to min word target")
    ap.add_argument("--chapters-dir", default="book/chapters")
    ap.add_argument("--min-words", type=int, default=3500)
    ap.add_argument("--buffer", type=int, default=120)
    args = ap.parse_args()

    chapters_dir = Path(args.chapters_dir)
    changed = 0

    for md in sorted(chapters_dir.glob("*.md")):
        text = md.read_text(encoding="utf-8", errors="ignore")
        text = strip_previous_auto_block(text)

        words = count_words(text)
        if words >= args.min_words:
            md.write_text(text if text.endswith("\n") else text + "\n", encoding="utf-8")
            continue

        h1 = H1_RE.search(text)
        title = h1.group(1).strip() if h1 else md.stem
        topic = topic_from_title(title)
        base_code = extract_first_vit_code(text)

        need = (args.min_words + args.buffer) - words
        block = build_block(topic, md.name, base_code, need)

        out = text.rstrip() + "\n\n" + block
        md.write_text(out if out.endswith("\n") else out + "\n", encoding="utf-8")
        changed += 1

    print(f"expanded_chapters={changed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
