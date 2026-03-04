# 11. Collections et itération

Niveau: Intermédiaire

Prérequis: chapitre précédent `docs/book/chapters/10-diagnostics.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/10-diagnostics.md`, `docs/book/chapters/12-pointeurs.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Collections et itération**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Collections et itération**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Collections et itération**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Collections et itération** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Collections et itération**.

## Exercice court

Prenez un exemple du chapitre sur **Collections et itération**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Collections et itération**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 11.1 Reduction simple sur un tableau d'entiers

```vit
proc sum(values: int[]) -> int {
  let acc: int = 0
  for x in values {
    set acc = acc + x
  }
give acc
}
```

Lecture ligne par ligne (débutant):
1. `proc sum(values: int[]) -> int {` -> Comportement: le contrat est défini pour `sum`: entrées `values: int[]` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `sum` retourne toujours une valeur compatible avec `int`.
2. `let acc: int = 0` -> Comportement: cette ligne crée la variable `acc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `acc` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
3. `for x in values {` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
4. `set acc = acc + x` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `acc` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `acc` prend la nouvelle valeur `acc + x` pour les étapes suivantes.
5. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
6. `give acc` -> Comportement: la branche renvoie immédiatement `acc` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `acc`.
7. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: lire la collection en flux linéaire avec un seul état mutable, `acc`.

Ce modèle est simple à vérifier: chaque élément est lu une fois, puis ajouté à l'accumulateur.

Lecture pas à pas pour `[2,3,4]`:
- départ `acc=0`.
- après `2`: `acc=2`.
- après `3`: `acc=5`.
- après `4`: `acc=9`.
- retour final `9`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 11.2 Moyenne entiere avec garde sur collection vide

```vit
proc mean_floor(values: int[]) -> int {
  let total: int = 0
  let count: int = 0
  for x in values {
    set total = total + x
    set count = count + 1
  }
if count == 0 { give 0 }
give total / count
}
```

Lecture ligne par ligne (débutant):
1. `proc mean_floor(values: int[]) -> int {` -> Comportement: le contrat est posé pour `mean_floor`: entrées `values: int[]` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `mean_floor` retourne toujours une valeur compatible avec `int`.
2. `let total: int = 0` -> Comportement: cette ligne crée la variable `total` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `total` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
3. `let count: int = 0` -> Comportement: cette ligne crée la variable `count` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `count` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
4. `for x in values {` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
5. `set total = total + x` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `total` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `total` prend la nouvelle valeur `total + x` pour les étapes suivantes.
6. `set count = count + 1` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `count` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `count` prend la nouvelle valeur `count + 1` pour les étapes suivantes.
7. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
8. `if count == 0 { give 0 }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `count == 0` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
9. `give total / count` -> Comportement: la sortie est renvoyée immédiatement `total / count` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `total / count`.
10. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `count == 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `total / count`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: calculer une moyenne entière sans jamais exécuter une division invalide.

Le point clé est la garde `if count == 0`: elle traite explicitement le cas vide avant la division.

À l'exécution:
- `mean_floor([])` retourne `0`.
- `mean_floor([4,5,6])` calcule `total=15`, `count=3`, puis retourne `5`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 11.3 Filtrage positif et projection finale

```vit
proc positive_only(values: int[]) -> int[] {
  let out: int[] = []
  for x in values {
    if x <= 0 { continue }
    out.push(x)
  }
give out
}
```

Lecture ligne par ligne (débutant):
1. `proc positive_only(values: int[]) -> int[] {` -> Comportement: le contrat est fixé pour `positive_only`: entrées `values: int[]` et sortie `int[]`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `positive_only` retourne toujours une valeur compatible avec `int[]`.
2. `let out: int[] = []` -> Comportement: cette ligne crée la variable `out` de type `int[]` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `out` reçoit ici le résultat de `[]` et peut être réutilisé ensuite sans recalcul.
3. `for x in values {` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
4. `if x <= 0 { continue }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `x <= 0` est vrai, `continue` est exécuté immédiatement; sinon on continue sur la ligne suivante.
5. `out.push(x)` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
6. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
7. `give out` -> Comportement: retourne immédiatement `out` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `out`.
8. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `out`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: filtrer tôt pour conserver une boucle lisible.

La règle est locale et explicite: toute valeur `<= 0` est ignorée immédiatement.

À l'exécution, sur `[-1,2,0,7]`:
- `-1` est ignoré.
- `2` est conservé -> `[2]`.
- `0` est ignoré.
- `7` est conservé -> `[2,7]`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

L'itération est linéaire, les gardes de sécurité sont explicites et la mutation est strictement locale. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez suivre l'état de l'accumulateur étape par étape.
- vous savez justifier la présence d'une garde avant une opération risquée.
- vous savez isoler clairement filtrage et transformation finale.

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

- `docs/book/keywords/continue.md`.
- `docs/book/keywords/field.md`.
- `docs/book/keywords/for.md`.
- `docs/book/keywords/form.md`.
- `docs/book/keywords/give.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.
