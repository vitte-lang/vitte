# 3. Conditions et boucles

Prérequis: `book/poche/02-variables-et-types.md`.
Voir aussi: `book/chapters/07-controle.md`.

## Objectif

Choisir un chemin (`if`) et répéter une action (`loop`/`for`).

## Exemple

```vit
entry main at app/demo {
  let n: int = 3
  if n > 0 {
    return 1
  }
  return 0
}
```

## Pourquoi

Le contrôle de flux est la logique centrale d'un programme.

## Test mental

Quelle sortie obtenez-vous si `n = 0`?

## À faire

1. Modifier la condition.
2. Tester 3 valeurs d'entrée.
3. Noter les sorties.

## Corrigé minimal

Toujours écrire d'abord le cas nominal, puis le cas limite.
