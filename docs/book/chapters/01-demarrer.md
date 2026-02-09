# 1. Démarrer

Ce chapitre est votre premier contact avec Vitte. L’objectif est simple : installer l’outil, lancer un build, comprendre la boucle de feedback, et savoir où regarder quand quelque chose ne marche pas. Nous ne cherchons pas la perfection, seulement une trajectoire qui vous permette d’avancer vite et proprement.

L’idée O’Reilly ici est claire : vous donner un terrain sûr et reproductible. Si vous gardez ce chapitre sous la main, vous pouvez toujours revenir à un état stable.

## Installation rapide

```sh
make build
```

Le binaire est disponible dans `bin/vitte`. Gardez cette commande en tête : si tout échoue, repartez d’un binaire propre. La règle d’or : ne déboguez jamais un outil que vous n’avez pas construit vous‑même.

## Vérifier l’environnement

```sh
vitte doctor
```

Le diagnostic doit être court et clair. S’il échoue, corrigez l’environnement avant de toucher au code. Beaucoup de problèmes « mystérieux » viennent d’une dépendance mal installée. Un compilateur est un écosystème, pas un exécutable isolé.

## Votre premier build

```sh
vitte build examples/syntax_features.vit
```

La première compilation est un petit rituel : elle vous rappelle que le langage existe sur votre machine, et que la boucle outil → code → binaire est bien en place. N’essayez pas de comprendre tout le code d’exemple ; utilisez‑le comme un test de santé.

## Lire un diagnostic

```sh
vitte explain E0001
```

Les codes d’erreur sont stables. Apprenez à les lire comme des balises : ils vous diront quoi chercher dans la doc ou dans le code du compilateur. Un code d’erreur bien choisi est une indexation du savoir.

## La boucle de feedback

Le secret d’une progression rapide est une boucle courte :

Écrire un petit changement. Lancer un check rapide. Lire l’erreur comme un guide. Corriger, puis recommencer.

C’est une discipline simple, mais elle transforme la productivité à long terme. Un compilateur qui donne un retour rapide est un multiplicateur de qualité.

## Quand ça casse

Quand tout semble cassé, revenez aux fondamentaux :

Rebuild complet. Exemple minimal dans un seul fichier. Reproduire le problème sans logique superflue.

Cette méthode est l’outil numéro un pour comprendre un langage émergent. Vous n’avez pas besoin d’être ingénieur compilateur : vous avez besoin d’isoler un fait.

## Règles simples pour bien commencer

Gardez les exemples courts. Utilisez `check` pour valider sans compiler. Lisez les codes d’erreur, pas seulement le message. Si un test échoue, réduisez le problème à un fichier et un cas minimal.

## Fil rouge : un mini‑outil

Nous reviendrons souvent à un petit outil en ligne de commande. L’idée est d’avoir un terrain stable pour tester vos nouvelles connaissances sans tout réécrire. Cette continuité vous permet de comparer vos décisions à travers les chapitres : « est‑ce que mon code est plus clair qu’avant ? ».

## Exercice : votre premier binaire propre

Créez un fichier `src/main.vit` avec une fonction `main` minimale. Lancez `check` puis `build`. Introduisez volontairement une erreur de type et lisez le diagnostic. Corrigez l’erreur et relancez le build.

Le but n’est pas d’apprendre une syntaxe, mais d’apprendre un cycle.

## À retenir

Apprendre Vitte, c’est apprendre une méthode de travail. Le langage vous aidera, mais la discipline de lecture, d’isolation, et de tests est ce qui fera la vraie différence.


## Exemple guidé : un programme qui échoue bien

Écrivez un programme qui ouvre un fichier, puis échouez volontairement. Le but est d’apprendre à produire un message clair.

Écrivez une version qui échoue avec un message pauvre. Améliorez le message avec le nom du fichier et l’action tentée. Ajoutez un code d’erreur stable (ou un enum interne).

Ce petit exercice vous apprend que « bien échouer » est une compétence, pas une conséquence.

## Pièges classiques de débutant

Confondre `check` et `build` et perdre du temps inutile. Modifier plusieurs fichiers à la fois, puis ne plus savoir ce qui a cassé. Lancer des builds longs alors qu’un check rapide aurait suffi.

## Checklist de démarrage

Le compilateur se construit localement. `doctor` retourne un statut propre. Le premier build réussit. Vous savez lire un diagnostic.

## Exercice long

Créez un petit outil `hello-vitte` qui :

Lit un argument `--name`. Affiche « Bonjour <name> ». Échoue proprement si l’argument est absent.

Le but n’est pas l’outil, mais la discipline : interface claire, erreurs claires, test simple.


## Étude de cas : diagnostic minimal

Vous compilez un fichier, et le compilateur affiche un diagnostic. Voici une méthode systématique :

Lisez le code d’erreur. Lisez la ligne indiquée. Supprimez tout le code non essentiel. Reproduisez l’erreur dans un fichier minimal.

Cette méthode est plus rapide que “deviner”. Elle vous force à transformer une émotion (“ça ne marche pas”) en un fait (“cette ligne provoque cette erreur”).

## Exercice long : projet « hello‑vitte »

Objectif : écrire un programme qui lit un argument `--name` et affiche un message.

Écrire un parser d’arguments minimal. Ajouter `--help`. Retourner un code d’erreur si `--name` est absent. Écrire un test manuel (exécuter avec et sans argument).

Ce projet est volontairement simple : il vous apprend la discipline de la boucle courte.


## Code complet (API actuelle)

```vit
use std/cli
use std/io/print
use std/core/option.Option

entry main at core/app {
  let args = args()
  if has_flag(args, "--help") {
    println_or_panic("usage: hello --name <name>")
    give 0
  }
  let name_opt = flag_value(args, "--name")
  when name_opt is Option.None {
    let _ = eprintln("error: missing --name")
    give 2
  }
  when name_opt is Option.Some {
    println_or_panic("Bonjour " + name_opt.value)
  }
  give 0
}
```

## API idéale (future)

Un module `std/cli/app` pourrait générer automatiquement `--help` et valider les arguments déclaratifs.

