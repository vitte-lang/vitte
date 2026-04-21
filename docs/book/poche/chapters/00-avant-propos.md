# 00. Avant-propos (version poche)

Prérequis: `docs/book/poche/SOMMAIRE.md`.
Voir aussi: `docs/book/chapters/00-avant-propos.md`.

## Objectif

Comprendre avant-propos de manière simple pour agir rapidement.

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
pick ParseInt {
  case Ok(value: int)
  case Err(code: int)
}

proc non_negative(x: int) -> ParseInt {

  if x < 0 { give ParseInt.Err(22) }

  give ParseInt.Ok(x)
}

// Conversion finale vers un code de sortie
proc to_exit(r: ParseInt) -> int {

  match r {
    case Ok(_) { give 0 }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}

// Point d'entree du scenario
entry main at core/app {
  return to_exit(non_negative(-3))
}
```

## Erreurs fréquentes

1. Vouloir couvrir tous les cas d'un coup.
2. Mélanger plusieurs changements dans la même étape.
3. Oublier de relire le message d'erreur exact.

## Mini-exercice (5 minutes)

Modifiez l'exemple pour créer une variante de avant-propos, puis vérifiez le résultat attendu.

## Checklist rapide

1. Le code compile.
2. Le résultat est compréhensible.
3. Le cas limite principal est testé.

## Corrigé minimal

Partir du cas nominal, faire une seule modification, recompiler, puis valider la sortie.

## Aller plus loin

Version complète: `docs/book/chapters/00-avant-propos.md`.
