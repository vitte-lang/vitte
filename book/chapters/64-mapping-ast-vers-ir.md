# 64. Mapping AST -> IR (vue pratique)

Niveau: Avancé

Prérequis: `book/chapters/15-pipeline.md`, `book/chapters/27-grammaire.md`.
Voir aussi: à définir.

## Objectif

Comprendre comment les noeuds AST sont transformes en IR exploitable par le backend.

## Etapes

1. Parser produit AST structure.
2. Resolve annote symboles et types.
3. Lowering AST -> IR normalise.
4. Backend consomme IR.

## Checklist

1. Invariants AST explicites.
2. Invariants IR verifiables.
3. Tests de non-regression sur transformations.

## Exemples progressifs (N1 -> N3)

### N1 (base): AST expression

Snippet Vitte:

```vit
entry main at app/demo { return 1 + 2 }
```

Commande:

```bash
make grammar-check
```

### N2 (intermédiaire): AST conditionnel

Snippet Vitte:

```vit
entry main at app/demo {
  if true { return 1 }
  // Sortie programme: code de retour observable
  return 0
}
```

Commande:

```bash
make grammar-test
```

### N3 (avancé): mapping stable AST -> IR

Snippet Vitte:

```vit
proc add(a:int,b:int)->int { give a+b }
entry main at app/demo { return add(1,2) }
```

Commandes:

```bash
make grammar-gate
make book-qa
```

### Anti-exemple

```text
Transformer AST->IR sans invariants documentés.
```

## Validation rapide

1. Identifier AST attendu.
2. Vérifier invariants IR.
3. Tester non-régression de transformation.

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

Thème: **mapping ast -> ir (vue pratique)**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
entry main at app/demo { return 1 + 2 }
```

Lecture ligne par ligne:
1. `entry main at app/demo { return 1 + 2 }` -> définit le point d'entrée du scénario.

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

Exemple approfondi pour **mapping ast vers ir**: chaîne d'analyse complète (scan -> parse -> validation structurelle -> projection diagnostic).

```vit
// Exemple long: flux complet et vérifiable
space demo/mapping-ast-vers-ir

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

Scénarios recommandés (mapping ast vers ir):
- Unité valide -> sortie 0.
- Entrée vide (bytes=0) -> sortie 101.
- Structure surdimensionnée -> sortie 202.
