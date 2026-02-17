# 20a. Architecture globale du langage

Niveau: Intermédiaire.

Prérequis: chapitre précédent `docs/book/chapters/20-repro.md` et `docs/book/glossaire.md`.
Voir aussi: `docs/book/chapters/21-projet-cli.md`, `docs/book/chapters/15-pipeline.md`, `docs/book/INDEX-technique.md`.

## Trame du chapitre

- Objectif.
- Exemple.
- Pourquoi.
- Test mental.
- À faire.
- Corrigé minimal.


Ce chapitre présente la chaîne complète d'exécution de Vitte, depuis le source jusqu'au résultat observable. L'objectif est d'offrir une carte mentale stable avant les chapitres projet.

Pipeline global:
- Source -> Lexer -> Parser -> AST.
- AST -> Vérification de types -> IR.
- IR -> Optimisations -> Génération code.
- Code -> Runtime/ABI -> Exécution -> Sortie.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: seules les déclarations de module (`space`, `pull`, `use`, `share`, `const`, `type`, `form`, `pick`, `proc`, `entry`, `macro`) apparaissent hors bloc.
- Statements: les instructions (`let`, `make`, `set`, `give`, `emit`, `if`, `loop`, `for`, `match`, `select`, `return`) restent dans un `block`.
- Types primaires: `bool`, `string`, `int`, `i32`, `i64`, `i128`, `u32`, `u64`, `u128` sont acceptés dans `type_primary`.

## Keywords à revoir

- `docs/book/keywords/proc.md`.
- `docs/book/keywords/type.md`.
- `docs/book/keywords/form.md`.
- `docs/book/keywords/pick.md`.
- `docs/book/keywords/match.md`.


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
