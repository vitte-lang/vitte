# 54. Fuzzing de l'entree source

Niveau: Avance

Prerequis: `book/chapters/53-property-based-testing-parser.md`.

## Objectif

Renforcer robustesse parseur/diagnostics face a des entrees arbitraires.

## Cibles

1. Lexer.
2. Parser.
3. Generation de diagnostics.

## Garde-fous

1. Timeout par execution.
2. Isolation des crashes.
3. Archivage des corpus qui declenchent un bug.


## Exemples progressifs (N1 -> N3)

### N1 (base): fuzz token simple

```vit
entry main at app/demo { return 0 }
```

### N2 (intermediaire): fuzz structure bloc

```vit
entry main at app/demo {
  if true { return 0 }
  return 1
}
```

### N3 (avance): fuzz corpus continu

```vit
match 1 {
  case 1 { give 1 }
  otherwise { give 0 }
}
```

### Anti-exemple

```vit
# crash detecte mais non archive dans corpus
```

## Validation rapide

1. Timeout par cas actif.
2. Crashes archives.
3. Cas crashants convertis en tests.
