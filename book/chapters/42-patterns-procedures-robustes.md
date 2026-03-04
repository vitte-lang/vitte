# 42. Patterns de procedures robustes

Niveau: Intermediaire

Prerequis: `book/chapters/06-procedures.md`, `book/chapters/07-controle.md`.
Voir aussi: `book/chapters/18-tests.md`.

## Objectif

Ecrire des `proc` lisibles, testables et robustes face aux cas limites.

## Patterns recommandes

1. Gardes en tete (fail fast).
2. Contrat explicite en signature.
3. Unites de comportement courtes.
4. Retour deterministe pour chaque branche.

## Exemple minimal

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  if x > 65535 { give -1 }
  give x
}
```

## Anti-patterns

- Branches implicites sans `give` clair.
- Procedure trop longue avec plusieurs responsabilites.
- Melange validation, transformation et I/O.

## Checklist

1. Chaque branche a une sortie observable.
2. Les cas limites sont traites avant le nominal.
3. Un test couvre chaque garde.


## Exemples progressifs (N1 -> N3)

### N1 (base): garde minimale

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  give x
}
```

### N2 (intermediaire): bornes completes

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  if x > 65535 { give -1 }
  give x
}
```

### N3 (avance): comportement testable

```vit
proc parse_port_or_default(x: int, d: int) -> int {
  if x < 0 { give d }
  if x > 65535 { give d }
  give x
}
```

### Anti-exemple

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
}
```

## Validation rapide

1. Une sortie explicite par branche.
2. Cas limites testes.
3. Proc courte et lisible.
