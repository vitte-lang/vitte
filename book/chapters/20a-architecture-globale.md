# 20a. Architecture globale du langage

Niveau: Intermédiaire.

Prérequis: chapitre précédent `book/chapters/20-repro.md` et `book/glossaire.md`.
Voir aussi: `book/chapters/21-projet-cli.md`, `book/chapters/15-pipeline.md`, `book/INDEX-technique.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

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



## Exemple Étendu


```vit
// Scenario architecture globale: execution complete et verifiable
space demo/architecture-globale

form Request { id: int amount: int quota: int }
pick Result { case Accepted(total: int) case Rejected(code: int) }

// Entrée applicative: validation des invariants de requête
proc parse_request(r: Request) -> Result {

  if r.id <= 0 { give Result.Rejected(91) }

  if r.quota < 0 { give Result.Rejected(92) }

  if r.amount < 0 { give Result.Rejected(93) }

  give Result.Accepted(r.amount)
}

// Politique métier: applique les règles de décision
proc apply_policy(total: int, quota: int) -> Result {
  let capped: int = total
  if capped > quota { set capped = quota }

  if capped < 5 { give Result.Rejected(94) }

  give Result.Accepted(capped)
}

// Persistance simulée: matérialise un résultat sans I/O réel
proc persist_sim(x: Result) -> Result {

  match x {
    case Accepted(v) {
      if v % 13 == 0 { give Result.Rejected(95) }

      give Result.Accepted(v)
    }
    case Rejected(c) { give Result.Rejected(c) }
    otherwise { give Result.Rejected(70) }
  }
}

// Conversion finale vers un code de sortie
proc to_exit(x: Result) -> int {

  match x {
    case Accepted(_) { give 0 }
    case Rejected(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let req: Request = Request(7, 12, 15)
  let p: Result = parse_request(req)
  let d: Result = apply_policy(12, req.quota)
  let s: Result = persist_sim(d)
  let _probe: int = to_exit(p)

  return to_exit(s)
}
```

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Requête nominale -> sortie 0.
- Entrée invalide id<=0 -> sortie 91.
- Refus métier valeur<5 -> sortie 94.


## Trade-offs

| Contrainte | Option A | Option B | Décision recommandée |
| --- | --- | --- | --- |
| Lisibilité prioritaire | Branches explicites | Code compact | A si l'équipe maintient le code longtemps |
| Perf critique | Spécialisation ciblée | Généralisation | A si profiling confirme le gain |
| Évolution rapide | Contrats stricts | Conventions implicites | A pour réduire les régressions |


## Décision Selon Contrainte

- Si la contrainte dominante est la sûreté: valider tôt, échouer explicitement.
- Si la contrainte dominante est la latence: mesurer d'abord, optimiser ensuite.
- Si la contrainte dominante est l'évolutivité: isoler orchestration, décisions et conversion de sortie.


## Diagnostic Rapide

| Symptôme | Cause probable | Vérification | Correction |
| --- | --- | --- | --- |
| Sortie inattendue | Garde absente ou mal ordonnée | Rejouer avec cas limite | Remonter la garde avant la zone sensible |
| Branche non prise | Condition trop large/trop stricte | Tracer l'entrée effective | Rendre la condition explicite et testée |
| Régression silencieuse | Contrat implicite | Comparer nominal vs limite | Formaliser le contrat dans le code |


## Checkpoint

À ce stade, vous devez savoir:
- expliquer le flux entrée -> décision -> sortie sans ambiguïté,
- isoler un cas limite réel et prévoir sa sortie,
- identifier où ajouter une garde sans casser le nominal.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
