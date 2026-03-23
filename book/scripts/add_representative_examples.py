#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
from pathlib import Path

H1_RE = re.compile(r"^#\s+(.+)$", re.M)

START = "<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->"
END = "<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->"


def clean_topic(title: str) -> str:
    t = re.sub(r"^\d+[a-zA-Z]?\.\s*", "", title).strip()
    return t or "Sujet"


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


def build_examples_block(topic: str) -> str:
    topic_l = topic.lower()
    return f"""{START}

## Exemples représentatifs (par cas d'usage)

Cette section donne des exemples variés et réalistes pour **{topic_l}**.
Objectif: multiplier les angles de lecture sans alourdir le noyau du chapitre.

### Exemple 1: cas nominal minimal

```vit
entry main at app/demo {{
  return 0
}}
```

Quand l'utiliser: valider la base exécutable avant tout ajout de complexité.

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

### Exemple 8: checklist de lecture rapide

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

        out = text.rstrip() + "\n\n" + build_examples_block(topic).rstrip() + "\n"
        md.write_text(out, encoding="utf-8")
        changed += 1

    print(f"examples_added={changed}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
