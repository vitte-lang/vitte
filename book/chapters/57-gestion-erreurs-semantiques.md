# 57. Gestion des erreurs semantiques

Niveau: Avance

Prérequis: `book/chapters/10-diagnostics.md`, `book/chapters/35-anatomie-message-erreur.md`.
Voir aussi: à définir.

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
  // Sortie programme: code de retour observable
  return unknown_name
}
```

### N2 (intermediaire): mismatch type

```vit
entry main at app/demo {
  let x: int = "bad"
  // Sortie programme: code de retour observable
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
  // Sortie programme: code de retour observable
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

Exemple approfondi pour **gestion erreurs semantiques**: pipeline diagnostic (capture, classification, redaction, projection sortie).

```vit
// Exemple long: flux complet et vérifiable
space demo/gestion-erreurs-semantiques

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

Scénarios recommandés (gestion erreurs semantiques):
- Niveau info ou warn -> sortie 0.
- Erreur métier code 17 -> sortie 17.
- Payload hors limites -> sortie 82.
