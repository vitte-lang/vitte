# Mot-cle `and`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`and` exprime une conjonction logique courte-circuitee.

Forme de base en Vitte. `cond1 and cond2`.

Exemple 1, construit pas a pas.

```vit
proc can_start(ready: bool, valid: bool) -> bool {
  give ready and valid
}

entry main at core/app {
  if can_start(true, true) { return 1 }
  return 0
}
```

Pourquoi cette etape est solide. `and` impose que les deux predicats soient vrais pour produire `true`. La semantique courte-circuitee autorise l'arret de l'evaluation des que le premier operande est faux.

Ce qui se passe a l'execution. `can_start(true,true)=true`, `can_start(true,false)=false`, `can_start(false,true)=false` sans evaluation utile du second predicat dans la troisieme situation.

Exemple 2, construit pas a pas.

```vit
proc in_range(x: int, lo: int, hi: int) -> bool {
  give x >= lo and x <= hi
}

proc route(x: int) -> int {
  if in_range(x, 0, 100) and x != 13 { give 1 }
  give 0
}
```

Pourquoi cette etape est solide. La premiere conjonction borne le domaine numerique. La seconde conjonction ajoute une exclusion metier. Le predicat compose reste lisible parce qu'il assemble des tests courts et nommes.

Ce qui se passe a l'execution. `route(50)=1`. `route(13)=0` a cause de l'exclusion. `route(120)=0` car la borne echoue.

Point de vigilance. Une chaine `a and b and c and d` devient vite opaque. Au-dela de deux ou trois termes, extraire des predicats nommes.

Pour prolonger la logique. Voir `docs/book/logique/conditions.md`, `docs/book/keywords/or.md`, `docs/book/keywords/not.md`.

Exemple 3, construit pas a pas.

```vit
proc both(a: bool, b: bool) -> bool {
  give a and b
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle and dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer and sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de and au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
