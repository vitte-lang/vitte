# 20a. Architecture globale du langage

Niveau: Intermédiaire.

Prérequis: chapitre précédent `book/chapters/20-repro.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/21-projet-cli.md`, `book/chapters/15-pipeline.md`, `book/INDEX-technique.md`.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Architecture globale du langage**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez faire

Vous allez identifier les points clés de **Architecture globale du langage**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Architecture globale du langage**.

## Explication pas à pas

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Architecture globale du langage** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Architecture globale du langage**.

## Exercice court

Prenez un exemple du chapitre sur **Architecture globale du langage**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Architecture globale du langage**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `book/keywords/proc.md`.
- `book/keywords/type.md`.
- `book/keywords/form.md`.
- `book/keywords/pick.md`.
- `book/keywords/match.md`.

## Objectif

Comprendre la frontière de chaque couche pour localiser rapidement un bug, une régression ou une décision d'architecture.

## Exemple

Entrée: une procédure avec une condition et un retour.
Traitement: le parseur forme l'AST, la vérification confirme les types, puis le backend produit le code exécutable.
Sortie: même comportement observable sur toutes les plateformes compatibles.

## Pourquoi

La plupart des erreurs viennent d'une confusion de couche. Une architecture explicite permet de corriger au bon endroit. Elle évite aussi les contournements qui masquent le vrai problème.

## Test mental

Question: que se passe-t-il si l'entrée est invalide ?
Réponse attendue: le rejet doit se faire dans la première couche capable de prouver l'invalidité (lexing, parsing, typing).

## À faire

1. Classer trois erreurs fictives dans la bonne couche du pipeline.
2. Décrire un invariant par couche (lexer, parser, type checker, backend).

## Corrigé minimal

Une erreur de token relève du lexer; une erreur de forme relève du parser; une erreur de type relève du checker. Le backend ne corrige pas les incohérences sémantiques.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **architecture globale du langage**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/demo {
  return 0
}
```

Lecture ligne par ligne:
1. `entry main at app/demo {` -> définit le point d'entrée du scénario.
2. `return 0` -> renvoie la sortie vérifiable.
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
