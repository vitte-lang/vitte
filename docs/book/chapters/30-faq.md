# 30. FAQ

Niveau: Avancé.

Prérequis: chapitre précédent `docs/book/chapters/29-style.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/29-style.md`, `docs/book/chapters/27-grammaire.md`, `docs/book/glossaire.md`.

## Problème Concret

Situation réelle: pour FAQ, la question n'est pas 'quella règle écrire' mais 'quel chemin le code prend vraiment'. Cette lecture par exécution évite les interprétations vagues.
Question directrice: quelle condition est évaluée en premier, et quelle sortie cette décision impose-t-elle ?

## Fil Rouge (Projet Unique)

Fil conducteur: vous retrouvez le même pipeline pour observer ce qui change réellement quand on modifie une branche.
Objectif pédagogique: relire un bloc, prédire la sortie, puis confirmer la prédiction avec une exécution simple et reproductible.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **FAQ**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez lire les extraits dans l'ordre d'exécution réel, puis valider les sorties attendues sur un cas nominal et un cas d'erreur.

## Exemple minimal

Premier réflexe recommandé: lisez d'abord les entrées et les conditions, ensuite seulement la forme syntaxique.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou le test principal.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **FAQ** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas d'erreur dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **FAQ**.

## Exercice court

Prenez un exemple du chapitre sur **FAQ**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au résultat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **FAQ**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas d'erreur.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

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

Règle pratique: si `parse/resolve/ir` passent et que l'échec arrive en `backend/cpp`, chercher d'abord dans l'intégration runtime.

## 30.3 EBNF source of truth

Source normative:
- `src/vitte/grammar/vitte.ebnf`.

Copie documentaire:
- `docs/book/grammar-surface.ebnf`.
- `docs/grammar/vitte.ebnf`.

Politique d'alignement:
- toute évolution de la grammaire source doit être répliquée dans la copie doc.
- les exemples des chapitres doivent rester compatibles avec cette grammaire.
- synchronisation automatique: `python3 docs/book/scripts/sync_grammar_surface.py`.
- validation bloquante: `python3 docs/book/scripts/sync_grammar_surface.py --check`.

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

Règle: valider d'abord la règle ABI et les symboles exportés avant d'optimiser.

## 30.7 Commandes diagnostics minimales

Commandes utiles:
- `vitte build <fichier.vitte>`: pipeline complet.
- `vitte check <fichier.vit>`: parse + resolve + IR sans backend.
- `vitte build --stage parse|resolve|ir|backend <fichier.vit>`: isolation de couche.
- `vitte check --diag-json <fichier.vit>`: diagnostics structurés CI/IDE.
- `vitte check --dump-ast <fichier.vit>` et `vitte check --dump-ir <fichier.vit>`: debug stable.
- `vitte reduce <fichier.vit>`: reproducer minimal automatique.
- lire le log de stage (`parse`, `resolve`, `ir`, `backend`).
- `python3 docs/book/scripts/qa_book.py`: QA doc standard.
- `python3 docs/book/scripts/qa_book.py --strict`: QA CI stricte.

## 30.8 Cinq scénarios de debug pas à pas

1. Input: statement top-level (`emit ..`).
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
- `let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `when .. is ..`, `return`.

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

- `docs/book/chapters/keywords/emit.md`.
- `docs/book/chapters/keywords/return.md`.
- `docs/book/chapters/keywords/space.md`.
- `docs/book/chapters/keywords/use.md`.
- `docs/book/chapters/keywords/make.md`.

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

### Exemple B: variante cas d'erreur (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant un test explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter un test avant cette ligne.
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


## Exemple Étendu


```vit
// Scenario faq: execution complete et verifiable
space demo/faq

form Input { id: int value: int quota: int }
pick Eval { case Accepted(score: int) case Rejected(code: int) }

proc validate(x: Input) -> Eval {

  if x.id <= 0 { give Eval.Rejected(21) }

  if x.quota < 0 { give Eval.Rejected(22) }

  if x.value < 0 { give Eval.Rejected(23) }

  give Eval.Accepted(x.value)
}

proc transform(score: int, quota: int) -> int {
  let capped: int = score
  if capped > quota { set capped = quota }

  if capped < 0 { give 0 }

  give capped * 2
}

proc decide(r: Eval, quota: int) -> Eval {

  match r {
    case Accepted(s) {
      let out: int = transform(s, quota)

      if out >= 10 { give Eval.Accepted(out) }

      give Eval.Rejected(31)
    }
    case Rejected(c) { give Eval.Rejected(c) }
    otherwise { give Eval.Rejected(70) }
  }
}

// Conversion finale vers un code de sortie
proc to_exit(r: Eval) -> int {

  match r {
    case Accepted(_) { give 0 }
    case Rejected(code) { give code }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let x: Input = Input(1, 8, 9)
  let v: Eval = validate(x)
  let d: Eval = decide(v, x.quota)

  return to_exit(d)
}
```

## Explication détaillée du gros bloc

Ici, l'objectif est de comprendre le chemin réel du programme, ligne par ligne, jusqu'au code de sortie.

### 1. Rôle de chaque partie
- Point de départ: `entry main at core/app`.
- `validate`: lit `x: Input` et renvoie `Eval`.
- `transform`: lit `score: int, quota: int` et renvoie `int`.
- `decide`: lit `r: Eval, quota: int` et renvoie `Eval`.
- `to_exit`: lit `r: Eval` et renvoie `int`.

### 2. Ordre réel d'exécution
1. Le programme entre dans `main`.
2. `validate` est appelé pour traiter l'étape suivante.
3. `decide` est appelé pour traiter l'étape suivante.
4. `to_exit` est appelé pour traiter l'étape suivante.
5. La valeur finale est convertie en sortie process (`return ...`).

### 3. Tests qui changent le chemin
- Test évalué: `x.id <= 0`.
- Test évalué: `x.quota < 0`.
- Test évalué: `x.value < 0`.
- Test évalué: `capped > quota`.
- Test évalué: `capped < 0`.
- Test évalué: `out >= 10`.
- Sélection par `match r`: le chemin dépend de l'état reçu.
- Sélection par `match r`: le chemin dépend de l'état reçu.

### 4. Trace rapide avec valeurs
- Exemple nominal: `entrée valide -> validate -> decide -> to_exit -> sortie 0`.
- Exemple erreur: `entrée invalide -> validate renvoie un code d'erreur -> sortie non nulle`.

### 5. Pourquoi ce découpage est utile
- Vous testez chaque fonction seule, puis le flux complet.
- Vous savez où modifier une règle sans casser tout le programme.
- Vous pouvez expliquer la sortie en suivant simplement les appels.

### 6. Vérification rapide
1. Relancer avec une entrée normale et noter la sortie.
2. Relancer avec une entrée invalide et vérifier le code d'erreur.
3. Confirmer que la même entrée donne toujours la même sortie.


## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les tests sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas d'erreur réel:
- Entree degradee ou incomplete: le test doit couper le flux tot avec une sortie explicite.

A tester:
- Cas nominal -> sortie 0.
- Cas quota strict -> comportement stable.
- Cas invalide id<=0 -> sortie 21.


### 7. Ligne par ligne (variables + valeurs)

Lecture pratique: suivez les variables dans l'ordre réel d'exécution, puis vérifiez la sortie observée.

- Point d'entrée:
- `entry main at core/app` lance le scénario complet.

- Fonctions du bloc:
- `validate` lit `x: Input` puis renvoie `Eval`.
- `transform` lit `score: int, quota: int` puis renvoie `int`.
- `decide` lit `r: Eval, quota: int` puis renvoie `Eval`.
- `to_exit` lit `r: Eval` puis renvoie `int`.

- Variables créées (valeur initiale):
- `capped: int` démarre avec `score`.
- `out: int` démarre avec `transform(s, quota)`.
- `x: Input` démarre avec `Input(1, 8, 9)`.
- `v: Eval` démarre avec `validate(x)`.
- `d: Eval` démarre avec `decide(v, x.quota)`.

- Variables modifiées pendant le traitement:
- `capped` est mis à jour avec `quota`.

- Conditions qui changent le chemin:
- si `x.id <= 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `x.quota < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `x.value < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `capped > quota` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `capped < 0` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.
- si `out >= 10` est vrai: sortie anticipée ou branche dédiée; sinon: le flux continue.

- Trace nominale (valeurs exemple):
- initialisation: capped=score -> out=transform(s, quota) -> x=Input(1, 8, 9) -> v=validate(x)
- enchaînement: validate -> decide -> to_exit
- sortie finale sur ce chemin: `to_exit(d)`.

- Trace d'erreur (valeurs exemple):
- si `x.id <= 0` devient vrai, la fonction renvoie immédiatement `Eval.Rejected(21)`.

- Vérification rapide:
- relancer avec une entrée normale et noter la sortie,
- relancer avec une entrée invalide et noter le code d'erreur,
- confirmer qu'une même entrée produit toujours la même sortie.

## Trade-offs

| Contrainte | Option A | Option B | Décision recommandée |
| --- | --- | --- | --- |
| Lisibilité prioritaire | Branches explicites | Code compact | A si l'équipe maintient le code longtemps |
| Perf critique | Spécialisation ciblée | Généralisation | A si profiling confirme le gain |
| Évolution rapide | Contrats stricts | Conventions implicites | A pour réduire les régressions |


## Décision Selon Contrainte

- Si la contrainte dominante est la sûreté: valider tôt, échouer explicitement.
- Si la contrainte dominante est la latence: mesurer d'abord, optimiser ensuite.
- Si la contrainte dominante est l'évolutivité: isoler orchestration, décisions et conversion de sortie.


## Diagnostic Rapide

| Symptôme | Cause probable | Vérification | Correction |
| --- | --- | --- | --- |
| Sortie inattendue | Test absente ou mal ordonnée | Rejouer avec cas d'erreur | Remonter le test avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Règle implicite | Comparer nominal vs limite | Formaliser la règle dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas d'erreur réel et prévoir sa sortie,
- identifier où ajouter un test sans casser le nominal.


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: tests d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les tests d'entrée sont placés avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
