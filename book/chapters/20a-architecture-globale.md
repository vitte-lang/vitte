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
  // Sortie programme: code de retour observable
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



## Exemple Étendu

Exemple approfondi pour **architecture globale**: flux applicatif complet (entrée, politique métier, persistance simulée, code de sortie).

```vit
// Exemple long: flux complet et vérifiable
space demo/architecture-globale

form Request { id: int amount: int quota: int }
pick Result { case Accepted(total: int) case Rejected(code: int) }

// Entrée applicative: validation des invariants de requête
proc parse_request(r: Request) -> Result {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if r.id <= 0 { give Result.Rejected(91) }
  // Garde: bloque un cas invalide avant de continuer
  if r.quota < 0 { give Result.Rejected(92) }
  // Garde: bloque un cas invalide avant de continuer
  if r.amount < 0 { give Result.Rejected(93) }
  // Sortie locale: valeur retournee par la procedure
  give Result.Accepted(r.amount)
}

// Politique métier: applique les règles de décision
proc apply_policy(total: int, quota: int) -> Result {
  let capped: int = total
  if capped > quota { set capped = quota }
  // Garde: bloque un cas invalide avant de continuer
  if capped < 5 { give Result.Rejected(94) }
  // Sortie locale: valeur retournee par la procedure
  give Result.Accepted(capped)
}

// Persistance simulée: matérialise un résultat sans I/O réel
proc persist_sim(x: Result) -> Result {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match x {
    case Accepted(v) {
      // Garde: bloque un cas invalide avant de continuer
  if v % 13 == 0 { give Result.Rejected(95) }
      // Sortie locale: valeur retournee par la procedure
  give Result.Accepted(v)
    }
    case Rejected(c) { give Result.Rejected(c) }
    otherwise { give Result.Rejected(70) }
  }
}

// Projection finale: convertit l'état métier en code de sortie
proc to_exit(x: Result) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match x {
    case Accepted(_) { give 0 }
    case Rejected(c) { give c }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let req: Request = Request(7, 12, 15)
  let p: Result = parse_request(req)
  let d: Result = apply_policy(12, req.quota)
  let s: Result = persist_sim(d)
  let _probe: int = to_exit(p)
  // Sortie programme: code de retour observable
  return to_exit(s)
}
```

Scénarios recommandés (architecture globale):
- Requête nominale -> sortie 0.
- Entrée invalide id<=0 -> sortie 91.
- Refus métier valeur<5 -> sortie 94.
