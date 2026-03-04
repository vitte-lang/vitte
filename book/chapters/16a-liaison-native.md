# 16a. Liaison native avec `std/bridge`

Niveau: Intermédiaire.

Prérequis: chapitre précédent `docs/book/chapters/16-interop.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/17-stdlib.md`, `book/chapters/23-projet-sys.md`, `book/chapters/30-faq.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Liaison native avec `std/bridge`**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Liaison native avec `std/bridge`**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Liaison native avec `std/bridge`**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Liaison native avec `std/bridge`** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Liaison native avec `std/bridge`**.

## Exercice court

Prenez un exemple du chapitre sur **Liaison native avec `std/bridge`**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Liaison native avec `std/bridge`**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

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
