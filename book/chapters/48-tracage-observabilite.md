# 48. Tracage et observabilite

Niveau: Avance

Prérequis: `book/chapters/47-debugage-reproductible.md`.
Voir aussi: à définir.

## Objectif

Rendre le comportement du programme observable en production et en test.

## Axes

1. Logs structures avec contexte minimal utile.
2. Correlation par identifiant de requete.
3. Metriques sur erreurs/latence/debit.
4. Traces sur chemins critiques.

## Checklist

1. Chaque erreur critique est tracable.
2. Les logs sont actionnables.
3. Les metriques permettent une alerte utile.

## Exemples progressifs (N1 -> N3)

### N1 (base): message contextualise

```vit
proc handle(req_id: int) -> int { give req_id }
```

### N2 (intermediaire): point d'erreur observable

```vit
proc handle_checked(ok: bool, code: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if not ok { give code }
  // Sortie locale: valeur retournee par la procedure
  give 0
}
```

### N3 (avance): correlation metrique/log

```vit
proc handle_trace(req_id: int, ok: bool) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if not ok { give req_id }
  // Sortie locale: valeur retournee par la procedure
  give 0
}
```

### Anti-exemple

```vit
proc handle_silent() -> int { give 0 }
```

## Validation rapide

1. Identifier contexte minimal dans logs.
2. Associer erreur a metrique.
3. Verifier exploitabilite en incident.

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

Thème: **tracage et observabilite**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
proc handle(req_id: int) -> int { give req_id }
```

Lecture ligne par ligne:
1. `proc handle(req_id: int) -> int { give req_id }` -> pose un contrat clair de fonction.

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

Exemple approfondi pour **tracage observabilite**: contrat ABI explicite (version, bornes, appel natif simulé, projection de code).

```vit
// Exemple long: flux complet et vérifiable
space demo/tracage-observabilite

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

Scénarios recommandés (tracage observabilite):
- ABI valide -> sortie 0.
- Version incompatible -> sortie 51.
- Payload hors contrat -> sortie 52 ou 53.
