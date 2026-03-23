# 4. Syntaxe essentielle

Niveau: Débutant

Prérequis: chapitre précédent `book/chapters/03-projet.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/03-projet.md`, `book/chapters/05-types.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Syntaxe essentielle**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Syntaxe essentielle**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Syntaxe essentielle**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Syntaxe essentielle** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Syntaxe essentielle**.

## Exercice court

Prenez un exemple du chapitre sur **Syntaxe essentielle**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Syntaxe essentielle**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 4.1 Définir une procédure et sa sortie

Une procédure commence toujours par une promesse explicite: "si vous me donnez ces valeurs, je vous rends ce type de résultat". En Vitte, cette promesse se lit dans la signature. Le lecteur n'a pas à deviner les types, ni le format du retour.

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Lecture ligne par ligne (débutant):
1. `proc add(a: int, b: int) -> int {` -> Comportement: le contrat est défini pour `add`: entrées `a: int, b: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `add` retourne toujours une valeur compatible avec `int`.
2. `give a + b` -> Comportement: la branche renvoie immédiatement `a + b` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `a + b`.
3. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
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
1. `proc clamp01(v: int) -> int {` -> Comportement: le contrat est posé pour `clamp01`: entrées `v: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `clamp01` retourne toujours une valeur compatible avec `int`.
2. `if v < 0 { give 0 }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `v < 0` est vrai, `give 0` est exécuté immédiatement; sinon on continue sur la ligne suivante.
3. `if v > 1 { give 1 }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `v > 1` est vrai, `give 1` est exécuté immédiatement; sinon on continue sur la ligne suivante.
4. `give v` -> Comportement: la sortie est renvoyée immédiatement `v` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `v`.
5. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
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
1. `proc count(n: int) -> int {` -> Comportement: le contrat est fixé pour `count`: entrées `n: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps. -> Preuve: un appel valide à `count` retourne toujours une valeur compatible avec `int`.
2. `let i: int = 0` -> Comportement: cette ligne crée la variable `i` de type `int` pour nommer explicitement une étape intermédiaire du raisonnement. -> Preuve: `i` reçoit ici le résultat de `0` et peut être réutilisé ensuite sans recalcul.
3. `loop {` -> Comportement: cette ligne ouvre une boucle contrôlée qui répète les mêmes étapes jusqu'à une condition d'arrêt claire (`break` ou `give`). -> Preuve: à chaque tour, les gardes internes décident de continuer ou de sortir proprement.
4. `if i >= n { break }` -> Comportement: cette garde traite le cas limite avant le calcul. -> Preuve: si `i >= n` est vrai, `break` est exécuté immédiatement; sinon on continue sur la ligne suivante.
5. `set i = i + 1` -> Comportement: cette ligne réalise une mutation volontaire et visible: l'état `i` change ici, à cet endroit précis du flux. -> Preuve: après exécution, `i` prend la nouvelle valeur `i + 1` pour les étapes suivantes.
6. `}` -> Comportement: cette accolade clôt le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
7. `give i` -> Comportement: retourne immédiatement `i` pour la branche courante, la sortie de branche est explicite et vérifiable. -> Preuve: dès cette instruction, la fonction quitte la branche avec la valeur `i`.
8. `}` -> Comportement: cette accolade ferme le bloc logique. -> Preuve: après cette fermeture, l'exécution revient au niveau supérieur de structure.
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

- `book/keywords/break.md`.
- `book/keywords/case.md`.
- `book/keywords/continue.md`.
- `book/keywords/give.md`.
- `book/keywords/if.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

<!-- AUTO_EXPANSION_V1 START -->

## Approfondissement concret (sans répétition)

### 1. Snippet de référence

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

### 2. Lecture du code ligne par ligne

1. `proc add(a: int, b: int) -> int {` -> déclare un contrat clair entre entrées et sortie.
2. `give a + b` -> rend la sortie observable sans ambiguïté.
3. `}` -> participe au flux principal du traitement.

### 3. Exécution réelle (entrée -> traitement -> sortie)

1. Entrée: préciser les valeurs acceptées et refusées.
2. Traitement: suivre le chemin nominal, puis la première garde.
3. Sortie: vérifier la valeur retournée ou l'erreur attendue.

### 4. Cas limite et erreur volontaire

- Cas limite: forcer la garde et confirmer la sortie de secours.
- Cas erreur: injecter un type inattendu et lire le diagnostic exact.
- Correction: modifier une seule ligne, recompiler, valider.

### 5. Refactor concret à faible risque

Méthode: garder la signature, simplifier une branche, et prouver que le comportement reste identique avec un test nominal + un test limite.

### 6. Série de scénarios représentatifs

Cas 1: pour **syntaxe essentielle**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 2: pour **syntaxe essentielle**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 3: pour **syntaxe essentielle**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 4: pour **syntaxe essentielle**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 5: pour **syntaxe essentielle**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 6: pour **syntaxe essentielle**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 7: pour **syntaxe essentielle**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 8: pour **syntaxe essentielle**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 9: pour **syntaxe essentielle**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 10: pour **syntaxe essentielle**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 11: pour **syntaxe essentielle**, inspecter l'axe 'stabilité des appels' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 12: pour **syntaxe essentielle**, inspecter l'axe 'lisibilité du module' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 13: pour **syntaxe essentielle**, inspecter l'axe 'robustesse en refactor' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 14: pour **syntaxe essentielle**, inspecter l'axe 'stabilité du comportement' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 15: pour **syntaxe essentielle**, inspecter l'axe 'qualité du diagnostic' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 16: pour **syntaxe essentielle**, inspecter l'axe 'contrat d'entrée' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 17: pour **syntaxe essentielle**, inspecter l'axe 'branche nominale' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 18: pour **syntaxe essentielle**, inspecter l'axe 'garde limite' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 19: pour **syntaxe essentielle**, inspecter l'axe 'sortie de secours' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 20: pour **syntaxe essentielle**, inspecter l'axe 'signature publique' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la cohérence avant/après. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 21: pour **syntaxe essentielle**, inspecter l'axe 'cohérence des types' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la trace de correction. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 22: pour **syntaxe essentielle**, inspecter l'axe 'ordre d'exécution' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider l'absence d'effet de bord. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 23: pour **syntaxe essentielle**, inspecter l'axe 'gestion d'erreur' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la sortie exacte. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 24: pour **syntaxe essentielle**, inspecter l'axe 'lisibilité du flux' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compréhension en relecture. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 25: pour **syntaxe essentielle**, inspecter l'axe 'coût de maintenance' en CI. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la compatibilité des appels. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 26: pour **syntaxe essentielle**, inspecter l'axe 'stabilité des appels' sur entrée invalide. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la lisibilité du message d'erreur. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 27: pour **syntaxe essentielle**, inspecter l'axe 'lisibilité du module' après extraction de procédure. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le scénario de non-régression. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 28: pour **syntaxe essentielle**, inspecter l'axe 'robustesse en refactor' après simplification d'une branche. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider le comportement du cas limite. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.
Cas 29: pour **syntaxe essentielle**, inspecter l'axe 'stabilité du comportement' avant merge. Objectif: isoler une seule hypothèse de code, comparer l'état avant/après, puis valider la stabilité du contrat. Si le résultat diverge, corriger une seule ligne, recompiler, et documenter la cause racine.

### 7. Checklist finale de compréhension

1. Le contrat d'entrée est explicite.
2. Le cas nominal est testable sans ambiguïté.
3. Le cas limite est traité explicitement.
4. Le diagnostic d'erreur est actionnable.
5. Le corrigé suit une modification locale et vérifiable.

<!-- AUTO_EXPANSION_V1 END -->

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **syntaxe essentielle**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Lecture ligne par ligne:
1. `proc add(a: int, b: int) -> int {` -> pose un contrat clair de fonction.
2. `give a + b` -> renvoie la sortie vérifiable.
3. `}` -> participe au déroulé du traitement.

### Exemple B: variante cas limite (même intention, comportement sécurisé)

Objectif: conserver la logique métier tout en ajoutant une garde explicite.

Étapes:
1. Identifier la ligne qui décide la sortie.
2. Ajouter une garde avant cette ligne.
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
