# 43. Anti-patterns frequents en code Vitte (version poche)

Prérequis: `book/poche/SOMMAIRE.md`.
Voir aussi: `book/chapters/43-anti-patterns-code-vitte.md`.

## Objectif

Comprendre anti-patterns frequents en code de manière simple pour agir rapidement.

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
proc process(x: int, y: int, z: int) -> int {
  if x < 0 { give -1 }
  if y < 0 { give -1 }
  if z < 0 { give -1 }
  give x + y + z
}
```

## Erreurs fréquentes

1. Vouloir couvrir tous les cas d'un coup.
2. Mélanger plusieurs changements dans la même étape.
3. Oublier de relire le message d'erreur exact.

## Mini-exercice (5 minutes)

Modifiez l'exemple pour créer une variante de anti-patterns frequents en code, puis vérifiez le résultat attendu.

## Checklist rapide

1. Le code compile.
2. Le résultat est compréhensible.
3. Le cas limite principal est testé.

## Corrigé minimal

Partir du cas nominal, faire une seule modification, recompiler, puis valider la sortie.

## Aller plus loin

Version complète: `book/chapters/43-anti-patterns-code-vitte.md`.
