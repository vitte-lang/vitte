# 17. Packages

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/16-interop.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/16-interop.md`, `book/chapters/18-tests.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Packages**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Packages**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Packages**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Packages** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Packages**.

## Exercice court

Prenez un exemple du chapitre sur **Packages**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Packages**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 17.1 Import minimal et procédure pure

```vit
use std/core/types.int
proc inc(x: int) -> int {
  give x + 1
}
```

Lecture ligne par ligne (débutant):
1. `use std/core/types.int` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `proc inc(x: int) -> int {` -> Comportement: le contrat est défini pour `inc`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `inc` retourne toujours une valeur compatible avec `int`.
3. `give x + 1` -> Comportement: la branche renvoie immédiatement `x + 1` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `x + 1`.
4. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x + 1`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: montrer qu'un import minimal suffit quand la logique reste simple et pure.

Le message est important: les packages sont un support technique, pas un prétexte pour diluer la logique métier.

À l'exécution, `inc(41)=42`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 17.2 Récurrence déterministe sur bornes connues

```vit
use std/core/types.int
proc checksum(seed: int, n: int) -> int {
  let i: int = 0
  let acc: int = seed
  loop {
    if i >= n { break }
    set acc = ((acc * 33) + i + 17) % 1000003
    set i = i + 1
  }
give acc
}
```

Lecture ligne par ligne (débutant):
1. `use std/core/types.int` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
2. `proc checksum(seed: int, n: int) -> int {` -> Comportement: le contrat est posé pour `checksum`: entrées `seed: int, n: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `checksum` retourne toujours une valeur compatible avec `int`.
3. `let i: int = 0` -> Comportement: cette ligne crée la variable `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `i` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
4. `let acc: int = seed` -> Comportement: cette ligne crée la variable `acc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `acc` reçoit ici le résultat de `seed` et peut être réutilisé ensuite sans recalcul.
5. `loop {` -> Comportement: cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`). -> Preuve: à chaque tour, les gardes internes décident de continuer ou de sortir proprement.
6. `if i >= n { break }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `i >= n` est vrai, `break` est exécuté immédiatement; sinon on continue sur la ligne suivante.
7. `set acc = ((acc * 33) + i + 17) % 1000003` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `acc` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `acc` prend la nouvelle valeur `((acc * 33) + i + 17) % 1000003` pour les étapes suivantes.
8. `set i = i + 1` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `i` prend la nouvelle valeur `i + 1` pour les étapes suivantes.
9. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
10. `give acc` -> Comportement: la sortie est renvoyée immédiatement `acc` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `acc`.
11. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: écrire une récurrence déterministe dont la terminaison est bornée.

Lecture recommandée:
- identifier l'état initial (`acc = seed`).
- vérifier la borne (`i >= n`).
- suivre la formule de transition.

À l'exécution, `(seed=7,n=3)` produit `248`, puis `8202`, puis `270685`, et retourne `270685`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- laisser une boucle sans borne claire ou sans condition d'arrêt vérifiable.
- faire évoluer plusieurs variables d'état sans documenter leur rôle.
- optimiser trop tôt sans verrouiller d'abord le comportement attendu.

## 17.3 Normalisation métier indépendante du socle

```vit
form Reading {
  value: int
}
proc normalize_reading(r: Reading) -> int {
  if r.value < 0 { give 0 }
  if r.value > 100 { give 100 }
  give r.value
}
```

Lecture ligne par ligne (débutant):
1. `form Reading {` -> Comportement: cette ligne ouvre la structure `Reading` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable. -> Preuve: plusieurs fonctions peuvent manipuler `Reading` sans redéfinir ses champs.
2. `value: int` -> Comportement: cette ligne déclare le champ `value` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation. -> Preuve: le compilateur refusera une affectation incompatible avec `int`.
3. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
4. `proc normalize_reading(r: Reading) -> int {` -> Comportement: le contrat est fixé pour `normalize_reading`: entrées `r: Reading` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `normalize_reading` retourne toujours une valeur compatible avec `int`.
5. `if r.value < 0 { give 0 }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `r.value < 0` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `if r.value > 100 { give 100 }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `r.value > 100` est vrai, `give 100` est exécuté immédiatement; sinon on continue sur la ligne suivante.
7. `give r.value` -> Comportement: retourne immédiatement `r.value` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `r.value`.
8. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `r.value < 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `r.value`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: garder la logique métier indépendante des détails du socle technique.

Ici, la règle métier est lisible en trois gardes, sans dépendre d'une API externe.

À l'exécution:
- `Reading(-4)` donne `0`.
- `Reading(50)` donne `50`.
- `Reading(140)` donne `100`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Les imports sont précis, les algorithmes sont bornés et la logique de domaine reste isolée. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez distinguer ce qui relève du socle et ce qui relève du métier.
- vous savez prouver la terminaison d'une boucle avec sa borne.
- vous pouvez modifier une règle métier sans toucher aux imports techniques.

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

- `docs/book/keywords/break.md`.
- `docs/book/keywords/const.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/form.md`.
- `docs/book/keywords/give.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.
