# 57. Gestion des erreurs semantiques

Niveau: Avance

Prérequis: `book/chapters/10-diagnostics.md`, `book/chapters/35-anatomie-message-erreur.md`.
Voir aussi: à définir.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Differencier clairement erreurs syntaxiques et erreurs semantiques pour accelerer la correction.

## Exemples d'erreurs semantiques

1. Symbole inconnu dans la portee.
2. Type incompatible dans une affectation.
3. Appel de procedure avec arite invalide.
4. Usage invalide d'un module/export.

## Bon diagnostic

1. Message precise la regle violee.
2. Position exacte + element attendu.
3. Suggestion de correction concrete.

## Exemples progressifs (N1 -> N3)

### N1 (base): symbole inconnu

```vit
entry main at app/demo {
  return unknown_name
}
```

### N2 (intermediaire): mismatch type

```vit
entry main at app/demo {
  let x: int = "bad"

  return x
}
```

### N3 (avance): diagnostic complet

```vit
proc f(a: int) -> int { give a }
entry main at app/demo { return f(1) }
```

### Anti-exemple

```vit
entry main at app/demo { return }
# message non actionnable si diagnostic pauvre
```

## Validation rapide

1. Regle semantique identifiee.
2. Position precise.
3. Suggestion de correction.

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

Thème: **gestion des erreurs semantiques**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/demo {
  return unknown_name
}
```

Lecture ligne par ligne:
1. `entry main at app/demo {` -> définit le point d'entrée du scénario.
2. `return unknown_name` -> renvoie la sortie vérifiable.
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
// Scenario gestion erreurs semantiques: execution complete et verifiable
space demo/gestion-erreurs-semantiques

form Event { code: int severity: int payload_len: int }
pick Diagnostic { case Info(code: int) case Warn(code: int) case Error(code: int) }

// Classification: mappe un événement vers un niveau explicite
proc classify(e: Event) -> Diagnostic {

  if e.code == 0 { give Diagnostic.Info(0) }

  if e.severity <= 2 { give Diagnostic.Warn(e.code) }

  give Diagnostic.Error(e.code)
}

// Redaction: borne la charge utile avant diffusion
proc redact(e: Event) -> int {

  if e.payload_len < 0 { give 81 }

  if e.payload_len > 4096 { give 82 }

  give 0
}

proc handle(e: Event) -> int {
  let r: int = redact(e)

  if r != 0 { give r }
  let d: Diagnostic = classify(e)

  match d {
    case Info(_) { give 0 }
    case Warn(_) { give 0 }
    case Error(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let e: Event = Event(17, 3, 120)

  return handle(e)
}
```

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- Niveau info ou warn -> sortie 0.
- Erreur métier code 17 -> sortie 17.
- Payload hors limites -> sortie 82.


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
