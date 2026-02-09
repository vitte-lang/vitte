# 14. Macros

Les macros sont un outil tranchant. Elles peuvent clarifier un motif répétitif, mais elles peuvent aussi masquer le code. Vitte encourage une utilisation prudente.

## Un exemple simple

```vit
macro nop() {
  asm("nop")
}
```

## Quand les utiliser

Pour factoriser un motif purement syntaxique. Pour éviter une répétition qui ne porte pas de sens.

## Quand les éviter

Pour cacher des effets. Pour « simplifier » une logique complexe.

## Le coût cognitif

Chaque macro ajoute une couche de lecture. Utilisez‑les comme des outils de scalpel, pas comme des raccourcis universels.

## Erreurs courantes

Faire dépendre la logique métier d’une macro obscure. Cacher un appel système. Utiliser une macro alors qu’une fonction claire suffit.

## À retenir

Une macro doit rendre le code plus lisible, jamais l’inverse.


## Exemple guidé : macro vs fonction

Implémentez un motif avec une macro, puis avec une fonction. Gardez la version la plus lisible. Les macros sont rares par défaut.

## Checklist macros

La macro simplifie la lecture. Les effets sont visibles. La macro est documentée.


## Exercice : éliminer une macro

Prenez une macro existante et réécrivez‑la en fonction. Si la lisibilité augmente, gardez la fonction.


## Code complet (API actuelle)

```vit
macro assert(cond) {
  if !(cond) { builtin.trap("assert failed") }
}
```

## API idéale (future)

Une macro d’assertion avec message, fichier, et ligne réduirait le coût de débogage.

