# Mot-cle `for`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`for` parcourt une source iterable et injecte chaque element dans une variable locale.

Forme de base en Vitte. `for x in source { ... }`.

Exemple 1, construit pas a pas.

```vit
proc sum(values: int[]) -> int {
  let acc: int = 0
  for x in values {
    set acc = acc + x
  }
  give acc
}
```

Pourquoi cette etape est solide. Le parcours est declaratif: pas d'index manuel, moins de risques de borne.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc positive(values: int[]) -> int {
  let acc: int = 0
  for x in values {
    if x <= 0 { continue }
    set acc = acc + x
  }
  give acc
}
```

Pourquoi cette etape est solide. Le filtre en ligne garde la lisibilite du flux de reduction.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Utiliser `for` avec trop d'effets secondaires externes detruit sa valeur de clarte.

Pour prolonger la logique. Voir `docs/book/keywords/in.md` et `docs/book/logique/boucles.md`.

Exemple 3, construit pas a pas.

```vit
proc count(values: int[]) -> int {
  let n: int = 0
  for _ in values { set n = n + 1 }
  give n
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle for dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer for sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de for au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
