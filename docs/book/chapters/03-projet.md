# 3. Anatomie d’un projet

Dans ce chapitre, on définit une structure de projet simple, claire, et reproductible. Un projet Vitte doit être lisible en un seul coup d’œil. La structure ne doit pas être décorative : elle doit permettre à un nouveau contributeur de comprendre où est le cœur du code en moins de cinq minutes.

## Structure minimale

`src/` pour le code. `tests/` pour les tests. `Makefile` pour les commandes de build.

Le choix n’est pas unique, mais l’important est la cohérence et la simplicité.

## Un fichier principal

Un programme exécutable commence par une entrée claire :

```vit
entry main at core/app {
  return 0
}
```

C’est la première ligne de contrat entre votre code et la plateforme.

## Des modules clairs

Organisez votre code en modules qui ont chacun un rôle net. Si un module commence à tout faire, c’est un signal pour le scinder. La modularité n’est pas une règle esthétique : c’est une méthode pour limiter la propagation des erreurs.

## Une dépendance est une décision

Quand vous ajoutez un module, vous ajoutez aussi une surface de maintenance. Posez toujours la question : « cet ajout simplifie‑t‑il l’ensemble ? » Si la réponse est non, vous êtes en train de payer la dette d’une abstraction.

## Construire un pipeline local

L’objectif est de pouvoir :

Lancer un check rapide. Compiler en mode debug. Compiler en mode release.

Un Makefile simple fait déjà beaucoup. On le détaillera plus loin dans le livre.

## Les chemins de build

Séparez ce qui est temporaire de ce qui est durable. Les artefacts de build ne doivent pas polluer le code source. Un projet propre est un projet qui se nettoie facilement.

## Le coût d’un mauvais projet

Une structure confuse ralentit les diagnostics et décourage les contributions. Prenez ce temps au début, vous le récupérerez en fin de sprint.

## Exercice : refactorer un projet minimal

Créez un dossier `src/` et un fichier `main.vit`. Ajoutez un module `io` dans `src/io.vit`. Déplacez la logique d’entrée/sortie dans ce module. Vérifiez que l’interface reste claire.

## À retenir

Un projet Vitte bien organisé est un projet qui se débogue facilement.


## Exemple de Makefile minimal

Un Makefile n’a pas besoin d’être complexe. Trois cibles suffisent : `check`, `build`, `test`. La valeur est dans la constance.

## Erreurs courantes de structure

Mettre tout dans `src/main.vit`. Mélanger tests et code de production. Avoir un `Makefile` qui cache la logique au lieu de la rendre visible.

## Checklist projet

Les dossiers ont un rôle clair. Le point d’entrée est facile à trouver. Les tests sont dans un dossier dédié. Le build ne pollue pas la racine.


## Exemple filé

Imaginez un outil `logscan` qui lit un fichier, filtre des lignes, et affiche un résumé.

`src/main.vit` contient l’entrée. `src/scan.vit` contient la logique de parsing. `src/format.vit` contient l’affichage. `tests/` contient un échantillon et un test de non‑régression.

Ce découpage vous donne un “plan de ville”. Sans plan, vous vous perdez.


## Code complet (API actuelle)

Structure minimale de projet :

`src/main.vit`. `src/lib.vit`. `tests/`.

Exemple d’entrée :

```vit
entry main at core/app {
  return 0
}
```

## API idéale (future)

Un fichier manifeste (`Vitte.toml`) pourrait décrire le projet, les cibles, et les flags.

