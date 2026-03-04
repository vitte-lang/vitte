# 53. Property-based testing pour le parser

Niveau: Avance

Prerequis: `book/chapters/27-grammaire.md`, `book/chapters/52-tests-grammaire-valid-invalid.md`.

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
