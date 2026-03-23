# 2. Variables et types

Prérequis: `book/poche/01-premier-programme.md`.
Voir aussi: `book/chapters/05-types.md`.

## Objectif

Déclarer une variable et comprendre son type.

## Exemple

```vit
entry main at app/demo {
  let age: int = 42
  return age
}
```

## Pourquoi

Un type explicite évite beaucoup d'erreurs invisibles.

## Test mental

Si `age` devient `string`, que faut-il changer?

## À faire

1. Créer 2 variables (`int`, `string`).
2. Retourner une valeur simple.
3. Vérifier les erreurs éventuelles.

## Corrigé minimal

Conserver `let nom: type = valeur` partout en début d'apprentissage.
