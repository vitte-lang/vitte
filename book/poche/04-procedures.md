# 4. Procédures

Prérequis: `book/poche/03-conditions-et-boucles.md`.
Voir aussi: `book/chapters/06-procedures.md`.

## Objectif

Extraire du code dans une `proc` réutilisable.

## Exemple

```vit
proc add(a: int, b: int) -> int {
  give a + b
}

entry main at app/demo {
  return add(2, 3)
}
```

## Pourquoi

Une procédure réduit la duplication et rend le code testable.

## Test mental

Que faut-il modifier pour additionner 3 nombres?

## À faire

1. Créer `mul(a, b)`.
2. L'appeler depuis `main`.
3. Comparer résultats.

## Corrigé minimal

Une procédure = un objectif clair + un type de retour explicite.
