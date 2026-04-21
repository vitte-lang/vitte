# 40. Gestion de compatibilite (breaking/non-breaking) (version poche)

Prérequis: `docs/book/poche/SOMMAIRE.md`.
Voir aussi: `docs/book/chapters/40-compatibilite-breaking.md`.

## Objectif

Comprendre gestion de compatibilite (breaking/non-breaking) de manière simple pour agir rapidement.

## Idée clé en 3 points

1. Identifier le besoin concret.
2. Appliquer la forme minimale correcte.
3. Vérifier le résultat avec un test court.

## Quand l'utiliser

1. Quand vous avez besoin d'une solution lisible immédiatement.
2. Quand vous voulez valider une base avant d'aller plus loin.
3. Quand vous devez expliquer rapidement le sujet à quelqu'un d'autre.

## Exemple minimal

```vit
proc load_user(id: int) -> int { give id }
proc load_user_safe(id: int) -> int { give id }
```

## Erreurs fréquentes

1. Vouloir couvrir tous les cas d'un coup.
2. Mélanger plusieurs changements dans la même étape.
3. Oublier de relire le message d'erreur exact.

## Mini-exercice (5 minutes)

Modifiez l'exemple pour créer une variante de gestion de compatibilite (breaking/non-breaking), puis vérifiez le résultat attendu.

## Checklist rapide

1. Le code compile.
2. Le résultat est compréhensible.
3. Le cas limite principal est testé.

## Corrigé minimal

Partir du cas nominal, faire une seule modification, recompiler, puis valider la sortie.

## Aller plus loin

Version complète: `docs/book/chapters/40-compatibilite-breaking.md`.
