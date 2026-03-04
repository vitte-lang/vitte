# 0. Avant-propos

Niveau: Débutant

Prérequis: aucun prérequis technique; lire `book/glossaire.md` pour le vocabulaire de base.
Voir aussi: `docs/book/chapters/01-demarrer.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Avant-propos**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Avant-propos**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Avant-propos**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Avant-propos** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Avant-propos**.

## Exercice court

Prenez un exemple du chapitre sur **Avant-propos**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Avant-propos**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## Niveau local

- Niveau local section coeur: Avancé.
- Niveau local exemples guidés: Intermédiaire.
- Niveau local exercices de diagnostic: Avancé.

Rôle d'ouverture: installer la méthode de lecture, la discipline d'explication et les repères de base.

Ce chapitre poursuit un objectif clair: poser la méthode de travail de ce livre Vitte pour que chaque chapitre soit exécutable, vérifiable et réutilisable en projet réel. Au lieu d'empiler des recettes, nous allons construire une lecture fiable du code, avec des choix explicites et des effets vérifiables.

L'approche adoptée est volontairement littérale: chaque exemple doit être lisible comme une démonstration courte, avec une intention claire, un chemin d'exécution explicite et une conclusion vérifiable. Ce rythme est celui d'un manuel: comprendre, exécuter, puis retenir l'invariant utile.

La méthode reste constante: poser une intention, l'implémenter dans une forme compacte, puis observer précisément ce que le programme garantit à l'exécution.

Repère: voir le `Glossaire Vitte` dans `book/glossaire.md` et la `Checklist de relecture` dans `docs/book/checklist-editoriale.md`. Complément: `docs/book/erreurs-classiques.md`.
## 0.0 Repères de base

Avant d'entrer dans les exemples, nous fixons ici les bases de vocabulaire. Elles ne seront plus redéfinies en détail dans chaque chapitre.

- `int`: type entier canonique utilisé dans les exemples de base. Règle: dès qu'une largeur binaire contractuelle est requise, utiliser `i32/i64/i128/u32/u64/u128`. Exemple simple: `let age: int = 42`.
- `i32`: entier signé 32 bits (`-2^31` à `2^31-1`), utile quand la taille doit être explicite.
- `i64`: entier signé 64 bits, utile pour des compteurs/horodatages plus larges.
- `i128`: entier signé 128 bits, utile pour des bornes numériques très larges.
- `u32`: entier non signé 32 bits (`0` à `2^32-1`), utile pour tailles/indices non négatifs.
- `u64`: entier non signé 64 bits, utile pour tailles volumineuses et compteurs monotones.
- `u128`: entier non signé 128 bits, utile pour identifiants/compteurs très larges non négatifs.
- `string`: type du texte, utilisé pour stocker des mots, phrases, identifiants ou messages. Exemple simple: `let name: string = "alice"`.
- `bool`: type logique avec seulement deux valeurs possibles, `true` ou `false`, utilisé pour décider si une condition est vérifiée. Exemple simple: `let ok: bool = true`.
- `proc`: mot-clé qui déclare une procédure (fonction) avec des paramètres d'entrée et un type de sortie annoncé. Exemple simple: `proc add(a: int, b: int) -> int { ... }`.
- `entry`: mot-clé qui déclare le point d'entrée du programme, c'est-à-dire l'endroit où l'exécution commence réellement. Exemple simple: `entry main at core/app { ... }`.
- `let`: mot-clé qui crée une variable locale nommée, avec un type explicite et une valeur initiale. Exemple simple: `let total: int = 0`.
- `set`: mot-clé qui modifie une variable déjà créée avec `let`; on l'utilise pour faire évoluer un état dans une boucle ou un algorithme. Exemple simple: `set total = total + 1`.
- `if`: mot-clé de condition; il exécute une branche uniquement si l'expression logique est vraie. Exemple simple: `if total > 10 { give 10 }`.
- `match`: mot-clé de sélection de branche selon la forme d'une valeur (souvent un type déclaré avec `pick`); il rend les décisions explicites. Exemple simple: `match result { ... }`.
- `case`: branche précise dans un `match`; chaque `case` traite une forme de valeur donnée. Exemple simple: `case Ok(v) { give v }`.
- `otherwise`: branche de secours dans un `match`, utilisée si aucune branche `case` ne correspond. Exemple simple: `otherwise { give 70 }`.
- `give`: mot-clé de retour immédiat depuis une branche locale d'une procédure, souvent utilisé pour rendre les sorties explicites dans des gardes. Exemple simple: `give total`.
- `return`: mot-clé de fin d'exécution du bloc d'entrée (ou d'un bloc exécutable), généralement utilisé pour renvoyer un code de sortie observable. Exemple simple: `return 0`.
- `form`: mot-clé qui déclare une structure de données nommée avec des champs typés; il sert à organisér des données métier. Exemple simple: `form User { id: int name: string }`.
- `pick`: mot-clé qui déclare un type à variantes fermées (somme de cas), pratique pour modéliser des états (`Ok`, `Err`, etc.) sans ambiguïté. Exemple simple: `pick Result { case Ok case Err(code: int) }`.

Convention du livre: à partir d'ici, les explications ligne par ligne se concentrent sur la logique du code, les invariants et l'effet d'exécution, pas sur la redéfinition répétée des mots-clés de base.

## 0.0.1 Comment lire une ligne de code

Pour lire une ligne de code sans vous perdre, appliquez toujours les mêmes 4 questions, dans cet ordre:

- Entrée: quelles données sont utilisées ici (variables, paramètres, champs) ?
- Condition: y a-t-il une garde ou une branche qui peut bloquer/changer le flux ?
- Sortie: quelle valeur est produite, renvoyée ou propagée ?
- Effet d'état: qu'est-ce qui est modifié après cette ligne (variable, structure, collection) ?

Méthode pratique: si vous savez répondre à ces 4 questions pour chaque ligne, vous comprenez déjà la logique globale du bloc.

## 0.1 Lire Vitte comme un langage de contrats explicites

```vit
proc contract_demo(x: int) -> int {
  if x < 0 { give 0 }
  give x
}
```

Lecture ligne par ligne (débutant):
1. `proc contract_demo(x: int) -> int {` -> Comportement: le contrat annonce une entrée `x` et une sortie `int`. -> Preuve: tout appel doit produire un entier.
2. `if x < 0 { give 0 }` -> Comportement: garde immédiate sur le cas invalide. -> Preuve: `x = -3` déclenche la branche et renvoie `0`.
3. `give x` -> Comportement: chemin nominal sans transformation cachée. -> Preuve: `x = 8` renvoie `8`.
4. `}` -> Comportement: fin du bloc, aucune instruction implicite. -> Preuve: la sortie est déjà déterminée par les `give` précédents.

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: si `x < 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Une signature claire et des gardes visibles réduisent les interprétations implicites. Le style du livre suit cette discipline partout.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `contract_demo(-3)=0` et `contract_demo(8)=8`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 0.2 Travailler en boucle courte écriture, vérification, correction

```vit
proc loop_demo(n: int) -> int {
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
1. `let i: int = 0` -> Comportement: initialise l’index de boucle. -> Preuve: la première itération part toujours de `i = 0`.
2. `let acc: int = 0` -> Comportement: initialise l’accumulateur. -> Preuve: la somme démarre à zéro.
3. `if i >= n { break }` -> Comportement: borne de sortie explicite. -> Preuve: la boucle s’arrête dès que la condition devient vraie.
4. `set acc = acc + i` -> Comportement: ajoute la valeur courante à la somme. -> Preuve: `acc` augmente de `i` à chaque tour valide.
5. `set i = i + 1` -> Comportement: progression monotone vers la borne. -> Preuve: `i` finit par atteindre `n`.
6. `give acc` -> Comportement: renvoie la somme calculée. -> Preuve: pour `n = 4`, la sortie vaut `6` (0+1+2+3).

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `acc`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Le code est écrit pour être testé rapidement. La borne de boucle explicite facilite audit et debug.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `loop_demo(4)=6`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- laisser une boucle sans borne claire ou sans condition d'arrêt vérifiable.
- faire évoluer plusieurs variables d'état sans documenter leur rôle.
- optimiser trop tôt sans verrouiller d'abord le comportement attendu.

## 0.3 Garder une séparation stricte entre métier et projection système

```vit
pick OpResult {
  case Ok(value: int)
  case Err(code: int)
}
proc to_exit(r: OpResult) -> int {
  match r {
    case Ok(_) { give 0 }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}
```

Lecture ligne par ligne (débutant):
1. `pick OpResult { ... }` -> Comportement: encode explicitement les états `Ok` et `Err`. -> Preuve: aucun autre état n’est accepté sans modifier le type.
2. `match r {` -> Comportement: force un choix de branche lisible. -> Preuve: un même `r` prend toujours la même branche.
3. `case Ok(_) { give 0 }` -> Comportement: projection technique du succès vers le code `0`. -> Preuve: `Ok(42)` renvoie `0`.
4. `case Err(c) { give c }` -> Comportement: propage le code d’erreur métier. -> Preuve: `Err(64)` renvoie `64`.
5. `otherwise { give 70 }` -> Comportement: garde de secours explicite. -> Preuve: une forme non prévue renvoie `70` au lieu de casser.

Mini tableau Entrée -> Sortie (exemples):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok(value: int)` permet de confirmer la branche attendue.

Test mental standard: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: Le métier produit des valeurs typées. La projection système est une étape finale et localisée.

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

À l'exécution, `to_exit(Ok(42))=0` et `to_exit(Err(64))=64`.

Ce déroulé concret sert de preuve locale: il confirme que la forme du code et le résultat attendu restent alignés.

Erreurs fréquentes à éviter:
- étendre la zone sensible au lieu de la garder courte et auditable.
- placer la validation après l'opération risquée.
- masquer la frontière technique, ce qui rend le diagnostic plus coûteux.

## À retenir

Vous lisez ce livre comme un atelier technique, chaque exemple est runnable et chaque chapitre prolonge les invariants du précédent. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin. L'objectif final est de rendre chaque décision de code explicable à la première lecture, comme dans un texte de référence.

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
- `docs/book/keywords/case.md`.
- `docs/book/keywords/continue.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.
