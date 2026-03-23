# 9. Modules et organisation

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/08-structures.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/08-structures.md`, `book/chapters/10-diagnostics.md`, `book/glossaire.md`.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Modules et organisation**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Modules et organisation**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Modules et organisation**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Modules et organisation** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Modules et organisation**.

## Exercice court

Prenez un exemple du chapitre sur **Modules et organisation**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Modules et organisation**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 9.1 Module source

```vit
space app/core
proc add(a: int, b: int) -> int {
  give a + b
}
```

Lecture simple du code:
1. `space app/core` : cette ligne définit une étape explicite du flux.
2. `proc add(a: int, b: int) -> int {` : le contrat est défini pour `add`: entrées `a: int, b: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
3. `give a + b` : la branche renvoie immédiatement `a + b` pour la branche courante, la sortie de branche est explicite et vérifiable.
4. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `a + b`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: poser un module source minimal, centré sur une responsabilité unique.

Ce noyau est simple à testér et à réutiliser, car il ne dépend pas d'une couche externe.

À l'exécution:
- `add(10,32)` retourne `42`.
- la fonction reste locale au module tant qu'elle n'est pas exportée.

Erreurs classiques à éviter:
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

Lecture simple du code:
1. `space app/math` : cette ligne définit une étape explicite du flux.
2. `pull app/core as core` : cette ligne définit une étape explicite du flux.
3. `share add_pair` : cette ligne définit une étape explicite du flux.
4. `proc add_pair(x: int, y: int) -> int {` : le contrat est posé pour `add_pair`: entrées `x: int, y: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
5. `give core.add(x, y)` : la sortie est renvoyée immédiatement `core.add(x, y)` pour la branche courante, la sortie de branche est explicite et vérifiable.
6. `}` : cette accolade ferme le bloc logique.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `core.add(x, y)`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: consommer un module externe de manière explicite et exposer une surface publique contrôlée.

Le `pull.. as core` rend la dépendance visible, et `share add_pair` limite précisément ce que le module expose.

À l'exécution, `add_pair(1,2)` délègue à `core.add(1,2)` puis retourne `3`.

Erreurs classiques à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 9.3 Module domaine pur

```vit
space app/domain
form Ticket { id: int, priority: int }
proc is_critical(t: Ticket) -> bool { give t.priority >= 9 }
```

Lecture simple du code:
1. `space app/domain` : cette ligne définit une étape explicite du flux.
2. `form Ticket { id: int, priority: int }` : cette ligne définit une étape explicite du flux.
3. `proc is_critical(t: Ticket) -> bool { give t.priority >= 9 }` : cette ligne définit une étape explicite du flux.
Ce qu'on vérifie en pratique:
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Question utile: que se passe-t-il si l'entrée est invalide ?
Repère: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: garder le domaine indépendant des couches d'orchestration et d'infrastructure.

Cette séparation rend les règles métier stables: elles ne changent pas quand l'entrypoint, l'IO ou le transport évoluent.

À l'exécution:
- `Ticket(priority=9)` retourne `true`.
- `Ticket(priority=8)` retourne `false`.

Erreurs classiques à éviter:
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
Repère: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
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

- `book/keywords/as.md`.
- `book/keywords/bool.md`.
- `book/keywords/false.md`.
- `book/keywords/field.md`.
- `book/keywords/form.md`.

## Checkpoint synthèse

Mini quiz:
1. Quelle est l'invariant central de ce chapitre ?
2. Quelle garde évite l'état invalide le plus fréquent ?
3. Quel test simple prouve le comportement nominal ?
