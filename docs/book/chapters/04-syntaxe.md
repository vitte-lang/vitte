# 4. Syntaxe de base

La syntaxe de Vitte est volontairement sobre. Elle évite les surprises et favorise les formes régulières. Si vous venez de C, vous reconnaîtrez des formes familières, sans les angles morts historiques.

## Valeurs et expressions

Les expressions sont directes : vous lisez ce que la machine exécutera. Le style recommandé est d’écrire des expressions courtes, puis de les nommer.

```vit
entry main at core/app {
  let x = 1
  let y = x + 2
  return y
}
```

## Blocs et portée

Les blocs définissent la portée des noms. Si vous réutilisez un identifiant, faites‑le consciemment et localement ; les variables globales sont une dette à long terme.

## Indentation et lisibilité

Vous écrivez pour la machine, mais aussi pour un futur lecteur. L’indentation cohérente, les lignes courtes, et les noms précis sont des optimisations humaines.

## Commentaires

Les commentaires sont là pour expliquer une intention, pas pour répéter le code. Un bon commentaire répond à « pourquoi ? ».

## Style minimal

Un bon code Vitte n’est pas forcément « court », mais il est souvent « plat ». Évitez les cascades de conditions, les expressions trop imbriquées, et les effets cachés.

## Densité de lecture

Un lecteur humain n’a pas la patience d’un compilateur. Si un bloc demande plus d’une respiration pour être compris, il est peut‑être trop dense. Découper est un acte de respect.

## À retenir

Une syntaxe courte vaut mieux qu’une syntaxe clever. Nommez tôt, testez tôt, refactorez tôt. La lisibilité est une performance à long terme.


## Exemple long : du code lisible

Prenez un bloc trop dense, puis découpez‑le. Le but est d’apprendre à réduire la charge cognitive.

Avant : une expression longue, imbriquée.
Après : trois lignes avec des noms clairs.

## Erreurs courantes

Utiliser des noms courts pour des concepts longs. Mélanger déclaration et logique dans la même ligne. Empiler plusieurs effets dans une seule expression.

## Checklist de lisibilité

Une ligne, une idée. Chaque variable a un nom lisible. Les blocs ne dépassent pas une taille raisonnable.


## Exercice : rendre lisible

Prenez une fonction avec trois `if` imbriqués et refactorez‑la en retours précoces. Comparez les deux versions. La version lisible est souvent plus courte, mais surtout plus honnête.


## Code complet (API actuelle)

```vit
entry main at core/app {
  let x = 1
  let y = x + 2
  if y > 2 { return 0 }
  return 1
}
```

## API idéale (future)

Des conventions standardisées pour les blocs `match` et les guard‑clauses.

