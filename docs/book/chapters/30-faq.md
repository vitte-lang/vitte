# 30. FAQ

Niveau: Avancé.

Prérequis: chapitre précédent `docs/book/chapters/29-style.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/29-style.md`, `docs/book/chapters/27-grammaire.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


## Niveau local

- Niveau local section coeur: Avancé.
- Niveau local exemples guidés: Intermédiaire.
- Niveau local exercices de diagnostic: Avancé.

Ce chapitre répond aux incidents de build les plus fréquents avec un diagnostic par couche. L'objectif n'est pas de mémoriser des messages d'erreur, mais de savoir localiser la cause et appliquer une correction minimale fiable.

Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.

## 30.1 FAQ Parseur vs Backend

Question: pourquoi `expected top-level declaration` et `undefined symbol` n'ont pas la même cause ?

Réponse courte:
- `expected top-level declaration`: erreur de parsing/grammaire (forme syntaxique invalide).
- `undefined symbol`: parsing réussi, mais résolution ou édition de liens échoue (symbole absent).

Exemple parseur (non compilable):

```vit
emit 10 + 20
# erreur: `emit` est un statement, pas une déclaration top-level.
```

Exemple backend/link (parse OK, link KO):

```vit
entry main at app/core {
  return unknown_fn(42)
}
# erreur: symbole inconnu à la résolution ou au link.
```

## 30.2 Pourquoi ça parse mais ça ne link pas ?

Checklist courte:
- runtime manquant (symboles attendus par le backend non fournis).
- symbole natif absent (fonction déclarée mais non définie).
- ABI incohérente (signature/calling convention incompatible).
- ordre d'édition de liens incorrect (objet/lib non inclus).

Règle pratique: si `parse/resolve/ir` passent et que l'échec arrive en `backend/clang`, chercher d'abord dans l'intégration runtime.

## 30.3 EBNF source of truth

Source normative:
- `src/vitte/grammar/vitte.ebnf`.

Copie documentaire:
- `docs/book/grammar-surface.ebnf`.

Politique d'alignement:
- toute évolution de la grammaire source doit être répliquée dans la copie doc.
- les exemples des chapitres doivent rester compatibles avec cette grammaire.

## 30.4 FAQ `emit` vs `return`

Question: quand utiliser `emit` et quand utiliser `return` ?

Réponse:
- `emit`: statement dans un bloc, destiné à émettre une valeur/trace.
- `return`: sortie explicite d'un `entry` ou d'une `proc`.

Exemple compilable:

```vit
entry main at app/print {
  return 10 + 20 * 3
}
```

Exemple non compilable:

```vit
emit 10 + 20 * 3
# erreur: statement top-level invalide.
```

## 30.5 Tableau Erreur -> couche responsable

| Erreur observée | Couche responsable principale |
| --- | --- |
| token illégal | lexing |
| `expected top-level declaration` | parsing |
| `unknown symbol` | resolve |
| incohérence de forme IR | ir |
| génération C++ invalide | backend |
| erreur clang (`undefined`, headers, link) | clang |

## 30.6 FAQ Projet kernel

Points critiques kernel:
- interruptions: cohérence `interrupts.s` et interfaces runtime.
- headers runtime: présence et compatibilité (`vitte_runtime.hpp`, types de base).
- mode freestanding: éviter les dépendances userland implicites.

Règle: valider d'abord le contrat ABI et les symboles exportés avant d'optimiser.

## 30.7 Commandes diagnostics minimales

Commandes utiles:
- `vitte build <fichier.vitte>`: pipeline complet.
- lire le log de stage (`parse`, `resolve`, `ir`, `backend`).
- `python3 docs/book/scripts/qa_book.py`: QA doc standard.
- `python3 docs/book/scripts/qa_book.py --strict`: QA CI stricte.

## 30.8 Cinq scénarios de debug pas à pas

1. Input: statement top-level (`emit ...`).
- Message: `expected top-level declaration`.
- Correction: encapsuler dans `entry` ou `proc`.

2. Input: appel fonction non définie.
- Message: `undefined symbol`.
- Correction: définir la fonction ou corriger l'import/module.

3. Input: `use` dans un bloc.
- Message: erreur de parsing sur `use`.
- Correction: déplacer `use` au top-level.

4. Input: `make` utilisé comme expression.
- Message: forme inattendue après `=`.
- Correction: `make` en statement (`make x as T = expr`) ou remplacer par `let`.

5. Input: `trait` avec champ incomplet.
- Message: token inattendu dans `trait`.
- Correction: utiliser la forme déclarative conforme (`field_list`) ou adapter la grammaire source.

## 30.9 Minimal reproducer (FAQ)

<<< reproducer parse >>>
```vit
emit 1
# casse: statement top-level.
```

Fix minimal:

```vit
entry main at app/repro {
  emit 1
  return 0
}
```

<<< reproducer link >>>
```vit
entry main at app/repro {
  return native_missing(1)
}
# parse OK, link KO.
```

Fix minimal:
- définir `native_missing` dans le runtime ciblé.
- ou remplacer l’appel par un symbole existant.

## 30.10 FAQ mots-clés top-level seulement

Matrice rapide:

| Mot-clé | Top-level | Stmt | Expr |
| --- | --- | --- | --- |
| `space` | oui | non | non |
| `pull` | oui | non | non |
| `use` | oui | non | non |
| `share` | oui | non | non |
| `const` | oui | non | non |
| `make` | oui/global | oui/local | non |
| `emit` | non | oui | non |
| `return` | non | oui | non |

Top-level uniquement (dans cette grammaire):
- `space`, `pull`, `use`, `share`, `const`, `let` (global), `make` (global), `type`, `form`, `pick`, `proc`, `entry`, `macro`.

Statement-level (dans un bloc):
- `let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `when ... is ...`, `return`.

## 30.11 Plan de triage en 90 secondes

1. Lire la première erreur, pas la cascade.
2. Identifier la couche (`parse` vs `resolve` vs `backend/clang`).
3. Réduire au plus petit input reproductible.
4. Corriger une seule cause, relancer le build.
5. Vérifier qu'aucune régression n'apparaît dans les stages précédents.

## Résolution des exercices

Exercice A: fichier avec `emit` top-level.
- Correction attendue:

```vit
entry main at app/fix {
  emit 1
  return 0
}
```

Exercice B: `make` utilisé dans `let x = make 0`.
- Correction attendue:

```vit
proc ok() -> int {
  make x as int = 0
  give x
}
```

Exercice C: `use` dans une procédure.
- Correction attendue:

```vit
use std.io.{read}
proc ok() -> int {
  give 0
}
```

Exercice D: `pull as core/math`.
- Correction attendue:

```vit
pull core/math as math
```

Exercice E: `trait` avec champ sans type.
- Correction attendue:

```vit
trait Pair {
  left: int,
  right: int
}
```

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `u32`, `u64` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/keywords/emit.md`.
- `docs/book/keywords/return.md`.
- `docs/book/keywords/space.md`.
- `docs/book/keywords/use.md`.
- `docs/book/keywords/make.md`.

## Objectif

Savoir diagnostiquer rapidement un échec de build en distinguant couche syntaxique, couche sémantique et couche backend/runtime.

## Exemple

Exemple concret: `emit` top-level échoue au parseur; `return` dans `entry` compile; symbole manquant échoue au link.

## Pourquoi

Cette structure évite les corrections “au hasard” et réduit le temps de triage sur incidents réels.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: identifier la première couche en faute, appliquer la correction minimale locale, puis relancer le pipeline complet.

## À faire

1. Reproduisez un échec parseur et corrigez-le en moins de 2 modifications.
2. Reproduisez un échec link (`undefined symbol`) et documentez la cause exacte.
3. Exécutez `qa_book.py` puis `qa_book.py --strict` et comparez les écarts.

## Corrigé minimal

- Parseur: corriger la forme grammaticale avant toute hypothèse runtime.
- Link/backend: vérifier symboles, runtime et ABI avant refactor.
- Documentation: maintenir l'alignement `src/vitte/grammar/vitte.ebnf` <-> `docs/book/grammar-surface.ebnf`.

## Checkpoint synthèse

Mini quiz:
1. Quelle différence de cause entre `expected top-level declaration` et `undefined symbol` ?
2. Quel fichier est la source normative EBNF ?
3. Quelle est la première action dans un triage 90 secondes ?
