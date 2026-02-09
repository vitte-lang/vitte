# 18. Tests et outillage

Un bon test est un document vivant. Il capture l’intention et vous protège contre les régressions. Dans un langage système, un test est aussi un garde‑fou de sécurité.

## Types de tests

Tests unitaires pour les fonctions critiques. Tests d’intégration pour les flux complets. Tests de non‑régression pour les bugs corrigés.

## L’outillage

Le principe est simple : un test doit être rapide à lancer et facile à comprendre. Les scripts de test du repo sont pensés dans cet esprit.

## Un bon test

Un bon test est court, précis, et raconte une histoire. Si vous devez relire le test trois fois pour comprendre ce qu’il fait, le test est trop complexe.

## Tests comme documentation

Un test est une preuve. Quand vous revenez six mois plus tard, c’est souvent plus clair qu’un commentaire. Écrivez des tests comme des exemples explicites.

## Erreurs courantes

Tester trop de choses dans un seul test. Écrire un test qui dépend de l’ordre d’exécution. Utiliser des données aléatoires sans seed.

## À retenir

Les tests ne sont pas un coût, ils sont une assurance.


## Exemple guidé : un test de bug

Reproduisez un bug réel (même petit), puis écrivez un test minimal. C’est une pratique clé pour un langage en évolution.

## Checklist tests

Tests courts. Tests stables. Tests qui racontent une histoire.


## Exercice : test minimal

Prenez un bug corrigé et écrivez un test qui échoue avant le fix et passe après. Ce test est votre mémoire technique.


## Code complet (API actuelle)

Exemple : test de non‑régression (conceptuel) basé sur un binaire.

```sh
vitte check tests/case.vit
```

## API idéale (future)

Un framework de test intégré (assertions, fixtures, snapshots) simplifierait l’écriture de tests.

