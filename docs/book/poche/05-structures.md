# 5. Structures de données

Prérequis: `docs/book/poche/04-procedures.md`.
Voir aussi: `docs/book/chapters/08-structures.md`.

## Objectif

Regrouper des données liées dans une structure.

## Exemple

```vit
form User {
  id: int
  name: string
}
```

## Pourquoi

Les structures évitent les paramètres dispersés et confus.

## Test mental

Que se passe-t-il si vous ajoutez `email: string`?

## À faire

1. Créer `User`.
2. Ajouter un champ.
3. Adapter le code utilisateur.

## Corrigé minimal

Commencer par 2 ou 3 champs maximum, puis évoluer petit à petit.
