# 16a. Liaison native avec `std/bridge`

Niveau: Intermédiaire.

Prérequis: chapitre précédent `docs/book/chapters/16-interop.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/17-stdlib.md`, `docs/book/chapters/23-projet-sys.md`, `docs/book/chapters/30-faq.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.

Ce chapitre introduit une façade stable pour la liaison native: `std/bridge/*`. L’idée est simple: garder des noms métier Vitte (`io`, `memory`, `system`, `time`, `net`) au lieu d’exposer directement une terminologie C.

Noms métier directs disponibles:
- `std/bridge/print`
- `std/bridge/read`
- `std/bridge/path`
- `std/bridge/env`
- `std/bridge/bytes`
- `std/bridge/alloc`
- `std/bridge/process`

Pipeline de liaison:
- Entrée: appel métier (`cwd`, `now`, `print_line`, etc.).
- Traitement: façade `std/bridge/*` vers modules std existants.
- Sortie: même comportement observable, avec vocabulaire homogène.
- Invariant: le code applicatif dépend d’une API Vitte stable.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: `use` reste au niveau module, jamais dans un bloc.
- Statements: `let`, `give`, `return` restent dans les blocs de `proc`/`entry`.
- Types primaires: `int`, `i32`, `u32`, `string`, `bool` suivent `type_primary`.

## Keywords à revoir

- `docs/book/keywords/use.md`.
- `docs/book/keywords/proc.md`.
- `docs/book/keywords/entry.md`.
- `docs/book/keywords/give.md`.
- `docs/book/keywords/return.md`.

## Objectif

Construire un point d’entrée unique pour la liaison native afin de réduire les dépendances directes à des détails d’implémentation.

## Exemple

```vit
use std/bridge/io
use std/bridge/system
use std/bridge/time

entry main at app/bridge_demo {
  let home = get("HOME")
  let here = cwd()
  let t0 = now()
  let _ = print_line("bridge ready")
  return 0
}
```

Lecture ligne par ligne:
1. `use std/bridge/io` importe la façade d’E/S métier; le code appelant n’a pas besoin de connaître `std/io/*`.
2. `use std/bridge/system` importe la façade système (`env`, `path`, `process`, `os`) dans un point unique.
3. `use std/bridge/time` importe l’accès temporel sans coupler le code à un backend précis.
4. `entry main at app/bridge_demo {` définit le point d’exécution du programme.
5. `let home = get("HOME")` lit une variable d’environnement via la façade système.
6. `let here = cwd()` récupère le répertoire courant via la même façade.
7. `let t0 = now()` capture un instant via la façade temps.
8. `let _ = print_line("bridge ready")` produit une sortie texte via la façade I/O.
9. `return 0` termine proprement le programme.
10. `}` ferme le bloc d’entrée.

## Pourquoi

Une façade de liaison évite la dispersion des appels natifs dans tout le code. Elle améliore la lisibilité, simplifie les migrations internes et stabilise les contrats pour les projets.

## Test mental

Question: que se passe-t-il si l’entrée est invalide ?
Réponse attendue: la validation doit être traitée dans la façade ou juste avant l’appel, pas dupliquée dans chaque appelant.

## À faire

1. Remplacer dans un module applicatif un import direct (`std/io/*` ou `std/os/*`) par `std/bridge/*`.
2. Vérifier que le comportement observable ne change pas (mêmes sorties, même code retour).
3. Identifier une règle d’invariant à conserver (exemple: pas d’appel natif direct hors façade).

## Corrigé minimal

- Import centralisé:
  - `use std/bridge/io`
  - `use std/bridge/system`
- Appels inchangés côté métier (`get`, `cwd`, `print_line`).
- Invariant respecté: un seul point d’abstraction pour la liaison native.
