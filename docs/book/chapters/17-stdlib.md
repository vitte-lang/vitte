# 17. Standard library (tour guidé)

La bibliothèque standard est volontairement pragmatique. Elle vise à couvrir l’essentiel sans cacher les détails importants.

## Navigation

Commencez par les modules de base : I/O, OS, process, cli. Ce sont ceux qui reviennent dans presque tous les projets.

## Exemple rapide

```vit
use std/cli

entry main at core/app {
  let args = args()
  let _ = has_flag(args, "--help")
  return 0
}
```

## Philosophie

La stdlib propose des primitives lisibles, pas un framework. Elle vous accompagne sans vous enfermer.

## Comment la lire

Lisez les modules comme des mini‑bibliothèques. Cherchez les types principaux, puis les fonctions d’entrée. Une bonne lib se comprend en trois étapes : surface, détails, contraintes.

## Erreurs courantes

Utiliser un module parce qu’il existe, pas parce qu’il est nécessaire. Appeler la stdlib sans lire la signature.

## À retenir

La stdlib est un outil, pas un mode de vie.


## Exemple guidé : lire et écrire

Prenez un module I/O, écrivez un petit utilitaire, puis réduisez‑le pour isoler l’essentiel. La stdlib se comprend en pratique.

## Checklist stdlib

Vous connaissez les modules essentiels. Vous lisez les signatures avant d’utiliser. Vous évitez les dépendances inutiles.


## Exercice : petit utilitaire

Écrivez un utilitaire de ligne de commande qui lit un fichier et compte les lignes. L’objectif est de manipuler deux modules std et de rester lisible.


## Code complet (API actuelle)

```vit
use std/cli
use std/io/print

entry main at core/app {
  let args = args()
  if has_flag(args, "--help") {
    println_or_panic("usage: tool --help")
    return 0
  }
  println_or_panic("ok")
  return 0
}
```

## API idéale (future)

Un module `std/cli/app` pourrait générer automatiquement `--help` et valider les arguments.

