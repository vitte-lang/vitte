# 4. Syntaxe essentielle

Niveau: Débutant

Prérequis: chapitre précédent `docs/book/chapters/03-projet.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/03-projet.md`, `docs/book/chapters/05-types.md`, `docs/book/glossaire.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Écrire du code, ce n'est pas seulement donner des ordres à une machine. C'est aussi laisser une trace lisible pour la personne qui reviendra demain, dans une semaine, ou dans six mois: parfois un collègue, souvent vous-même. La syntaxe essentielle de Vitte sert précisément à cela. Elle impose assez de structure pour rendre le comportement prévisible, sans alourdir l'écriture.

Dans ce chapitre, nous allons installer trois réflexes de base. D'abord, écrire une procédure comme un contrat clair entre des entrées et une sortie. Ensuite, traiter les cas limites avant le cas normal, pour éviter les surprises. Enfin, manipuler une boucle en gardant l'état visible à chaque étape. Ces trois réflexes suffisent déjà à produire un code propre, stable et facile à discuter.


Repère: voir le `Glossaire Vitte` dans `docs/book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 4.1 Définir une procédure et sa sortie

Une procédure commence toujours par une promesse explicite: "si vous me donnez ces valeurs, je vous rends ce type de résultat". En Vitte, cette promesse se lit dans la signature. Le lecteur n'a pas à deviner les types, ni le format du retour.

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Lecture ligne par ligne (débutant):
1. `proc add(a: int, b: int) -> int {` ici, le contrat complet est défini pour `add`: entrées `a: int, b: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `add` retourne toujours une valeur compatible avec `int`.
2. `give a + b` ici, la branche renvoie immédiatement `a + b` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `a + b`.
3. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `a + b`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Cette fonction se lit presque comme une phrase: prends `a` et `b`, additionne-les, rends le résultat. Pour `add(1, 2)`, le calcul donne `3`. Pour `add(10, -4)`, il donne `6`. Rien d'autre ne se produit, aucun effet de bord ne vient perturber la lecture.

Ce détail est plus important qu'il n'y paraît. Le mot-clé `give` rend la sortie volontaire et visible. Dans un projet réel, cette explicitation réduit énormément les malentendus: on sait où la valeur est produite, et on sait pourquoi.

Quand vous relisez une procédure, posez-vous trois questions simples:
- quelles sont les entrées autorisées.
- quel résultat est promis.
- à quel endroit précis la valeur est renvoyée.

Erreurs fréquentes à éviter:
- oublier d'annoncer le bon type de retour dans la signature.
- faire des calculs corrects mais renvoyer la mauvaise valeur.
- écrire une fonction simple avec une structure inutilement complexe.

## 4.2 Encadrer une valeur avec des gardes

Dans la plupart des programmes, les bugs viennent moins du cas normal que des bords: valeurs négatives, dépassements, entrées inattendues. Une bonne habitude consiste donc à protéger d'abord les limites, puis à laisser le cas nominal en dernier.

```vit
proc clamp01(v: int) -> int {
  if v < 0 { give 0 }
  if v > 1 { give 1 }
  give v
}
```

Lecture ligne par ligne (débutant):
1. `proc clamp01(v: int) -> int {` sur cette ligne, le contrat complet est posé pour `clamp01`: entrées `v: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `clamp01` retourne toujours une valeur compatible avec `int`.
2. `if v < 0 { give 0 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `v < 0` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if v > 1 { give 1 }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `v > 1` est vrai, `give 1` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give v` sur cette ligne, la sortie est renvoyée immédiatement `v` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `v`.
5. `}` sur cette ligne, le bloc logique est fermé et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `v < 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `v`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention est claire: forcer une valeur à rester dans l'intervalle `[0, 1]`. On lit le code dans l'ordre naturel. Si `v` est trop petite, on renvoie immédiatement `0`. Si elle est trop grande, on renvoie immédiatement `1`. Sinon, on la renvoie telle quelle.

Lecture pas à pas:
- `clamp01(-2)`: la première garde est vraie (`v < 0`), donc retour `0`.
- `clamp01(0)`: aucune garde n'est vraie, donc retour `0`.
- `clamp01(9)`: la seconde garde est vraie (`v > 1`), donc retour `1`.

La force de ce style, c'est sa franchise. Chaque ligne répond à une question précise: "que faire si c'est trop bas?", "que faire si c'est trop haut?", "que faire sinon?". Cette structure rend le code solide et agréable à relire.

Erreurs fréquentes à éviter:
- traiter le cas nominal avant les bornes, ce qui cache les exceptions.
- oublier un des deux bords de l'intervalle.
- multiplier les branches imbriquées alors que des gardes simples suffisent.

## 4.3 Itérer avec une mutation explicite

Une boucle devient dangereuse dès que l'on ne sait plus exactement comment l'état évolue. En Vitte, la mutation est explicite avec `set`, ce qui vous oblige à montrer noir sur blanc la transition d'un état au suivant.

```vit
proc count(n: int) -> int {
  let i: int = 0
  loop {
    if i >= n { break }
    set i = i + 1
  }
give i
}
```

Lecture ligne par ligne (débutant):
1. `proc count(n: int) -> int {` ce passage fixe le contrat complet de `count`: entrées `n: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. Exemple concret: un appel valide à `count` retourne toujours une valeur compatible avec `int`.
2. `let i: int = 0` cette ligne crée la variable locale `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. Exemple concret: `i` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
3. `loop {` cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`). Exemple concret: à chaque tour, les gardes internes décident de continuer ou de sortir proprement.
4. `if i >= n { break }` cette garde traite un cas précis le plus tôt possible pour protéger la suite du flux de calcul. Exemple concret: si `i >= n` est vrai, `break` est exécuté immédiatement; sinon on continue sur la ligne suivante.
5. `set i = i + 1` cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux. Exemple concret: après exécution, `i` prend la nouvelle valeur `i + 1` pour les étapes suivantes.
6. `}` ce passage clôt le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.
7. `give i` ce passage retourne immédiatement `i` pour la branche courante, la sortie de branche est explicite et vérifiable. Exemple concret: dès cette instruction, la fonction quitte la branche avec la valeur `i`.
8. `}` ici, l'accolade ferme le bloc logique en cours et délimite clairement la portée des instructions précédentes. Exemple concret: après cette fermeture, l'exécution revient au niveau supérieur de structure.


Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `i`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

Ici, la logique est volontairement simple. On initialise `i` à `0`. À chaque tour, on vérifie la condition d'arrêt: si `i >= n`, on quitte la boucle. Sinon, on avance d'un pas avec `set i = i + 1`.

Pour `n = 4`, la trajectoire est:
- départ `i = 0`.
- tour 1: `i = 1`.
- tour 2: `i = 2`.
- tour 3: `i = 3`.
- tour 4: `i = 4`.
- tour suivant: `i >= n`, arrêt, retour `4`.

Ce que cet exemple enseigne va au-delà du comptage. Il montre comment garder une boucle sous contrôle: une variable d'état clairement identifiée, une condition d'arrêt lisible, une progression unique et explicite.

Erreurs fréquentes à éviter:
- oublier d'incrémenter la variable, ce qui crée une boucle infinie.
- placer la condition d'arrêt au mauvais endroit.
- faire évoluer plusieurs états à la fois sans nécessité.

## À retenir

Si vous ne retenez qu'une idée, retenez celle-ci: un bon code n'est pas seulement correct, il est explicite. Dans Vitte, cette explicitation passe par des signatures nettes, des gardes honnêtes sur les limites, et des boucles où chaque mutation se voit. Quand ces trois éléments sont en place, vous obtenez un programme que l'on peut comprendre sans deviner, corriger sans trembler, et faire évoluer sans le casser.

Critère pratique de qualité pour ce chapitre:
- vous pouvez expliquer chaque exemple à voix haute en moins d'une minute.
- vous savez prédire le résultat d'un appel avant de l'exécuter.
- vous savez identifier immédiatement l'endroit où le flux s'arrête.


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
- `docs/book/keywords/case.md`.
- `docs/book/keywords/continue.md`.
- `docs/book/keywords/give.md`.
- `docs/book/keywords/if.md`.


## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

