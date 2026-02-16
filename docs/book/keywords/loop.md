# Mot-cle `loop`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`loop` cree une repetition explicite sans borne implicite.

Forme de base en Vitte. `loop { ... }` avec sortie via `break` ou `give/return`.

Exemple 1, construit pas a pas.

```vit
proc count(limit: int) -> int {
  let i: int = 0
  loop {
    if i >= limit { break }
    set i = i + 1
  }
  give i
}
```

Pourquoi cette etape est solide. La terminaison depend uniquement de la garde. L'invariant de progression doit etre lisible.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc find_pos(values: int[]) -> int {
  let i: int = 0
  loop {
    if i >= values.len() { give -1 }
    if values[i] > 0 { give i }
    set i = i + 1
  }
}
```

Pourquoi cette etape est solide. La boucle combine borne de tableau et sortie anticipee sur condition metier.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Oublier une mutation de progression dans `loop` cree des boucles infinies.

Pour prolonger la logique. Voir `docs/book/logique/boucles.md`.

Exemple 3, construit pas a pas.

```vit
proc spin(n: int) -> int {
  let i: int = 0
  loop { if i >= n { break } set i = i + 1 }
  give i
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle loop dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer loop sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de loop au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
