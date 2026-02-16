# Mot-cle `case`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`case` declare une variante dans `pick` et une branche dans `match`.

Forme de base en Vitte. `case Nom(... )` en declaration et en decomposition.

Exemple 1, construit pas a pas.

```vit
pick Job {
  case Ready
  case Failed(code: int)
}
```

Pourquoi cette etape est solide. La declaration fixe l'espace d'etats compile-time.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc status(j: Job) -> int {
  match j {
    case Ready { give 0 }
    case Failed(c) { give c }
    otherwise { give -1 }
  }
}
```

Pourquoi cette etape est solide. Chaque branche `case` capture sa variante et eventuel payload.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Oublier de traiter un `case` critique conduit a des fallbacks trompeurs.

Pour prolonger la logique. Voir `docs/book/keywords/match.md`.

Exemple 3, construit pas a pas.

```vit
pick Flag { case On, case Off }
proc to_int(f: Flag) -> int {
  match f { case On { give 1 } case Off { give 0 } otherwise { give -1 } }
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle case dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer case sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de case au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
