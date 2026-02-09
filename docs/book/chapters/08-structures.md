# 8. Structures et enums

Les structures et enums servent à exprimer des données avec intention. Le lecteur doit comprendre « ce que c’est » avant de comprendre « ce que ça fait ».

## Structures

Une structure regroupe des champs liés. Préférez des structures petites et cohérentes.

Exemple conceptuel :

```vit
struct Point {
  x: i32
  y: i32
}
```

## Enums

Les enums expriment un choix fini. Ils sont utiles pour rendre les états explicites.

```vit
enum Status {
  Ok
  Err
}
```

## Modéliser l’intention

Un enum bien nommé vous évite des dizaines de commentaires. Il dit clairement « voici toutes les options possibles ». C’est un outil de précision, pas un gadget.

## Pas de structures “fourre‑tout”

Une structure qui finit par contenir « un peu de tout » devient vite un stockage sans intention. Quand vous sentez ce glissement, découpez‑la.

## Champs dérivés

Évitez de stocker des champs qui peuvent être recalculés sans coût significatif. Cela réduit les risques d’incohérence. Une structure doit rester une vérité unique.

## À retenir

Une structure est un nom pour un groupe d’invariants. Une enum est un nom pour un choix fini. Si la syntaxe exacte change, conservez l’intention : donner un nom à un groupe stable.


## Exemple guidé : refactorer une structure

Prenez une structure “fourre‑tout” et découpez‑la en deux structures plus petites. Ajoutez un enum pour rendre les états explicites.

## Erreurs courantes

Stocker des champs dérivés. Utiliser des structures énormes pour éviter de réfléchir au modèle. Mettre des valeurs optionnelles sans les signaler.

## Checklist structures

Chaque champ a une raison d’être. Les champs dérivés sont évités. Les invariants sont documentés.


## Exercice : structurer un état

Imaginez un downloader avec trois états : “en attente”, “en cours”, “terminé”. Écrivez un enum pour ces états, et évitez les drapeaux booléens multiples.


## Code complet (API actuelle)

```vit
form Config {
  port: i32
  host: string
}

pick State {
  Idle
  Running
  Failed(code: i32)
}
```

## API idéale (future)

Un support plus direct pour les “optionnels” (champ nullable) sans `Option` explicite.

