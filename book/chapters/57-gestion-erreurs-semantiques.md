# 57. Gestion des erreurs semantiques

Niveau: Avance

Prerequis: `book/chapters/10-diagnostics.md`, `book/chapters/35-anatomie-message-erreur.md`.

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
  return unknown_name
}
```

### N2 (intermediaire): mismatch type

```vit
entry main at app/demo {
  let x: int = "bad"
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
