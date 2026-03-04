# 51. Tests de non-regression compilateur

Niveau: Avance

Prerequis: `book/chapters/15-pipeline.md`, `book/chapters/31-erreurs-build.md`.

## Objectif

Eviter la reintroduction de bugs deja corriges dans parse, typecheck, backend.

## Strategie

1. Capturer chaque bug en test minimal.
2. Classer par couche de pipeline.
3. Verifier diagnostics attendus et code genere.
4. Bloquer merge si regression.

## Checklist

1. Test lie a un incident reel.
2. Reproducer minimal et stable.
3. Assertion exacte sur symptome corrige.


## Exemples progressifs (N1 -> N3)

### N1 (base): bug parse capture

```vit
emit 1
```

### N2 (intermediaire): bug type capture

```vit
entry main at app/demo {
  let x: int = "oops"
  return x
}
```

### N3 (avance): regression verrouillee

```vit
entry main at app/demo {
  let x: int = 1
  return x
}
```

### Anti-exemple

```vit
# fix sans test associe
```

## Validation rapide

1. Reproduire bug initial.
2. Ajouter test de non-regression.
3. Verifier absence de reintroduction.
