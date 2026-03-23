# 51. Tests de non-regression compilateur

Niveau: Avance

Prérequis: `book/chapters/15-pipeline.md`, `book/chapters/31-erreurs-build.md`.
Voir aussi: à définir.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Eviter la reintroduction de bugs deja corriges dans parse, typecheck, backend.

## Strategie

1. Capturer chaque bug en test minimal.
2. Classer par couche de pipeline.
3. Verifier diagnostics attendus et code genere.
4. Bloquer merge si regression.

## Checklist

1. Test lie a un incident reel.
2. Reproducer minimal et stable.
3. Assertion exacte sur symptome corrige.

## Exemples progressifs (N1 -> N3)

### N1 (base): bug parse capture

```vit
emit 1
```

### N2 (intermediaire): bug type capture

```vit
entry main at app/demo {
  let x: int = "oops"

  return x
}
```

### N3 (avance): regression verrouillee

```vit
entry main at app/demo {
  let x: int = 1

  return x
}
```

### Anti-exemple

```vit
# fix sans test associe
```

## Validation rapide

1. Reproduire bug initial.
2. Ajouter test de non-regression.
3. Verifier absence de reintroduction.

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

Thème: **tests de non-regression compilateur**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
emit 1
```

Lecture ligne par ligne:
1. `emit 1` -> participe au déroulé du traitement.

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
// Scenario tests non regression compilateur: execution complete et verifiable
space demo/tests-non-regression-compilateur

form TestCase { id: int input: int expected: int }
pick CaseResult { case Pass(id: int) case Fail(id: int, got: int, expected: int) }

proc subject(x: int) -> int {

  if x < 0 { give 0 }

  if x > 100 { give 100 }

  give x
}

// Exécute un cas de test et retourne un résultat typé
proc run(c: TestCase) -> CaseResult {
  let got: int = subject(c.input)

  if got == c.expected { give CaseResult.Pass(c.id) }

  give CaseResult.Fail(c.id, got, c.expected)
}

// Agrège les résultats et projette un code CI
proc ci_exit(a: CaseResult, b: CaseResult, c: CaseResult) -> int {
  let ok: int = 0

  match a { case Pass(_) { set ok = ok + 1 } otherwise { } }

  match b { case Pass(_) { set ok = ok + 1 } otherwise { } }

  match c { case Pass(_) { set ok = ok + 1 } otherwise { } }

  if ok == 3 { give 0 }

  if ok == 2 { give 11 }

  if ok == 1 { give 12 }

  give 13
}

// Point d'entree du scenario
entry main at core/app {
  let c1: TestCase = TestCase(1, -3, 0)
  let c2: TestCase = TestCase(2, 42, 42)
  let c3: TestCase = TestCase(3, 160, 100)
  let r1: CaseResult = run(c1)
  let r2: CaseResult = run(c2)
  let r3: CaseResult = run(c3)

  return ci_exit(r1, r2, r3)
}
```

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- 3 sur 3 réussis -> sortie 0.
- Régression partielle -> sortie 11.
- Échec global -> sortie 13.


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


## Mini Étude De Cas (Avant / Après)

Avant: logique métier et sortie technique mélangées, diagnostic coûteux.
Après: gardes d'entrée, décision métier, projection finale séparées; comportement plus lisible et testable.
Impact: revue plus rapide, régression plus facile à localiser.


## Ce Que Je Ferais En Revue De Code

1. Vérifier que les gardes d'entrée apparaissent avant les opérations sensibles.
2. Vérifier que la décision métier est séparée de la projection de sortie.
3. Vérifier un test nominal et un test limite réellement exécutables.
