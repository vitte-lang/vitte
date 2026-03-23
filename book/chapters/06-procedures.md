# 6. Procédures et contrats

Niveau: Débutant

Prérequis: chapitre précédent `book/chapters/05-types.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/05-types.md`, `book/chapters/07-controle.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Procédures et contrats**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Procédures et contrats**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Procédures et contrats**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Procédures et contrats** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Procédures et contrats**.

## Exercice court

Prenez un exemple du chapitre sur **Procédures et contrats**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Procédures et contrats**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 6.1 Contrat bornant

```vit
proc clamp(x: int, lo: int, hi: int) -> int {
  if x < lo { give lo }
  if x > hi { give hi }
  give x
}
```

Lecture ligne par ligne (débutant):
1. `proc clamp(x: int, lo: int, hi: int) -> int {` : le contrat est défini pour `clamp`: entrées `x: int, lo: int, hi: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if x < lo { give lo }` : cette garde traite le cas limite avant le calcul.
3. `if x > hi { give hi }` : cette garde traite le cas limite avant le calcul.
4. `give x` : la branche renvoie immédiatement `x` pour la branche courante, la sortie de branche est explicite et vérifiable.
5. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: si `x < lo` est vrai, la sortie devient `lo`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: définir une procédure dont le résultat reste toujours dans un intervalle autorisé, tant que les bornes sont cohérentes (`lo <= hi`).

Dans une lecture de production, ce choix réduit le coût mental: on voit immédiatement ce qui est garanti, ce qui est refusé, et où la décision est prise.

Lecture pas à pas:
- `clamp(-1, 0, 10)` retourne `0` (borne basse).
- `clamp(5, 0, 10)` retourne `5` (cas nominal).
- `clamp(99, 0, 10)` retourne `10` (borne haute).

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 6.2 Extraire la précondition

```vit
proc validate_bounds(lo: int, hi: int) -> bool {
  give lo <= hi
}
```

Lecture ligne par ligne (débutant):
1. `proc validate_bounds(lo: int, hi: int) -> bool {` : le contrat est posé pour `validate_bounds`: entrées `lo: int, hi: int` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps.
2. `give lo <= hi` : la sortie est renvoyée immédiatement `lo <= hi` pour la branche courante, la sortie de branche est explicite et vérifiable.
3. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `lo <= hi`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: extraire une précondition réutilisable pour éviter de répéter la même règle dans plusieurs fonctions.

Cette séparation est importante: `validate_bounds` ne fait qu'une chose, et la fait clairement. Elle sert ensuite de garde commune pour toutes les procédures qui manipulent des bornes.

À l'exécution:
- `validate_bounds(0,10)` retourne `true`.
- `validate_bounds(10,0)` retourne `false`.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 6.3 Composer la procédure finale

```vit
proc normalize(temp: int, lo: int, hi: int) -> int {
  if not validate_bounds(lo, hi) { give lo }
  if temp < lo { give lo }
  if temp > hi { give hi }
  give temp
}
```

Lecture ligne par ligne (débutant):
1. `proc normalize(temp: int, lo: int, hi: int) -> int {` : le contrat est fixé pour `normalize`: entrées `temp: int, lo: int, hi: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if not validate_bounds(lo, hi) { give lo }` : cette garde traite le cas limite avant le calcul.
3. `if temp < lo { give lo }` : cette garde traite le cas limite avant le calcul.
4. `if temp > hi { give hi }` : cette garde traite le cas limite avant le calcul.
5. `give temp` : retourne immédiatement `temp` pour la branche courante, la sortie de branche est explicite et vérifiable.
6. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: si `not validate_bounds(lo, hi)` est vrai, la sortie devient `lo`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `temp`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: composer les deux idées précédentes dans une procédure complète, en traitant d'abord les cas invalides puis le chemin nominal.

Ordre de lecture recommandé:
- valider d'abord les bornes (`validate_bounds`).
- puis appliquer la saturation basse et haute.
- enfin retourner la valeur nominale.

Lecture pas à pas:
- `normalize(50, 80, 20)` retourne `80` car les bornes sont invalides.
- `normalize(130, 0, 100)` retourne `100` car la valeur dépasse la borne haute.
- `normalize(60, 0, 100)` retourne `60` car la valeur est déjà valide.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Précondition explicite, branches testables, sortie stable. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez dire quelle hypothèse est exigée avant le calcul.
- vous pouvez testér chaque branche avec un exemple concret.
- vous pouvez justifier le résultat retourné sans ambiguïté.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: une garde explicite ou un chemin de secours déterministe doit s'appliquer.
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

- `book/keywords/bool.md`.
- `book/keywords/break.md`.
- `book/keywords/continue.md`.
- `book/keywords/false.md`.
- `book/keywords/for.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

## Checkpoint synthèse

Mini quiz:
1. Quelle est l'invariant central de ce chapitre ?
2. Quelle garde évite l'état invalide le plus fréquent ?
3. Quel test simple prouve le comportement nominal ?

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **procédures et contrats**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc clamp(x: int, lo: int, hi: int) -> int {
  if x < lo { give lo }
  if x > hi { give hi }
  give x
}
```

Lecture ligne par ligne:
1. `proc clamp(x: int, lo: int, hi: int) -> int {` -> pose un contrat clair de fonction.
2. `if x < lo { give lo }` -> sépare nominal et cas limite.
3. `if x > hi { give hi }` -> sépare nominal et cas limite.
4. `give x` -> renvoie la sortie vérifiable.
5. `}` -> participe au déroulé du traitement.

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
