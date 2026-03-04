# 19. Performance

Niveau: Avancé

Prérequis: chapitre précédent `docs/book/chapters/18-tests.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/18-tests.md`, `docs/book/chapters/20-repro.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Performance**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Performance**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Performance**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Performance** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Performance**.

## Exercice court

Prenez un exemple du chapitre sur **Performance**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Performance**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 19.1 Établir une version de référence

```vit
proc sum_loop(n: int) -> int {
  let i: int = 0
  let acc: int = 0
  loop {
    if i >= n { break }
    set acc = acc + i
    set i = i + 1
  }
give acc
}
```

Lecture ligne par ligne (débutant):
1. `proc sum_loop(n: int) -> int {` -> Comportement: le contrat est défini pour `sum_loop`: entrées `n: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `sum_loop` retourne toujours une valeur compatible avec `int`.
2. `let i: int = 0` -> Comportement: cette ligne crée la variable `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `i` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
3. `let acc: int = 0` -> Comportement: cette ligne crée la variable `acc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `acc` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
4. `loop {` -> Comportement: cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`). -> Preuve: à chaque tour, les gardes internes décident de continuer ou de sortir proprement.
5. `if i >= n { break }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `i >= n` est vrai, `break` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `set acc = acc + i` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `acc` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `acc` prend la nouvelle valeur `acc + i` pour les étapes suivantes.
7. `set i = i + 1` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `i` prend la nouvelle valeur `i + 1` pour les étapes suivantes.
8. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
9. `give acc` -> Comportement: la branche renvoie immédiatement `acc` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `acc`.
10. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: poser une baseline lisible qui servira de point de comparaison.

Cette version définit à la fois le résultat attendu et la structure de coût initiale (une boucle, une addition, un incrément à chaque tour).

À l'exécution, `sum_loop(4)` accumule `0+1+2+3` et retourne `6`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 19.2 Variante avec branche de filtrage

```vit
proc sum_even(n: int) -> int {
  let i: int = 0
  let acc: int = 0
  loop {
    if i >= n { break }
    if (i % 2) != 0 {
      set i = i + 1
      continue
    }
  set acc = acc + i
  set i = i + 1
}
give acc
}
```

Lecture ligne par ligne (débutant):
1. `proc sum_even(n: int) -> int {` -> Comportement: le contrat est posé pour `sum_even`: entrées `n: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `sum_even` retourne toujours une valeur compatible avec `int`.
2. `let i: int = 0` -> Comportement: cette ligne crée la variable `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `i` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
3. `let acc: int = 0` -> Comportement: cette ligne crée la variable `acc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `acc` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
4. `loop {` -> Comportement: cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`). -> Preuve: à chaque tour, les gardes internes décident de continuer ou de sortir proprement.
5. `if i >= n { break }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `i >= n` est vrai, `break` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `if (i % 2) != 0 {` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
7. `set i = i + 1` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `i` prend la nouvelle valeur `i + 1` pour les étapes suivantes.
8. `continue` -> Comportement: cette ligne définit une étape explicite du flux. -> Preuve: sa présence influence l'état ou la valeur observée à la fin du scénario.
9. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
10. `set acc = acc + i` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `acc` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `acc` prend la nouvelle valeur `acc + i` pour les étapes suivantes.
11. `set i = i + 1` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `i` prend la nouvelle valeur `i + 1` pour les étapes suivantes.
12. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
13. `give acc` -> Comportement: la sortie est renvoyée immédiatement `acc` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `acc`.
14. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: introduire un filtrage conditionnel pour ne sommer que les valeurs paires.

La correction reste intacte, mais le profil de coût change: une branche supplémentaire est évaluée à chaque itération.

Lecture pas à pas pour `sum_even(6)`:
- `i=0` est pair, `acc=0`.
- `i=1` est impair, `continue`.
- `i=2` est pair, `acc=2`.
- `i=3` est impair, `continue`.
- `i=4` est pair, `acc=6`.
- `i=5` est impair, `continue`.
- arrêt sur `i=6`, retour `6`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 19.3 Variante sans branche de parité

```vit
proc sum_even_step(n: int) -> int {
  let i: int = 0
  let acc: int = 0
  loop {
    if i >= n { break }
    set acc = acc + i
    set i = i + 2
  }
give acc
}
```

Lecture ligne par ligne (débutant):
1. `proc sum_even_step(n: int) -> int {` -> Comportement: le contrat est fixé pour `sum_even_step`: entrées `n: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `sum_even_step` retourne toujours une valeur compatible avec `int`.
2. `let i: int = 0` -> Comportement: cette ligne crée la variable `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `i` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
3. `let acc: int = 0` -> Comportement: cette ligne crée la variable `acc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `acc` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
4. `loop {` -> Comportement: cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`). -> Preuve: à chaque tour, les gardes internes décident de continuer ou de sortir proprement.
5. `if i >= n { break }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `i >= n` est vrai, `break` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `set acc = acc + i` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `acc` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `acc` prend la nouvelle valeur `acc + i` pour les étapes suivantes.
7. `set i = i + 2` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `i` prend la nouvelle valeur `i + 2` pour les étapes suivantes.
8. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
9. `give acc` -> Comportement: retourne immédiatement `acc` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `acc`.
10. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: supprimer la branche de filtrage en avançant directement de deux en deux.

Ici, le flux est plus régulier: chaque tour exécute les mêmes opérations, sans `if` de parité.

À l'exécution, `sum_even_step(6)` traite aussi `0,2,4` et retourne `6`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## À retenir

Chaque optimisation préserve le résultat, change une variable de coût à la fois et reste mesurable. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez prouver que deux variantes calculent le même résultat.
- vous savez identifier ce qui change dans le coût d'exécution.
- vous savez comparer des versions sans mélanger plusieurs optimisations en même temps.

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
- `docs/book/keywords/give.md`.
- `docs/book/keywords/if.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.
