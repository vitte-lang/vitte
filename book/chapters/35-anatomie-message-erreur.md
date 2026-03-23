# 35. Anatomie d'un message d'erreur

Niveau: Intermediaire

Prérequis: `book/chapters/10-diagnostics.md`, `book/chapters/31-erreurs-build.md`.
Voir aussi: `book/grammar/diagnostics/expected`.

## Objectif

Lire un diagnostic en moins de 30 secondes: categorie, position, cause probable, correction.

## Anatomie standard

1. Code erreur (`E..`).
2. Position (fichier, ligne, colonne).
3. Message principal (ce qui est attendu vs trouve).
4. Contexte (ligne source, zone marquee).
5. Suggestion (action de correction).

## Exemple de lecture

Erreur: `expected top-level declaration` sur `emit 1` en ligne 1.

Interpretation:
- Couche: parse.
- Cause probable: instruction au top-level.
- Fix minimal: encapsuler dans `entry` ou `proc`.

## Strategie de correction

1. Ne corriger que la premiere erreur.
2. Relancer le compilateur.
3. Re-evaluer les erreurs restantes.
4. Eviter les modifications en cascade sans preuve.

## Checklist

1. Copier le message exact.
2. Identifier la couche (`parse`, `type`, `link`).
3. Valider la correction sur un cas minimal.
4. Ajouter un test de non-regression.

## Exemples progressifs (N1 -> N3)

### N1 (base): diagnostic syntaxique lisible

```vit
entry main at app/demo {
  // Sortie programme: code de retour observable
  return 0
}
```

### N2 (intermediaire): diagnostic token inattendu

```vit
entry main at app/demo {
  return
}
```

### N3 (avance): diagnostic semantique

```vit
entry main at app/demo {
  // Sortie programme: code de retour observable
  return unknown_value
}
```

### Anti-exemple

```vit
entry main at app/demo {
  // Sortie programme: code de retour observable
  return 0
  // Sortie programme: code de retour observable
  return 1
}
```

## Validation rapide

1. Identifier code + position du premier diagnostic.
2. Corriger une erreur a la fois.
3. Relancer jusqu'a zero erreur.

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

Thème: **anatomie d'un message d'erreur**. Cette section évite les généralités et part d'un extrait réel.

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

Exemple approfondi pour **anatomie message erreur**: pipeline diagnostic (capture, classification, redaction, projection sortie).

```vit
// Exemple long: flux complet et vérifiable
space demo/anatomie-message-erreur

form Event { code: int severity: int payload_len: int }
pick Diagnostic { case Info(code: int) case Warn(code: int) case Error(code: int) }

// Classification: mappe un événement vers un niveau explicite
proc classify(e: Event) -> Diagnostic {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if e.code == 0 { give Diagnostic.Info(0) }
  // Garde: bloque un cas invalide avant de continuer
  if e.severity <= 2 { give Diagnostic.Warn(e.code) }
  // Sortie locale: valeur retournee par la procedure
  give Diagnostic.Error(e.code)
}

// Redaction: borne la charge utile avant diffusion
proc redact(e: Event) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if e.payload_len < 0 { give 81 }
  // Garde: bloque un cas invalide avant de continuer
  if e.payload_len > 4096 { give 82 }
  // Sortie locale: valeur retournee par la procedure
  give 0
}

proc handle(e: Event) -> int {
  let r: int = redact(e)
  // Garde: bloque un cas invalide avant de continuer
  if r != 0 { give r }
  let d: Diagnostic = classify(e)
  // Match: decision explicite selon l'etat
  match d {
    case Info(_) { give 0 }
    case Warn(_) { give 0 }
    case Error(c) { give c }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let e: Event = Event(17, 3, 120)
  // Sortie programme: code de retour observable
  return handle(e)
}
```

Scénarios recommandés (anatomie message erreur):
- Niveau info ou warn -> sortie 0.
- Erreur métier code 17 -> sortie 17.
- Payload hors limites -> sortie 82.
