# 7. Contrôle de flux

Le contrôle de flux est la grammaire de votre logique. Si elle est confuse, votre programme le sera aussi. La simplicité des branches est une forme de robustesse.

## Conditions

Les conditions doivent rester lisibles. Évitez les expressions trop denses.

```vit
entry main at core/app {
  if ready { return 0 }
  return 1
}
```

## Boucles

Vitte propose des formes simples :

```vit
entry main at core/app {
  loop {
    if done { break }
  }
  for item in items {
    continue
  }
  return 0
}
```

## Match

`match` est utile quand plusieurs cas sont possibles. Utilisez‑le pour clarifier l’intention.

```vit
entry main at core/app {
  match 1 {
    case x { }
    otherwise { }
  }
  return 0
}
```

## Lisibilité avant tout

Un flux de contrôle trop dense rend les bugs invisibles. Privilégiez les branches courtes et les retours explicites.

## Gérer l’erreur tôt

Dans un code système, les erreurs sont normales. La bonne stratégie consiste à les traiter tôt, puis à continuer sur un chemin propre. C’est moins élégant, mais beaucoup plus fiable.

## Sortir proprement

Un retour clair vaut mieux qu’un enchaînement de drapeaux. Le lecteur doit savoir à quel moment la fonction se termine, et pourquoi.

## À retenir

La clarté du flux est plus importante que la concision. Les branches doivent être lisibles en un coup d’œil. Traiter l’erreur tôt améliore tout le reste.


## Exemple guidé : erreurs d’abord

Réécrivez une fonction qui accumule des erreurs en fin de fonction. Transformez‑la pour gérer les erreurs dès qu’elles arrivent. Comparez la clarté.

## Erreurs courantes

Utiliser des flags multiples pour décrire l’état. Écrire des conditions imbriquées trop profondes. Oublier un cas dans `match`.

## Checklist flux

Les branches sont courtes. Les sorties sont explicites. Les erreurs sont gérées tôt.


## Exercice : aplatir une logique

Transformez une chaîne de `if/else` imbriqués en `match` ou en retours précoces. Lisez ensuite votre code à voix haute : est‑il plus facile à expliquer ?


## Code complet (API actuelle)

```vit
use std/kernel/fs
use std/core/types.i32

proc open_or_fail(path: string) -> i32 {
  let fd = fs.open_read(path)
  if fd < 0 { give -1 }
  give fd
}
```

## API idéale (future)

Un type `Result` pour les opérations système afin d’éviter les valeurs sentinelles.

