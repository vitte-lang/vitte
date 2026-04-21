# 6. Modules simplement

Prérequis: `docs/book/poche/05-structures.md`.
Voir aussi: `docs/book/chapters/09-modules.md`.

## Objectif

Séparer le code en fichiers/modules lisibles.

## Exemple

```vit
space app/math
share add
```

## Pourquoi

Un module clair réduit les conflits et accélère la navigation.

## Test mental

Comment nommeriez-vous le module qui gère les utilisateurs?

## À faire

1. Créer un module `app/math`.
2. Exporter une procédure.
3. L'utiliser depuis `main`.

## Corrigé minimal

Nommer les modules par domaine métier, pas par “util” générique.
