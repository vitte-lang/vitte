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
    t = re.sub(r"^\d+[a-zA-Z]?\.\s*", "", title).strip()
    return t or "Sujet"


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
    for ln in lines[:8]:
        stripped = ln.strip()
        if stripped.startswith("entry "):
            why = "définit le point d'entrée exécutable."
        elif stripped.startswith("proc "):
            why = "déclare un contrat clair (entrées/sortie)."
        elif stripped.startswith("let "):
            why = "fixe une valeur intermédiaire réutilisable."
        elif stripped.startswith(("if ", "if(")):
            why = "ouvre une branche conditionnelle lisible."
        elif stripped.startswith(("return ", "give ")):
            why = "renvoie une valeur observable et testable."
        elif stripped.startswith("share "):
            why = "expose explicitement l'API publique."
        elif stripped.startswith("space "):
            why = "positionne le code dans son module."
        else:
            why = "participe au flux nominal du programme."
        out.append(f"{idx}. `{stripped}` -> {why}")
        idx += 1
    return "\n".join(out)


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
    line_by_line = explain_code_lines(base_code)
    return f"""{START}

## Exemples représentatifs (par cas d'usage)

Cette section s'appuie sur du code concret pour **{topic_l}**.
Objectif: comprendre vite ce que fait le code, pourquoi, et comment le corriger.

### Exemple 1: extrait réel du chapitre (cas nominal)

```vit
{base_code}
```

Lecture guidée (ligne par ligne):
{line_by_line}

Entrée -> Sortie attendue:
1. Entrée: données conformes au contrat.
2. Traitement: chemin nominal exécuté.
3. Sortie: valeur déterministe observable.

### Exemple 2: garde explicite (cas limite)

```vit
proc clamp_non_negative(x: int) -> int {{
  if x < 0 {{
    give 0
  }}
  give x
}}
```

Quand l'utiliser: éviter les comportements implicites sur entrées hors contrat.

### Exemple 3: erreur de type volontaire (diagnostic)

```vit
proc needs_int(x: int) -> int {{
  give x
}}
entry main at app/demo {{
  let s: string = "42"
  return needs_int(s)
}}
```

Quand l'utiliser: entraîner la lecture des diagnostics compilateur.

### Exemple 4: séparation module / API

```vit
space app/math
proc add(a: int, b: int) -> int {{
  give a + b
}}
share add
```

Quand l'utiliser: clarifier ce qui est public vs interne dans l'architecture.

### Exemple 5: flux de contrôle lisible

```vit
entry main at app/demo {{
  let n: int = 3
  if n > 0 {{
    return 1
  }}
  return 0
}}
```

Quand l'utiliser: expliciter une décision métier avec un chemin nominal et un fallback.

### Exemple 6: version testable d'une procédure

```vit
proc is_even(x: int) -> bool {{
  give x % 2 == 0
}}
```

Cas de test conseillés:
1. `is_even(2)` -> `true`.
2. `is_even(3)` -> `false`.
3. `is_even(0)` -> `true`.

Quand l'utiliser: convertir rapidement une règle en contrat vérifiable.

### Exemple 7: refactor sûr (avant/après)

Avant:
```vit
proc parse_port(s: string) -> int {{
  give 0
}}
```

Après:
```vit
proc parse_port(s: string) -> int {{
  if s == "" {{
    give 0
  }}
  give 8080
}}
```

Quand l'utiliser: faire évoluer le comportement sans casser la signature publique.

### Exemple 8: correction guidée basée sur le code

Procédure de correction:
1. Reproduire le bug sur un snippet minimal.
2. Corriger une seule ligne.
3. Recompiler et vérifier la sortie.
4. Ajouter un test de non-régression.

### Checklist de lecture rapide

1. Où est le contrat d'entrée?
2. Quel est le chemin nominal?
3. Quel est le cas limite traité?
4. Quelle erreur reste explicite?
5. Quel test prouve le comportement?

{END}
"""


def main() -> int:
    ap = argparse.ArgumentParser(description="Add representative examples block to chapters")
    ap.add_argument("--chapters-dir", default="book/chapters")
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
