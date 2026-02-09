# 9. Modules, use, pull, share

Un module est une frontière : ce qui est dedans est stable, ce qui est dehors est protégé par l’interface. En pratique, c’est le meilleur outil pour garder un projet lisible.

## Charger un module

```vit
use std/cli
```

Vous importez ce dont vous avez besoin, rien de plus. Ce geste devrait toujours être intentionnel : on ne « tire » pas un module pour voir si ça marche.

## Pull et partage

`pull` sert à importer du code local. `share` expose ce que vous voulez rendre public. Cette séparation force l’intention et limite les dépendances accidentelles.

## Choisir une architecture

Un module par responsabilité. Des noms courts, alignés sur le domaine. Une hiérarchie stable qui facilite la navigation.

## Interfaces fines

Une interface trop large invite les usages inattendus. Préférez une interface petite, lisible, et bien documentée. Un module qui fait peu, mais qui le fait bien, est une excellente brique.

## Dépendances explicites

Une dépendance implicite devient un bug implicite. Rendez vos dépendances visibles, et limitez‑les. C’est le meilleur moyen de garder un code portable.

## À retenir

La modularité est votre meilleure défense contre les dépendances confuses.


## Exemple guidé : scinder un module

Prenez un module qui gère à la fois I/O et parsing. Scindez‑le en deux modules. Vous verrez immédiatement les bénéfices en lisibilité.

## Erreurs courantes

Importer un module “par habitude”. Exposer trop de symboles publics. Créer une hiérarchie trop profonde.

## Checklist modules

Un module = une responsabilité. Les interfaces publiques sont petites. Les dépendances sont visibles.


## Exercice : modulariser un script

Prenez un script monolithique. Déplacez la logique d’I/O dans un module, et la logique métier dans un autre. Mesurez la différence de lisibilité.


## Code complet (API actuelle)

```vit
use std/cli
use std/io/print

entry main at core/app {
  let args = args()
  if has_flag(args, "--help") {
    println_or_panic("help")
  }
  give 0
}
```

## API idéale (future)

Un système de modules avec alias et reexports plus explicites pour les grands projets.

