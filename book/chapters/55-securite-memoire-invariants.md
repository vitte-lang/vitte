# 55. Securite memoire et invariants

Niveau: Avance

Prerequis: `book/chapters/12-pointeurs.md`, `book/chapters/16-interop.md`.

## Objectif

Prevenir corruptions memoire et comportements indetermines via invariants explicites.

## Invariants essentiels

1. Proprietaire clair de chaque buffer.
2. Duree de vie coherente des references.
3. Validation des bornes avant acces.
4. Contrats ABI explicites aux frontieres natives.

## Checklist

1. Invariants documentes dans le code.
2. Tests des cas limites memoire.
3. Verification sur chemins critiques.


## Exemples progressifs (N1 -> N3)

### N1 (base): borne explicite

```vit
proc safe_get(ok: bool, v: int) -> int {
  if not ok { give -1 }
  give v
}
```

### N2 (intermediaire): duree de vie controlee

```vit
proc keep_value(v: int) -> int {
  let x: int = v
  give x
}
```

### N3 (avance): invariants frontiere

```vit
proc abi_value(v: int) -> int { give v }
```

### Anti-exemple

```vit
proc unsafe_idx() -> int { give 0 }
# sans validation de contexte
```

## Validation rapide

1. Bornes verifiees.
2. Ownership clair.
3. Frontiere ABI testee.
