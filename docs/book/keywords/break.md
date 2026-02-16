# Mot-cle `break`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`break` stoppe immediatement la boucle courante.

Forme de base en Vitte. Utilisable dans `loop` et `for`.

Exemple 1, construit pas a pas.

```vit
proc until(limit: int) -> int {
  let i: int = 0
  loop {
    if i == limit { break }
    set i = i + 1
  }
  give i
}
```

Pourquoi cette etape est solide. `break` ferme la repetition quand la condition de sortie est atteinte.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc find(values: int[], needle: int) -> int {
  let i: int = 0
  let out: int = -1
  for x in values {
    if x == needle { set out = i; break }
    set i = i + 1
  }
  give out
}
```

Pourquoi cette etape est solide. La sortie anticipee economise des iterations inutiles et reste explicite.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Multiplier des `break` non documentes complique la preuve de terminaison.

Pour prolonger la logique. Voir `docs/book/keywords/loop.md`.

Exemple 3, construit pas a pas.

```vit
proc stop_at(limit: int) -> int {
  let i: int = 0
  loop {
    if i == limit { break }
    set i = i + 1
  }
  give i
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle break dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer break sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de break au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
