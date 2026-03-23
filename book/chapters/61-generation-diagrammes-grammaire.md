# 61. Generation de diagrammes de grammaire

Niveau: Intermédiaire

Prérequis: `book/chapters/27-grammaire.md`.
Voir aussi: `book/grammar/railroad/README.md`.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Automatiser la generation des diagrammes railroad a partir de l'EBNF source.

## Workflow

1. Modifier la source EBNF unique.
2. Synchroniser artefacts de grammaire.
3. Regenerer diagrammes.
4. Verifier changements attendus.

## Commandes utiles

- `make grammar-sync`
- `make grammar-docs`
- `make grammar-gate`

## Exemples progressifs (N1 -> N3)

### N1 (base): règle unique

Snippet grammaire:

```vit
stmt ::= return_stmt | expr_stmt ;
```

Commande:

```bash
make grammar-docs
```

### N2 (intermédiaire): famille de règles

Snippet grammaire:

```vit
expr ::= assign_expr ;
assign_expr ::= or_expr [ "=" assign_expr ] ;
```

Commandes:

```bash
make grammar-sync
make grammar-docs
```

### N3 (avancé): régénération vérifiée

Snippet grammaire:

```vit
program ::= WS? { toplevel WS? } ;
```

Commandes:

```bash
make grammar-docs
make grammar-docs-check
```

### Anti-exemple

```text
Modifier manuellement les SVG/README générés.
```

## Validation rapide

1. Modifier la source unique.
2. Régénérer les artefacts.
3. Vérifier `--check` vert.

## Pourquoi

Cette section explicite la valeur pratique: réduire les erreurs, accélérer le diagnostic et stabiliser les évolutions.

## Test mental

Question de contrôle: si vous modifiez une hypothèse clé, quel résultat doit changer et pourquoi?

## À faire

1. Exécuter l’exemple nominal.
2. Introduire un cas limite.
3. Vérifier la sortie et documenter l’écart.

## Corrigé minimal

Corrigé: conserver la version la plus simple qui respecte le contrat, puis ajouter un test de non-régression.

<!-- AUTO_REPRESENTATIVE_EXAMPLES_V1 START -->

## Exemples représentatifs basés sur le code du chapitre

Thème: **generation de diagrammes de grammaire**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
stmt ::= return_stmt | expr_stmt ;
```

Lecture ligne par ligne:
1. `stmt ::= return_stmt | expr_stmt ;` -> participe au déroulé du traitement.

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
// Scenario generation diagrammes grammaire: execution complete et verifiable
space demo/generation-diagrammes-grammaire

form SourceUnit { bytes: int lines: int tokens_hint: int }
pick ParseState { case Parsed(nodes: int) case Failed(code: int) }

// Scan: transforme l'entrée brute en signal exploitable
proc scan(u: SourceUnit) -> int {

  if u.bytes <= 0 { give 0 }

  if u.lines <= 0 { give 0 }

  give (u.tokens_hint + u.lines)
}

// Parse: construit un état syntaxique déterministe
proc parse(token_count: int) -> ParseState {

  if token_count == 0 { give ParseState.Failed(101) }

  if token_count < 4 { give ParseState.Failed(102) }

  give ParseState.Parsed(token_count)
}

proc validate_structure(nodes: int) -> int {

  if nodes <= 0 { give 201 }

  if nodes > 200000 { give 202 }

  give 0
}

// Conversion finale vers un code de sortie
proc to_exit(p: ParseState) -> int {

  match p {
    case Parsed(n) {
      let v: int = validate_structure(n)

      if v != 0 { give v }

      give 0
    }
    case Failed(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let u: SourceUnit = SourceUnit(120, 12, 18)
  let t: int = scan(u)
  let p: ParseState = parse(t)

  return to_exit(p)
}
```

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Unité valide -> sortie 0.
- Entrée vide (bytes=0) -> sortie 101.
- Structure surdimensionnée -> sortie 202.


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


## Pourquoi Cette Erreur Arrive En Prod

Cause fréquente: entrée partiellement valide, hypothèse implicite dans une branche, puis projection de sortie trop tardive.
Symptôme: comportement correct en nominal mais instable sous charge ou données incomplètes.
Mesure utile: tracer l'entrée effective, rejouer le cas limite, verrouiller la garde au bon niveau.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
