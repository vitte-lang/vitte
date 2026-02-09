# 19. Performance et profiling

Optimiser trop tôt est une erreur, mais ignorer la performance est une dette. Ce chapitre propose une méthode simple : mesurer, comprendre, optimiser.

## Mesurer

Avant toute optimisation, collectez des chiffres. Un gain supposé est rarement un gain réel.

## Hot paths

Concentrez‑vous sur les chemins chauds. La performance est un problème de priorité, pas de perfection.

## Optimisation lisible

Un micro‑gain qui détruit la lisibilité n’est pas une victoire. Le meilleur code rapide est celui qui reste clair.

## Règle de trois

Mesurer. Comprendre. Modifier.

Si vous sautez une étape, vous n’optimisez pas : vous devinez.

## Erreurs courantes

Optimiser une partie non critique. Cacher une allocation qui aurait pu être visible. Sacrifier la clarté pour un gain marginal.

## À retenir

La meilleure optimisation est celle qui simplifie le code tout en accélérant le chemin critique.


## Exemple guidé : mesurer un hot path

Choisissez une fonction lente, mesurez‑la, modifiez un point, puis mesurez à nouveau. La performance sans mesure est un mythe.

## Checklist perf

Mesure avant modification. Modification claire. Mesure après modification.


## Exercice : mesurer avant d’optimiser

Créez une boucle volontairement lente, mesurez‑la, puis optimisez un seul point. Si le gain n’est pas mesuré, annulez l’optimisation.


## Code complet (API actuelle)

Exemple minimal : compiler et mesurer la version de base avant toute optimisation.

```sh
vitte build src/main.vit
```

## API idéale (future)

Un outil `vitte perf` qui collecte automatiquement des compteurs et produit un rapport.

