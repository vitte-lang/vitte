# 61. Generation de diagrammes de grammaire

Niveau: Intermédiaire

Prérequis: `book/chapters/27-grammaire.md`.
Voir aussi: `book/grammar/railroad/README.md`.

## Objectif

Automatiser la generation des diagrammes railroad a partir de l'EBNF source.

## Workflow

1. Modifier la source EBNF unique.
2. Synchroniser artefacts de grammaire.
3. Regenerer diagrammes.
4. Verifier changements attendus.

## Commandes utiles

- `make grammar-sync`
- `make grammar-docs`
- `make grammar-gate`

## Exemples progressifs (N1 -> N3)

### N1 (base): règle unique

Snippet grammaire:

```vit
stmt ::= return_stmt | expr_stmt ;
```

Commande:

```bash
make grammar-docs
```

### N2 (intermédiaire): famille de règles

Snippet grammaire:

```vit
expr ::= assign_expr ;
assign_expr ::= or_expr [ "=" assign_expr ] ;
```

Commandes:

```bash
make grammar-sync
make grammar-docs
```

### N3 (avancé): régénération vérifiée

Snippet grammaire:

```vit
program ::= WS? { toplevel WS? } ;
```

Commandes:

```bash
make grammar-docs
make grammar-docs-check
```

### Anti-exemple

```text
Modifier manuellement les SVG/README générés.
```

## Validation rapide

1. Modifier la source unique.
2. Régénérer les artefacts.
3. Vérifier `--check` vert.

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

Thème: **generation de diagrammes de grammaire**. Cette section évite les généralités et part d'un extrait réel.

### Exemple A: lecture exécutable du snippet principal

```vit
stmt ::= return_stmt | expr_stmt ;
```

Lecture ligne par ligne:
1. `stmt ::= return_stmt | expr_stmt ;` -> participe au déroulé du traitement.

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

Exemple approfondi pour **generation diagrammes grammaire**: chaîne d'analyse complète (scan -> parse -> validation structurelle -> projection diagnostic).

```vit
// Exemple long: flux complet et vérifiable
space demo/generation-diagrammes-grammaire

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

Scénarios recommandés (generation diagrammes grammaire):
- Unité valide -> sortie 0.
- Entrée vide (bytes=0) -> sortie 101.
- Structure surdimensionnée -> sortie 202.
