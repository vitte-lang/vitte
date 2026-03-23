# 65. Contrats ABI et interop native

Niveau: Avancé

Prérequis: `book/chapters/16-interop.md`, `book/chapters/55-securite-memoire-invariants.md`.
Voir aussi: à définir.

## Problème Concret

Contexte réel: un flux de traitement doit rester lisible, testable et deterministic même quand l'entrée est partielle ou invalide.
Avant de parler syntaxe, ce chapitre répond à une question pratique: **quelle décision prend le code et pourquoi**.

## Fil Rouge (Projet Unique)

Mini-projet suivi: **OpsTicket** (ingestion, validation, decision, sortie).
Chaque chapitre modifie une partie du meme flux pour garder la continuité technique.

## Objectif

Stabiliser la frontiere Vitte/natif avec des contrats ABI explicites.

## Points critiques

1. Signatures coherentes entre mondes.
2. Tailles/alignements de types verifies.
3. Convention d'appel documentee.
4. Gestion claire de la mémoire partagee.

## Checklist

1. Tests d'interop automatiques.
2. Compatibilité multi-plateforme validee.
3. Strategie de version ABI definie.

## Exemples progressifs (N1 -> N3)

### N1 (base): frontière simple

Snippet Vitte:

```vit
proc host_add(a:int,b:int)->int { give a+b }
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): cohérence de type

Snippet Vitte:

```vit
proc host_add64(a:i64,b:i64)->i64 { give a+b }
```

Commandes:

```bash
make grammar-test
```

### N3 (avancé): compatibilité versionnée + runbook

Snippet Vitte:

```vit
proc host_add_v1(a:int,b:int)->int { give a+b }
proc host_add_v2(a:i64,b:i64)->i64 { give a+b }
```

Runbook:

```bash
# 1. verifier signatures Vitte/natif
# 2. verifier tailles/alignements
# 3. valider CI multi-cible
make grammar-gate
```

### Anti-exemple

```text
Changer ABI natif sans version ni plan de migration.
```

## Validation rapide

1. Aligner signatures.
2. Vérifier layout mémoire.
3. Vérifier tests interop.

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

Thème: **contrats abi et interop native**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc host_add(a:int,b:int)->int { give a+b }
```

Lecture ligne par ligne:
1. `proc host_add(a:int,b:int)->int { give a+b }` -> pose un contrat clair de fonction.

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
// Scenario contrats abi interop native: execution complete et verifiable
space demo/contrats-abi-interop-native

form AbiEnvelope { version: int payload_size: int flags: int }
pick NativeCall { case Ok(code: int) case Err(code: int) }

proc abi_version() -> int { give 3 }

// Validation ABI: refuse toute incompatibilité avant appel natif
proc validate_abi(e: AbiEnvelope) -> int {

  if e.version != abi_version() { give 51 }

  if e.payload_size <= 0 { give 52 }

  if e.payload_size > 4096 { give 53 }

  if e.flags < 0 { give 54 }

  give 0
}

// Appel natif simulé: exécution seulement si le contrat est valide
proc call_native(e: AbiEnvelope) -> NativeCall {
  let v: int = validate_abi(e)

  if v != 0 { give NativeCall.Err(v) }

  if e.payload_size % 2 == 0 { give NativeCall.Ok(0) }

  give NativeCall.Err(55)
}

// Conversion finale vers un code de sortie
proc to_exit(r: NativeCall) -> int {

  match r {
    case Ok(c) { give c }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  let e: AbiEnvelope = AbiEnvelope(3, 128, 1)
  let r: NativeCall = call_native(e)

  return to_exit(r)
}
```

## Design Notes

- Le snippet privilégie des frontières explicites plutôt qu'un code minimaliste.
- Les gardes sont placées tôt pour réduire le coût de diagnostic.
- La sortie est projetée en fin de flux pour garder le métier indépendant du transport.


Cas limite réel:
- Entree degradee ou incomplete: la garde doit couper le flux tot avec une sortie explicite.

A tester:
- ABI valide -> sortie 0.
- Version incompatible -> sortie 51.
- Payload hors contrat -> sortie 52 ou 53.


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
