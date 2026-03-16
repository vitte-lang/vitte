# 13. Génériques

Niveau: Intermédiaire

Prérequis: chapitre précédent `docs/book/chapters/12-pointeurs.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/12-pointeurs.md`, `docs/book/chapters/14-macros.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Génériques**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Génériques**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Génériques**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Génériques** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Génériques**.

## Exercice court

Prenez un exemple du chapitre sur **Génériques**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Génériques**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 13.1 Fonction identité paramétrée par type

```vit
proc id[T](x: T) -> T {
  give x
}
```

Lecture ligne par ligne (débutant):
1. `proc id[T](x: T) -> T {` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `give x` -> Comportement: la branche renvoie immédiatement `x` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `x`.
3. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: montrer qu'un même corps de fonction peut être spécialisé par type, sans changer le contrat entrée/sortie.

Le comportement est uniforme: la fonction renvoie exactement ce qu'elle reçoit, quel que soit `T`.

À l'exécution:
- `id[int](42)` retourne `42`.
- `id[string]("ok")` retourne `"ok"`.

Note de syntaxe actuelle:
- les appels génériques explicites comme `id[int](42)` sont implémentés
- cette surface reste `experimental`
- une forme comme `id[i](42)` ne devient pas un appel générique: elle reste une indexation suivie d'appel

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 13.2 Homogénéité imposée sur plusieurs arguments

```vit
proc first[T](a: T, b: T) -> T {
  give a
}
```

Lecture ligne par ligne (débutant):
1. `proc first[T](a: T, b: T) -> T {` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `give a` -> Comportement: la sortie est renvoyée immédiatement `a` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `a`.
3. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `a`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: imposer le même type sur plusieurs arguments pour éviter les mélanges incohérents.

Ici, la sécurité est précoce: l'erreur est signalée à la compilation, avant toute exécution.

À l'exécution:
- `first[int](7,9)` retourne `7`.
- `first(7,"x")` est rejeté au type-check.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 13.3 Structure générique et transformation

```vit
form Pair[T] {
  left: T
  right: T
}
proc swap_pair[T](p: Pair[T]) -> Pair[T] {
  give Pair[T](p.right, p.left)
}
```

Lecture ligne par ligne (débutant):
1. `form Pair[T] {` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `left: T` -> Comportement: cette ligne déclare le champ `left` avec le type `T`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `T`.
3. `right: T` -> Comportement: cette ligne déclare le champ `right` avec le type `T`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `T`.
4. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
5. `proc swap_pair[T](p: Pair[T]) -> Pair[T] {` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
6. `give Pair[T](p.right, p.left)` -> Comportement: retourne immédiatement `Pair[T](p.right, p.left)` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `Pair[T](p.right, p.left)`.
7. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Pair[T](p.right, p.left)`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: montrer que la généricité s'applique autant aux structures qu'aux procédures.

Le point clé est la conservation du type: un `Pair[int]` transformé reste un `Pair[int]`.

À l'exécution, `swap_pair(Pair[int](1,2))` retourne `Pair[int](2,1)`.

Lecture de surface utile:
- `Pair[int](1,2)` utilise aujourd'hui la même famille de syntaxe explicite que `id[int](42)`
- la forme qualifiée `mod.Pair[int](...)` est aussi supportée
- la promotion de cette syntaxe au noyau stable reste volontairement différée

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## À retenir

Le code est mutualisé, les invariants de type restent garantis et les erreurs sont détectées en compilation. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez identifier ce qui dépend de `T` et ce qui n'en dépend pas.
- vous savez détecter un mélange de types avant l'exécution.
- vous savez vérifier qu'une transformation conserve le type attendu.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
## À faire

1. Reprenez un exemple du chapitre et modifiez une condition de garde pour observer un comportement différent.
2. Écrivez un mini test mental sur une entrée invalide du chapitre, puis prédisez la branche exécutée.

## Corrigé minimal

- identifiez la ligne modifiée et expliquez en une phrase la nouvelle sortie attendue.
- nommez la garde ou la branche de secours réellement utilisée.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/keywords/as.md`.
- `docs/book/keywords/asm.md`.
- `docs/book/keywords/form.md`.
- `docs/book/keywords/give.md`.
- `docs/book/keywords/int.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.
