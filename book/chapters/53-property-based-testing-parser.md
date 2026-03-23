# 53. Property-based testing pour le parser

Niveau: Avance

Prérequis: `book/chapters/27-grammaire.md`, `book/chapters/52-tests-grammaire-valid-invalid.md`.
Voir aussi: à définir.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Trouver automatiquement des cas limites parser via generation de donnees.

## Proprietes utiles

1. Parse deterministe sur meme entree.
2. Entrees invalides ne crashent jamais le parser.
3. Invariants AST respectes apres parse.

## Demarche

1. Definir generateurs d'entrees.
2. Encoder les proprietes.
3. Reducer auto pour minimiser les contre-exemples.

## Exemples progressifs (N1 -> N3)

### N1 (base): entree deterministe

```vit
entry main at app/demo { return 0 }
```

### N2 (intermediaire): invalides robustes

```vit
entry main at app/demo { return }
```

### N3 (avance): proprietes AST

```vit
entry main at app/demo {
  return 1 + 2 * 3
}
```

### Anti-exemple

```vit
# generation aleatoire sans verification de propriete
```

## Validation rapide

1. Determinisme parse valide.
2. Pas de crash sur invalides.
3. Proprietes AST verifiees.

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

Thème: **property-based testing pour le parser**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/demo { return 0 }
```

Lecture ligne par ligne:
1. `entry main at app/demo { return 0 }` -> définit le point d'entrée du scénario.

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
// Scenario property based testing parser: execution complete et verifiable
space demo/property-based-testing-parser

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
