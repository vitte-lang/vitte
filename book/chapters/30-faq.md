# 30. FAQ

Niveau: Avancé.

Prérequis: chapitre précédent `book/chapters/29-style.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/29-style.md`, `book/chapters/27-grammaire.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **FAQ**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **FAQ**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **FAQ**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **FAQ** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **FAQ**.

## Exercice court

Prenez un exemple du chapitre sur **FAQ**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **FAQ**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## Niveau local

- Niveau local section coeur: Avancé.
- Niveau local exemples guidés: Intermédiaire.
- Niveau local exercices de diagnostic: Avancé.

Ce chapitre répond aux incidents de build les plus fréquents avec un diagnostic par couche. L'objectif n'est pas de mémoriser des messages d'erreur, mais de savoir localiser la cause et appliquer une correction minimale fiable.

Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `book/checklist-editoriale.md`. Complément: `book/erreurs-classiques.md`.

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

Règle pratique: si `parse/resolve/ir` passent et que l'échec arrive en `backend/cpp`, chercher d'abord dans l'intégration runtime.

## 30.3 EBNF source of truth

Source normative:
- `src/vitte/grammar/vitte.ebnf`.

Copie documentaire:
- `book/grammar-surface.ebnf`.
- `docs/grammar/vitte.ebnf`.

Politique d'alignement:
- toute évolution de la grammaire source doit être répliquée dans la copie doc.
- les exemples des chapitres doivent rester compatibles avec cette grammaire.
- synchronisation automatique: `python3 book/scripts/sync_grammar_surface.py`.
- validation bloquante: `python3 book/scripts/sync_grammar_surface.py --check`.

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
| erreur toolchain C++ (`undefined`, headers, link) | c++ |

## 30.6 FAQ Projet kernel

Points critiques kernel:
- interruptions: cohérence `interrupts.s` et interfaces runtime.
- headers runtime: présence et compatibilité (`vitte_runtime.hpp`, types de base).
- mode freestanding: éviter les dépendances userland implicites.

Règle: valider d'abord le contrat ABI et les symboles exportés avant d'optimiser.

## 30.7 Commandes diagnostics minimales

Commandes utiles:
- `vitte build <fichier.vitte>`: pipeline complet.
- `vitte check <fichier.vit>`: parse + resolve + IR sans backend.
- `vitte build --stage parse|resolve|ir|backend <fichier.vit>`: isolation de couche.
- `vitte check --diag-json <fichier.vit>`: diagnostics structurés CI/IDE.
- `vitte check --dump-ast <fichier.vit>` et `vitte check --dump-ir <fichier.vit>`: debug déterministe.
- `vitte reduce <fichier.vit>`: reproducer minimal automatique.
- lire le log de stage (`parse`, `resolve`, `ir`, `backend`).
- `python3 book/scripts/qa_book.py`: QA doc standard.
- `python3 book/scripts/qa_book.py --strict`: QA CI stricte.

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
2. Identifier la couche (`parse` vs `resolve` vs `backend/cpp`).
3. Réduire au plus petit input reproductible.
4. Corriger une seule cause, relancer le build.
5. Vérifier qu'aucune régression n'apparaît dans les stages précédents.

## 30.12 `int` vs tailles fixes

Règle éditoriale et technique de ce livre:
- `int`: type canonique pour les exemples généraux.
- `i32/i64/i128/u32/u64/u128`: à utiliser dès que la largeur binaire est contractuelle.

Cas où il faut éviter `int`:
- API binaire, réseau, fichiers, liaison native, kernel/freestanding.

## 30.13 Playbook debug: erreur -> reproducer -> fix -> rerun

1. Capturer la première erreur avec son code (`E000x/E100x/E200x/E300x`).
2. Réduire le fichier via `vitte reduce`.
3. Corriger uniquement la cause racine.
4. Relancer: `--stage` ciblé, puis `check`, puis `build`.
5. Rejouer QA/tests: `grammar-check`, `book-qa-strict`, `negative-tests`, `diag-snapshots`.

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
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/emit.md`.
- `book/keywords/return.md`.
- `book/keywords/space.md`.
- `book/keywords/use.md`.
- `book/keywords/make.md`.

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
- Documentation: maintenir l'alignement `src/vitte/grammar/vitte.ebnf` <-> `book/grammar-surface.ebnf`.

## Checkpoint synthèse

Mini quiz:
1. Quelle différence de cause entre `expected top-level declaration` et `undefined symbol` ?
2. Quel fichier est la source normative EBNF ?
3. Quelle est la première action dans un triage 90 secondes ?

<!-- AUTO_EXPANSION_V1 START -->

## Approfondissement concret (sans répétition)

### 1. Snippet de référence

```vit
emit 10 + 20
# erreur: `emit` est un statement, pas une déclaration top-level.
```

### 2. Lecture du code ligne par ligne

1. `emit 10 + 20` -> participe au flux principal du traitement.
2. `# erreur: `emit` est un statement, pas une déclaration top-level.` -> participe au flux principal du traitement.

### 3. Exécution réelle (entrée -> traitement -> sortie)

1. Entrée: préciser les valeurs acceptées et refusées.
2. Traitement: suivre le chemin nominal, puis la première garde.
3. Sortie: vérifier la valeur retournée ou l'erreur attendue.

### 4. Cas limite et erreur volontaire

- Cas limite: forcer la garde et confirmer la sortie de secours.
- Cas erreur: injecter un type inattendu et lire le diagnostic exact.
- Correction: modifier une seule ligne, recompiler, valider.

### 5. Refactor concret à faible risque

Méthode: garder la signature, simplifier une branche, et prouver que le comportement reste identique avec un test nominal + un test limite.

### 6. Série de scénarios représentatifs

Cas 1: pour **faq**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 2: pour **faq**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 3: pour **faq**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 4: pour **faq**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 5: pour **faq**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 6: pour **faq**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 7: pour **faq**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 8: pour **faq**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 9: pour **faq**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 10: pour **faq**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 11: pour **faq**, inspecter l'axe 'stabilité des appels' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 12: pour **faq**, inspecter l'axe 'lisibilité du module' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 13: pour **faq**, inspecter l'axe 'robustesse en refactor' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 14: pour **faq**, inspecter l'axe 'stabilité du comportement' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 15: pour **faq**, inspecter l'axe 'qualité du diagnostic' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 16: pour **faq**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 17: pour **faq**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 18: pour **faq**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 19: pour **faq**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 20: pour **faq**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 21: pour **faq**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 22: pour **faq**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 23: pour **faq**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 24: pour **faq**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 25: pour **faq**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 26: pour **faq**, inspecter l'axe 'stabilité des appels' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 27: pour **faq**, inspecter l'axe 'lisibilité du module' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 28: pour **faq**, inspecter l'axe 'robustesse en refactor' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 29: pour **faq**, inspecter l'axe 'stabilité du comportement' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 30: pour **faq**, inspecter l'axe 'qualité du diagnostic' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 31: pour **faq**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 32: pour **faq**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 33: pour **faq**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 34: pour **faq**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 35: pour **faq**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 36: pour **faq**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 37: pour **faq**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 38: pour **faq**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 39: pour **faq**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 40: pour **faq**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.

### 7. Checklist finale de compréhension

1. Le contrat d'entrée est explicite.
2. Le cas nominal est testable sans ambiguïté.
3. Le cas limite est traité explicitement.
4. Le diagnostic d'erreur est actionnable.
5. Le corrigé suit une modification locale et vérifiable.

<!-- AUTO_EXPANSION_V1 END -->

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **faq**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
emit 10 + 20
# erreur: `emit` est un statement, pas une déclaration top-level.
```

Lecture ligne par ligne:
1. `emit 10 + 20` -> participe au déroulé du traitement.
2. `# erreur: `emit` est un statement, pas une déclaration top-level.` -> participe au déroulé du traitement.

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

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->
