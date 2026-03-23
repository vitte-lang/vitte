# 16a. Liaison native avec `std/bridge`

Niveau: Intermédiaire.

Prérequis: chapitre précédent `book/chapters/16-interop.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/17-stdlib.md`, `book/chapters/23-projet-sys.md`, `book/chapters/30-faq.md`.

## Objectif

Comprendre le coeur du chapitre avec des exemples concrets et savoir reproduire le résultat sur votre propre code.

## Pourquoi

Ce chapitre vous donne une compréhension claire de **Liaison native avec `std/bridge`**.
Vous y trouvez le cadre, les invariants et les décisions de lecture utiles en pratique.

## Ce que vous allez réellement faire

Vous allez identifier les points clés de **Liaison native avec `std/bridge`**, exécuter les exemples, puis valider le comportement attendu avec un test simple par section.

## Exemple minimal

Commencez par le premier extrait de code de ce chapitre.
Lisez d'abord l'entrée, puis la sortie, avant d'examiner les détails d'implémentation liés à **Liaison native avec `std/bridge`**.

## Méthode de lecture

1. Repérez l'intention du bloc.
2. Vérifiez la condition ou la garde principale.
3. Confirmez la sortie observable.
4. Notez comment ce bloc sert **Liaison native avec `std/bridge`** dans l'ensemble du chapitre.

## Pièges fréquents

- Lire la syntaxe sans vérifier le comportement.
- Mélanger règle générale et cas limite dans la même explication.
- Introduire une optimisation avant d'avoir stabilisé le flux de **Liaison native avec `std/bridge`**.

## Exercice court

Prenez un exemple du chapitre sur **Liaison native avec `std/bridge`**.
Modifiez une condition ou une valeur d'entrée, puis vérifiez si le résultat reste conforme au contrat attendu.

## Résumé en 5 points

1. Vous connaissez l'objectif du chapitre sur **Liaison native avec `std/bridge`**.
2. Vous savez lire un exemple du chapitre de façon structurée.
3. Vous distinguez cas nominal et cas limite.
4. Vous évitez les pièges les plus fréquents.
5. Vous pouvez réutiliser ces règles dans le chapitre suivant.

## Conforme EBNF

<<< vérification rapide >>>
- Top-level: `use` reste au niveau module, jamais dans un bloc.
- Statements: `let`, `give`, `return` restent dans les blocs de `proc`/`entry`.
- Types primaires: `int`, `i32`, `u32`, `string`, `bool` suivent `type_primary`.

## Keywords à revoir

- `book/keywords/use.md`.
- `book/keywords/proc.md`.
- `book/keywords/entry.md`.
- `book/keywords/give.md`.
- `book/keywords/return.md`.

## Test mental

Question: comment vérifier qu'une fonction native est bien chargée et appelée avec la bonne signature ?
Repère: valider la signature, l'import, puis exécuter un cas nominal et un cas erreur.

## À faire

1. Ajouter un appel natif minimal.
2. Vérifier le cas nominal.
3. Provoquer une erreur de signature pour lire le diagnostic.

## Corrigé minimal

- Signature Vitte et signature native alignées.
- Chemin de liaison explicite.
- Test nominal + test erreur documentés.
