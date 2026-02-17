# 20. Reproductibilité

Niveau: Avancé

Prérequis: chapitre précédent `docs/book/chapters/19-performance.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/19-performance.md`, `docs/book/chapters/21-projet-cli.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre poursuit un objectif clair: garantir qu'un programme Vitte produit le même résultat à entrée identique, sur chaque exécution. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.


Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 20.1 Fonction pure de base

```vit
proc stable(x: int) -> int {
  give x * 2 + 1
}
```

Lecture ligne par ligne (débutant):
1. `proc stable(x: int) -> int {` ici, le contrat complet est défini pour `stable`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `stable` retourne toujours une valeur compatible avec `int`.
2. `give x * 2 + 1` ici, la branche renvoie immédiatement `x * 2 + 1` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `x * 2 + 1`.
3. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x * 2 + 1`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: partir d'une fonction pure, c'est-à-dire une fonction dont la sortie dépend uniquement de ses paramètres. Aucun état externe, aucune source aléatoire, aucun accès implicite au temps.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution:
- `stable(10)` retourne `21`.
- `stable(10)` retourne encore `21` au second appel.

C'est la première pierre de la reproductibilité: à entrée identique, résultat identique.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 20.2 Variabilité injectée explicitement

```vit
proc with_seed(seed: int) -> int {
  give seed
}
```

Lecture ligne par ligne (débutant):
1. `proc with_seed(seed: int) -> int {` sur cette ligne, le contrat complet est posé pour `with_seed`: entrées `seed: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `with_seed` retourne toujours une valeur compatible avec `int`.
2. `give seed` sur cette ligne, la sortie est renvoyée immédiatement `seed` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `seed`.
3. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `seed`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: rendre explicite ce qui varie. Si un calcul a besoin d'une "graine" (`seed`), cette graine doit être un paramètre, pas une valeur cachée.

Ce principe facilite les tests et le débogage: on peut rejouer exactement le même scénario en réutilisant la même valeur d'entrée.

À l'exécution, `with_seed(12345)` retourne `12345` de manière stable.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 20.3 Séquence déterministe de checksum

```vit
proc repro_checksum(seed: int, n: int) -> int {
  let i: int = 0
  let acc: int = seed
  loop {
    if i >= n { break }
    set acc = (acc * 31 + i) % 104729
    set i = i + 1
  }
give acc
}
```

Lecture ligne par ligne (débutant):
1. `proc repro_checksum(seed: int, n: int) -> int {` ce passage fixe le contrat complet de `repro_checksum`: entrées `seed: int, n: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `repro_checksum` retourne toujours une valeur compatible avec `int`.
2. `let i: int = 0` cette ligne crée la variable locale `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `i` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
3. `let acc: int = seed` cette ligne crée la variable locale `acc` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `acc` reçoit ici le résultat de `seed` et peut être réutilisé ensuite sans recalcul.
4. `loop {` cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`). Exemple concret: à chaque tour, les gardes internes décident de continuer ou de sortir proprement.
5. `if i >= n { break }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `i >= n` est vrai, `break` est exécuté immédiatement; sinon on continue sur la ligne suivante.
6. `set acc = (acc * 31 + i) % 104729` cette ligne réalise une mutation volontaire et visible: l'état `acc` change ici, à cet endroit précis du flux. Exemple concret: après exécution, `acc` prend la nouvelle valeur `(acc * 31 + i) % 104729` pour les étapes suivantes.
7. `set i = i + 1` cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux. Exemple concret: après exécution, `i` prend la nouvelle valeur `i + 1` pour les étapes suivantes.
8. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
9. `give acc` ce passage retourne immédiatement `acc` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `acc`.
10. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: composer une séquence déterministe dans une boucle bornée. Toute l'évolution de `acc` dépend seulement de `seed`, de `n` et de la formule de mise à jour.

Lecture recommandée:
- identifier l'état initial (`acc = seed`).
- vérifier la borne de boucle (`i < n`).
- suivre la formule de transition (`acc = ...`).

À l'exécution, pour `(seed=5, n=4)`, `acc` évolue ainsi:
- départ `acc = 5`.
- après `i=0`: `155`.
- après `i=1`: `4806`.
- après `i=2`: `44114`.
- après `i=3`: `57592`.
- arrêt de boucle, retour `57592`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- laisser une boucle sans borne claire ou sans condition d'arrêt vérifiable.
- faire évoluer plusieurs variables d'état sans documenter leur rôle.
- optimiser trop tôt sans verrouiller d'abord le comportement attendu.

## À retenir

Les entrées sont explicites, la boucle est bornée et aucune source d'aléatoire cachée n'apparaît. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous pouvez rejouer un résultat en conservant les mêmes entrées.
- vous savez lister toutes les sources possibles de variation.
- vous savez isoler et supprimer une dépendance implicite.


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
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `u32`, `u64` sont acceptés dans `type_primary`.

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

