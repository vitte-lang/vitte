# 9. Modules et organisation

Niveau: Intermédiaire

Prérequis: chapitre précédent `docs/book/chapters/08-structures.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/08-structures.md`, `docs/book/chapters/10-diagnostics.md`, `book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre poursuit un objectif clair: construire une frontière de module qui limite le couplage. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 9.1 Module source

```vit
space app/core
proc add(a: int, b: int) -> int {
  give a + b
}
```

Lecture ligne par ligne (débutant):
1. `space app/core` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `proc add(a: int, b: int) -> int {` ici, le contrat complet est défini pour `add`: entrées `a: int, b: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `add` retourne toujours une valeur compatible avec `int`.
3. `give a + b` ici, la branche renvoie immédiatement `a + b` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `a + b`.
4. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `a + b`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: poser un module source minimal, centré sur une responsabilité unique.

Ce noyau est simple à testér et à réutiliser, car il ne dépend pas d'une couche externe.

À l'exécution:
- `add(10,32)` retourne `42`.
- la fonction reste locale au module tant qu'elle n'est pas exportée.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 9.2 Module consommateur avec alias

```vit
space app/math
pull app/core as core
share add_pair
proc add_pair(x: int, y: int) -> int {
  give core.add(x, y)
}
```

Lecture ligne par ligne (débutant):
1. `space app/math` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `pull app/core as core` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
3. `share add_pair` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
4. `proc add_pair(x: int, y: int) -> int {` sur cette ligne, le contrat complet est posé pour `add_pair`: entrées `x: int, y: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `add_pair` retourne toujours une valeur compatible avec `int`.
5. `give core.add(x, y)` sur cette ligne, la sortie est renvoyée immédiatement `core.add(x, y)` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `core.add(x, y)`.
6. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `core.add(x, y)`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: consommer un module externe de manière explicite et exposer une surface publique contrôlée.

Le `pull ... as core` rend la dépendance visible, et `share add_pair` limite précisément ce que le module expose.

À l'exécution, `add_pair(1,2)` délègue à `core.add(1,2)` puis retourne `3`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 9.3 Module domaine pur

```vit
space app/domain
form Ticket { id: int, priority: int }
proc is_critical(t: Ticket) -> bool { give t.priority >= 9 }
```

Lecture ligne par ligne (débutant):
1. `space app/domain` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `form Ticket { id: int, priority: int }` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.
3. `proc is_critical(t: Ticket) -> bool { give t.priority >= 9 }` cette instruction participe directement au pipeline du chapitre et doit être lue comme une étape explicite du résultat final. Exemple concret: sa présence influence l'état ou la valeur observée à la fin du scénario.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: garder le domaine indépendant des couches d'orchestration et d'infrastructure.

Cette séparation rend les règles métier stables: elles ne changent pas quand l'entrypoint, l'IO ou le transport évoluent.

À l'exécution:
- `Ticket(priority=9)` retourne `true`.
- `Ticket(priority=8)` retourne `false`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## À retenir

Dépendances orientées, exports limités, domaine pur. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez dire qui dépend de qui en lisant les `pull`.
- vous savez limiter ce qui est public avec `share`.
- vous pouvez faire évoluer un module sans propager des changements partout.


## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## Mini défi transverse

Défi: combinez au moins deux notions des trois derniers chapitres dans une fonction courte (garde + transformation + sortie).
Vérification minimale: montrez un cas nominal et un cas invalide, puis expliquez quelle branche est prise.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/keywords/as.md`.
- `docs/book/keywords/bool.md`.
- `docs/book/keywords/false.md`.
- `docs/book/keywords/field.md`.
- `docs/book/keywords/form.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

## Checkpoint synthèse

Mini quiz:
1. Quelle est l'invariant central de ce chapitre ?
2. Quelle garde évite l'état invalide le plus fréquent ?
3. Quel test simple prouve le comportement nominal ?

