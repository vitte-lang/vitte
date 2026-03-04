# 1. Démarrer avec Vitte

Niveau: Débutant

Prérequis: chapitre précédent `docs/book/chapters/00-avant-propos.md` et `book/glossaire.md`.
Voir aussi: `docs/book/chapters/00-avant-propos.md`, `docs/book/chapters/02-philosophie.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Démarrer avec Vitte**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Démarrer avec Vitte**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Démarrer avec Vitte**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Démarrer avec Vitte** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Démarrer avec Vitte**.

## Exercice court

Prenez un exemple du chapitre sur **Démarrer avec Vitte**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Démarrer avec Vitte**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 1.1 Programme minimal exécutable

```vit
entry main at core/app {
  return 0
}
```

Lecture ligne par ligne (débutant):
1. `entry main at core/app {` -> Comportement: cette ligne fixe le point d'entrée `main` dans `core/app` et sert de scénario exécutable de bout en bout pour le chapitre. -> Preuve: lancer cette entrée permet de vérifier la chaîne complète des fonctions appelées.
2. `return 0` -> Comportement: cette ligne termine l'exécution du bloc courant avec le code `0`, utile pour observer le résultat global du scénario. -> Preuve: un test d'exécution peut vérifier directement que le programme retourne `0`.
3. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le scénario principal se termine avec `return 0`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `0`.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: `entry` fixe le point de depart. `return` fixe le code de sortie sans ambiguite.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, le programme entre dans `main` puis sort avec `0`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 1.2 Introduire une procédure

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
entry main at core/app {
  let r: int = add(20, 22)
  return r
}
```

Lecture ligne par ligne (débutant):
1. `proc add(a: int, b: int) -> int {` -> Comportement: le contrat est défini pour `add`: entrées `a: int, b: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `add` retourne toujours une valeur compatible avec `int`.
2. `give a + b` -> Comportement: la branche renvoie immédiatement `a + b` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `a + b`.
3. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
4. `entry main at core/app {` -> Comportement: cette ligne fixe le point d'entrée `main` dans `core/app` et sert de scénario exécutable de bout en bout pour le chapitre. -> Preuve: lancer cette entrée permet de vérifier la chaîne complète des fonctions appelées.
5. `let r: int = add(20, 22)` -> Comportement: cette ligne crée la variable `r` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `r` reçoit ici le résultat de `add(20, 22)` et peut être réutilisé ensuite sans recalcul.
6. `return r` -> Comportement: cette ligne termine l'exécution du bloc courant avec le code `r`, utile pour observer le résultat global du scénario. -> Preuve: un test d'exécution peut vérifier directement que le programme retourne `r`.
7. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `a + b`.
- Observation testable: exécuter le scénario permet de vérifier le code de sortie `r`.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Signature compile-time stable et appel vérifie. Pas d'effet secondaire cache.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `add(20,22)=42`, sortie finale `42`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 1.3 Ajouter une boucle bornée

```vit
proc sum_to(n: int) -> int {
  let i: int = 0
  let s: int = 0
  loop {
    if i > n { break }
    set s = s + i
    set i = i + 1
  }
give s
}
```

Lecture ligne par ligne (débutant):
1. `proc sum_to(n: int) -> int {` -> Comportement: le contrat est posé pour `sum_to`: entrées `n: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `sum_to` retourne toujours une valeur compatible avec `int`.
2. `let i: int = 0` -> Comportement: cette ligne crée la variable `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `i` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
3. `let s: int = 0` -> Comportement: cette ligne crée la variable `s` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `s` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
4. `loop {` -> Comportement: cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`). -> Preuve: à chaque tour, les gardes internes décident de continuer ou de sortir proprement.
5. `if i > n { break }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `i > n` est vrai, `break` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `set s = s + i` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `s` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `s` prend la nouvelle valeur `s + i` pour les étapes suivantes.
7. `set i = i + 1` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `i` prend la nouvelle valeur `i + 1` pour les étapes suivantes.
8. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
9. `give s` -> Comportement: la sortie est renvoyée immédiatement `s` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `s`.
10. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `s`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Borne de sortie explicite, mutation d'état localisée.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `sum_to(3)` retourne `6`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- laisser une boucle sans borne claire ou sans condition d'arrêt vérifiable.
- faire évoluer plusieurs variables d'état sans documenter leur rôle.
- optimiser trop tôt sans verrouiller d'abord le comportement attendu.

## À retenir

Entrée claire, signatures explicites, borne de boucle visible. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin. L'objectif final est de rendre chaque décision de code explicable à la première lecture, comme dans un texte de référence.

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

- `docs/book/keywords/at.md`.
- `docs/book/keywords/bool.md`.
- `docs/book/keywords/break.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/entry.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.
