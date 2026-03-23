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

## Approfondissement guidé

### 1. Ce qu'il faut vraiment retenir

Le coeur de **faq** est de prendre des décisions lisibles et vérifiables.
Dans un projet réel, la compréhension rapide prime sur la complexité apparente.
L'objectif de cette section est de transformer le chapitre en guide opérationnel,
pas en résumé théorique.

Trois idées pratiques gouvernent ce sujet:
1. faire un changement à la fois;
2. garder des invariants explicites;
3. valider le résultat avec une preuve simple (test, sortie, diagnostic).

### 2. Carte mentale utilisable en équipe

Quand vous travaillez sur **faq**, posez systématiquement ces questions:
- quel est le contrat d'entrée;
- quel est le résultat attendu;
- quels sont les cas limites visibles;
- quelle erreur doit être compréhensible en moins de 30 secondes.

Cette carte mentale évite les refactors fragiles.
Elle permet aussi d'aligner débutants et profils avancés sur le même langage de travail.

### Étude de cas pratique

Cas: un module lié à **faq** (30-faq.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **faq** (30-faq.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **faq** (30-faq.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **faq** (30-faq.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **faq** (30-faq.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

### Étude de cas pratique

Cas: un module lié à **faq** (30-faq.md) doit évoluer sans casser l'existant.
On commence par figer le comportement nominal avec un exemple concret,
puis on introduit une variation contrôlée.

Étape 1: définir un scénario simple, reproductible, et documenté.
Étape 2: identifier un seul point d'évolution.
Étape 3: appliquer la modification en conservant les invariants.
Étape 4: observer la sortie et les diagnostics.
Étape 5: corriger immédiatement l'écart le plus proche de la cause.

Cette méthode paraît lente, mais elle réduit fortement les régressions.
Elle accélère la livraison au niveau du sprint, car les retours arrière diminuent.

### Anti-patterns à éviter

1. Changer la structure et le comportement dans le même commit.
2. Ajouter des options avant d'avoir validé le cas nominal.
3. Masquer les erreurs derrière des valeurs par défaut silencieuses.
4. Empiler des exceptions sans règle de priorisation.
5. Écrire la documentation après coup sans trace de décision.

### Questions de revue (pair review)

- Le lecteur comprend-il le flux en une seule lecture?
- Le code expose-t-il clairement le contrat attendu?
- Les erreurs sont-elles actionnables?
- Le test couvre-t-il un cas nominal et un cas limite?
- Le changement est-il réversible sans risque majeur?

### Exercice guidé

Exercice A:
- Reprendre l'exemple principal du chapitre.
- Ajouter un cas limite explicite.
- Mesurer l'impact du changement.

Exercice B:
- Introduire une erreur volontaire.
- Lire le diagnostic exact.
- Corriger uniquement la première cause détectée.

Exercice C:
- Simplifier une partie du code sans changer le comportement.
- Vérifier que les tests restent verts.
- Expliquer en 5 lignes pourquoi la nouvelle version est plus maintenable.

### Corrigé détaillé (méthode)

Un corrigé solide commence par les invariants:
- ce qui doit toujours rester vrai;
- ce qui peut varier;
- ce qui doit échouer explicitement.

Ensuite, on trace la preuve minimale:
1. une entrée claire;
2. une transformation observable;
3. une sortie vérifiable.

Enfin, on documente les limites connues.
La transparence sur les limites augmente la qualité perçue du chapitre,
car le lecteur sait où s'arrête la garantie.

### Checklist de mise en production

- Contrat d'entrée explicite.
- Cas nominal validé.
- Cas limite validé.
- Erreurs lisibles.
- Section "À faire" exécutable.
- Corrigé minimal cohérent.
- Lien vers chapitre voisin pertinent.

<!-- AUTO_EXPANSION_V1 END -->

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs (par cas d'usage)

Cette section donne des exemples variés et réalistes pour **faq**.
Objectif: multiplier les angles de lecture sans alourdir le noyau du chapitre.

### Exemple 1: cas nominal minimal

```vit
entry main at app/demo {
  return 0
}
```

Quand l'utiliser: valider la base exécutable avant tout ajout de complexité.

### Exemple 2: garde explicite (cas limite)

```vit
proc clamp_non_negative(x: int) -> int {
  if x < 0 {
    give 0
  }
  give x
}
```

Quand l'utiliser: éviter les comportements implicites sur entrées hors contrat.

### Exemple 3: erreur de type volontaire (diagnostic)

```vit
proc needs_int(x: int) -> int {
  give x
}
entry main at app/demo {
  let s: string = "42"
  return needs_int(s)
}
```

Quand l'utiliser: entraîner la lecture des diagnostics compilateur.

### Exemple 4: séparation module / API

```vit
space app/math
proc add(a: int, b: int) -> int {
  give a + b
}
share add
```

Quand l'utiliser: clarifier ce qui est public vs interne dans l'architecture.

### Exemple 5: flux de contrôle lisible

```vit
entry main at app/demo {
  let n: int = 3
  if n > 0 {
    return 1
  }
  return 0
}
```

Quand l'utiliser: expliciter une décision métier avec un chemin nominal et un fallback.

### Exemple 6: version testable d'une procédure

```vit
proc is_even(x: int) -> bool {
  give x % 2 == 0
}
```

Cas de test conseillés:
1. `is_even(2)` -> `true`.
2. `is_even(3)` -> `false`.
3. `is_even(0)` -> `true`.

Quand l'utiliser: convertir rapidement une règle en contrat vérifiable.

### Exemple 7: refactor sûr (avant/après)

Avant:
```vit
proc parse_port(s: string) -> int {
  give 0
}
```

Après:
```vit
proc parse_port(s: string) -> int {
  if s == "" {
    give 0
  }
  give 8080
}
```

Quand l'utiliser: faire évoluer le comportement sans casser la signature publique.

### Exemple 8: checklist de lecture rapide

1. Où est le contrat d'entrée?
2. Quel est le chemin nominal?
3. Quel est le cas limite traité?
4. Quelle erreur reste explicite?
5. Quel test prouve le comportement?

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 END -->
