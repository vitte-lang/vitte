# 53. Property-based testing pour le parser

Niveau: Avance

Prérequis: `book/chapters/27-grammaire.md`, `book/chapters/52-tests-grammaire-valid-invalid.md`.
Voir aussi: à définir.

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
  // Sortie programme: code de retour observable
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

Exemple approfondi pour **property based testing parser**: chaîne d'analyse complète (scan -> parse -> validation structurelle -> projection diagnostic).

```vit
// Exemple long: flux complet et vérifiable
space demo/property-based-testing-parser

form SourceUnit { bytes: int lines: int tokens_hint: int }
pick ParseState { case Parsed(nodes: int) case Failed(code: int) }

// Scan: transforme l'entrée brute en signal exploitable
proc scan(u: SourceUnit) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if u.bytes <= 0 { give 0 }
  // Garde: bloque un cas invalide avant de continuer
  if u.lines <= 0 { give 0 }
  // Sortie locale: valeur retournee par la procedure
  give (u.tokens_hint + u.lines)
}

// Parse: construit un état syntaxique déterministe
proc parse(token_count: int) -> ParseState {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if token_count == 0 { give ParseState.Failed(101) }
  // Garde: bloque un cas invalide avant de continuer
  if token_count < 4 { give ParseState.Failed(102) }
  // Sortie locale: valeur retournee par la procedure
  give ParseState.Parsed(token_count)
}

proc validate_structure(nodes: int) -> int {
  // Bloc logique: validations et gardes d'entree
  // Garde: bloque un cas invalide avant de continuer
  if nodes <= 0 { give 201 }
  // Garde: bloque un cas invalide avant de continuer
  if nodes > 200000 { give 202 }
  // Sortie locale: valeur retournee par la procedure
  give 0
}

// Projection finale: convertit l'état métier en code de sortie
proc to_exit(p: ParseState) -> int {
  // Bloc logique: decision par branches explicites
  // Match: decision explicite selon l'etat
  match p {
    case Parsed(n) {
      let v: int = validate_structure(n)
      // Garde: bloque un cas invalide avant de continuer
  if v != 0 { give v }
      // Sortie locale: valeur retournee par la procedure
  give 0
    }
    case Failed(c) { give c }
    otherwise { give 70 }
  }
}

// Orchestration: enchaîne les étapes sans logique cachée
entry main at core/app {
  let u: SourceUnit = SourceUnit(120, 12, 18)
  let t: int = scan(u)
  let p: ParseState = parse(t)
  // Sortie programme: code de retour observable
  return to_exit(p)
}
```

Scénarios recommandés (property based testing parser):
- Unité valide -> sortie 0.
- Entrée vide (bytes=0) -> sortie 101.
- Structure surdimensionnée -> sortie 202.
