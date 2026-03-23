# 10. Diagnostics et erreurs

Niveau: Intermédiaire

Prérequis: chapitre précédent `book/chapters/09-modules.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/09-modules.md`, `book/chapters/11-collections.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Diagnostics et erreurs**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Diagnostics et erreurs**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Diagnostics et erreurs**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Diagnostics et erreurs** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Diagnostics et erreurs**.

## Exercice court

Prenez un exemple du chapitre sur **Diagnostics et erreurs**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Diagnostics et erreurs**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 10.1 Garde de division

```vit
proc safe_div(num: int, den: int) -> int {
  if den == 0 { give 0 }
  give num / den
}
```

Lecture ligne par ligne (débutant):
1. `proc safe_div(num: int, den: int) -> int {` : le contrat est défini pour `safe_div`: entrées `num: int, den: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if den == 0 { give 0 }` : cette garde traite le cas limite avant le calcul.
3. `give num / den` : la branche renvoie immédiatement `num / den` pour la branche courante, la sortie de branche est explicite et vérifiable.
4. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: si `den == 0` est vrai, la sortie devient `0`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `num / den`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: placer la frontière d'erreur au plus près de l'opération risquée.

Ce principe évite les crashs évitables: la division est protégée avant d'être exécutée.

À l'exécution:
- `safe_div(12,3)` retourne `4`.
- `safe_div(12,0)` retourne `0` sans tenter la division.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 10.2 Erreur typée

```vit
pick ParsePort {
  case Ok(value: int)
  case Err(code: int)
}
proc parse_port(x: int) -> ParsePort {
  if x < 0 { give Err(400) }
  if x > 65535 { give Err(422) }
  give Ok(x)
}
```

Lecture ligne par ligne (débutant):
1. `pick ParsePort {` : cette ligne ouvre le type fermé `ParsePort` pour forcer un ensemble fini de cas possibles et supprimer les états implicites.
2. `case Ok(value: int)` : ce cas décrit `Ok(value: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
3. `case Err(code: int)` : ce cas décrit `Err(code: int)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
4. `}` : cette accolade ferme le bloc logique.
5. `proc parse_port(x: int) -> ParsePort {` : le contrat est posé pour `parse_port`: entrées `x: int` et sortie `ParsePort`, elle clarifie l'intention avant lecture détaillée du corps.
6. `if x < 0 { give Err(400) }` : cette garde traite le cas limite avant le calcul.
7. `if x > 65535 { give Err(422) }` : cette garde traite le cas limite avant le calcul.
8. `give Ok(x)` : la sortie est renvoyée immédiatement `Ok(x)` pour la branche courante, la sortie de branche est explicite et vérifiable.
9. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: si `x < 0` est vrai, la sortie devient `Err(400)`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `Ok(x)`.
- Observation testable: forcer le cas `Ok(value: int)` permet de confirmer la branche attendue.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: encoder le diagnostic dans le type de retour, pour rendre les échecs aussi explicites que les succès.

Avec ce modèle, on ne perd pas l'information d'erreur: chaque cas garde son code associé.

À l'exécution:
- `parse_port(-1)` retourne `Err(400)`.
- `parse_port(8080)` retourne `Ok(8080)`.
- `parse_port(70000)` retourne `Err(422)`.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 10.3 Projection technique

```vit
proc to_exit(p: ParsePort) -> int {
  match p {
    case Ok(_) { give 0 }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}
```

Lecture ligne par ligne (débutant):
1. `proc to_exit(p: ParsePort) -> int {` : le contrat est fixé pour `to_exit`: entrées `p: ParsePort` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `match p {` : cette ligne démarre un dispatch déterministe sur `p`: une seule branche sera choisie selon la forme de la valeur analysée.
3. `case Ok(_) { give 0 }` : ce cas décrit `Ok(_)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
4. `case Err(c) { give c }` : ce cas décrit `Err(c)` et explicite la décision métier associée, ce qui réduit les ambiguïtés de lecture.
5. `otherwise { give 70 }` : cette ligne définit un chemin de secours explicite.
6. `}` : cette accolade ferme le bloc logique.
7. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: forcer le cas `Ok(_)` permet de confirmer la branche attendue.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: séparer la logique métier de sa projection technique (ici, le code de sortie).

Ce découplage est important: la politique système peut évoluer sans réécrire la logique de parsing.

À l'exécution:
- `to_exit(Ok(_))` retourne `0`.
- `to_exit(Err(422))` retourne `422`.
- `otherwise` garde un code de secours (`70`).

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Cause localisée, typée, projetée proprement. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous savez localiser la cause d'une erreur sans parcourir tout le code.
- vous savez distinguer résultat métier et projection technique.
- vous pouvez testér séparément le parsing et la politique d'exit code.

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

- `book/keywords/case.md`.
- `book/keywords/continue.md`.
- `book/keywords/field.md`.
- `book/keywords/form.md`.
- `book/keywords/give.md`.

## Objectif
Ce chapitre fixe un objectif opérationnel clair et vérifiable pour le concept étudié.

## Exemple
Exemple concret: partir d'une entrée simple, appliquer une transformation, puis observer la sortie attendue.

## Pourquoi
Ce bloc existe pour relier la syntaxe à l'intention métier, réduire les ambiguïtés et préparer les tests.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **diagnostics et erreurs**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc safe_div(num: int, den: int) -> int {
  if den == 0 { give 0 }
  give num / den
}
```

Lecture ligne par ligne:
1. `proc safe_div(num: int, den: int) -> int {` -> pose un contrat clair de fonction.
2. `if den == 0 { give 0 }` -> sépare nominal et cas limite.
3. `give num / den` -> renvoie la sortie vérifiable.
4. `}` -> participe au déroulé du traitement.

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
