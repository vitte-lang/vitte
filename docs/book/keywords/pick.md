# Mot-cle `pick`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`pick` modele des variantes exclusives, ideal pour etats metier et resultats.

Forme de base en Vitte. `pick Nom { case A(...), case B(...) }`.

Exemple 1, construit pas a pas.

```vit
pick Result {
  case Ok(value: int)
  case Err(code: int)
}
```

Pourquoi cette etape est solide. Le type somme encode explicitement les chemins de succes et d'echec.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc code(r: Result) -> int {
  match r {
    case Ok(v) { give v }
    case Err(e) { give e }
    otherwise { give -1 }
  }
}
```

Pourquoi cette etape est solide. Le traitement impose une decomposition par variante et limite les conventions implicites.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Remplacer `pick` par des entiers magiques `0/1/2` degrade fortement la maintenabilite.

Pour prolonger la logique. Voir `docs/book/logique/matching.md`.

Exemple 3, construit pas a pas.

```vit
pick State { case Idle, case Busy }
proc idle(s: State) -> bool { match s { case Idle { give true } otherwise { give false } } }
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle pick dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer pick sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de pick au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
