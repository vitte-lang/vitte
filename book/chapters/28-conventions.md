# 28. Conventions de code

Niveau: Avancé

Prérequis: chapitre précédent `book/chapters/27-grammaire.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/27-grammaire.md`, `book/chapters/29-style.md`, `book/glossaire.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Conventions de code**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Conventions de code**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Conventions de code**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Conventions de code** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Conventions de code**.

## Exercice court

Prenez un exemple du chapitre sur **Conventions de code**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Conventions de code**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## 28.1 Nommer les procédures selon leur intention

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  if x > 65535 { give -1 }
  give x
}
```

Lecture ligne par ligne (débutant):
1. `proc parse_port(x: int) -> int {` : le contrat est défini pour `parse_port`: entrées `x: int` et sortie `int`, elle clarifie l'intention avant lecture détaillée du corps.
2. `if x < 0 { give -1 }` : cette garde traite le cas limite avant le calcul.
3. `if x > 65535 { give -1 }` : cette garde traite le cas limite avant le calcul.
4. `give x` : la branche renvoie immédiatement `x` pour la branche courante, la sortie de branche est explicite et vérifiable.
5. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: si `x < 0` est vrai, la sortie devient `-1`.
- Cas nominal: sans garde bloquante, la branche principale renvoie `x`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: faire porter l'intention métier par le nom de la procédure.

Le verbe `parse` signale un contrat de validation, et les gardes en tête rendent le flux immédiat à relire.

À l'exécution, `parse_port(8080)=8080`, `parse_port(70000)=-1`.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## 28.2 Nommer les structures selon le domaine

```vit
form User {
  id: int
  name: string
}
```

Lecture ligne par ligne (débutant):
1. `form User {` : cette ligne ouvre la structure `User` qui regroupe des données cohérentes sous un même nom métier, utile pour garder un vocabulaire stable.
2. `id: int` : cette ligne déclare le champ `id` avec le type `int`, ce qui documente son rôle et limite les erreurs de manipulation.
3. `name: string` : cette ligne déclare le champ `name` avec le type `string`, ce qui documente son rôle et limite les erreurs de manipulation.
4. `}` : cette accolade ferme le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: le flux suit la branche principale et produit une sortie déterministe.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: utiliser des noms de structure et de champs qui décrivent le domaine, pas l'implémentation.

Le schéma devient auto-documenté: la structure se lit comme une mini spécification métier.

À l'exécution, cette déclaration n'exécute rien, mais elle fixe un contrat de données clair au compile-time.

Erreurs fréquentes à éviter:
- coder des conventions implicites au lieu de les porter par le type.
- mélanger des cas métier différents dans une même représentation.
- ajouter des variantes sans mettre à jour les points de traitement.

## 28.3 Nommer les prédicats en booléen explicite

```vit
proc is_admin(role: int) -> bool {
  give role == 9
}
```

Lecture ligne par ligne (débutant):
1. `proc is_admin(role: int) -> bool {` : le contrat est posé pour `is_admin`: entrées `role: int` et sortie `bool`, elle clarifie l'intention avant lecture détaillée du corps.
2. `give role == 9` : la sortie est renvoyée immédiatement `role == 9` pour la branche courante, la sortie de branche est explicite et vérifiable.
3. `}` : cette accolade clôt le bloc logique.
Entrée -> sortie (à vérifier):
- Cas limite: une garde explicite du bloc gère les entrées hors contrat avant le chemin nominal.
- Cas nominal: sans garde bloquante, la branche principale renvoie `role == 9`.
- Observation testable: répéter la même entrée doit reproduire exactement la même sortie.

Test mental: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le bloc doit activer une garde explicite ou un chemin de secours déterministe.

L'intention de cette étape est directe: rendre les prédicats immédiatement reconnaissables dans les conditions.

Le préfixe `is_` réduit la charge cognitive: dans un `if`, on lit directement une question booléenne.

À l'exécution, `is_admin(9)=true` et `is_admin(1)=false`.

Erreurs fréquentes à éviter:
- accumuler des cas spéciaux sans clarifier l'intention.
- introduire de la complexité avant de stabiliser le comportement.
- laisser des décisions implicites qui freinent la relecture.

## À retenir

Les noms expriment l'intention, les gardes sont en tête et les prédicats booléens sont reconnaissables instantanément. Ce chapitre doit vous laisser une grille de lecture stable: intention visible, contrat explicite, et comportement observable du début à la fin.

Critère pratique de qualité pour ce chapitre:
- vous pouvez deviner le rôle d'une fonction en lisant uniquement son nom.
- vous savez distinguer un prédicat booléen d'une transformation.
- vous pouvez relire un fichier et repérer les conventions violées en quelques secondes.

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

- `book/keywords/bool.md`.
- `book/keywords/continue.md`.
- `book/keywords/false.md`.
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

Thème: **conventions de code**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  if x > 65535 { give -1 }
  give x
}
```

Lecture ligne par ligne:
1. `proc parse_port(x: int) -> int {` -> pose un contrat clair de fonction.
2. `if x < 0 { give -1 }` -> sépare nominal et cas limite.
3. `if x > 65535 { give -1 }` -> sépare nominal et cas limite.
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
