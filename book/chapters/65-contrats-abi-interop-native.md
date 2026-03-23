# 65. Contrats ABI et interop native

Niveau: Avancé

Prérequis: `book/chapters/16-interop.md`, `book/chapters/55-securite-memoire-invariants.md`.
Voir aussi: à définir.

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

Exemple approfondi pour **contrats abi interop native**: contrat ABI explicite (version, bornes, appel natif simulé, projection de code).

```vit
// Exemple long: flux complet et vérifiable
space demo/contrats-abi-interop-native

form AbiEnvelope { version: int payload_size: int flags: int }
pick NativeCall { case Ok(code: int) case Err(code: int) }

proc abi_version() -> int { give 3 }

// Validation ABI: refuse toute incompatibilité avant appel natif
proc validate_abi(e: AbiEnvelope) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if e.version != abi_version() { give 51 }
  // Garde: bloque un cas invalide avant de continuer
  if e.payload_size <= 0 { give 52 }
  // Garde: bloque un cas invalide avant de continuer
  if e.payload_size > 4096 { give 53 }
  // Garde: bloque un cas invalide avant de continuer
  if e.flags < 0 { give 54 }
  // Sortie locale: valeur retournee par la procedure
  give 0
}

// Appel natif simulé: exécution seulement si le contrat est valide
proc call_native(e: AbiEnvelope) -> NativeCall {
  let v: int = validate_abi(e)
  // Garde: bloque un cas invalide avant de continuer
  if v != 0 { give NativeCall.Err(v) }
  // Garde: bloque un cas invalide avant de continuer
  if e.payload_size % 2 == 0 { give NativeCall.Ok(0) }
  // Sortie locale: valeur retournee par la procedure
  give NativeCall.Err(55)
}

// Projection finale: convertit l'état métier en code de sortie
proc to_exit(r: NativeCall) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match r {
    case Ok(c) { give c }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let e: AbiEnvelope = AbiEnvelope(3, 128, 1)
  let r: NativeCall = call_native(e)
  // Sortie programme: code de retour observable
  return to_exit(r)
}
```

Scénarios recommandés (contrats abi interop native):
- ABI valide -> sortie 0.
- Version incompatible -> sortie 51.
- Payload hors contrat -> sortie 52 ou 53.
