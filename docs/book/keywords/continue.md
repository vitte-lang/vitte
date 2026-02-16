# Mot-cle `continue`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`continue` saute le reste du bloc courant et passe a l'iteration suivante.

Forme de base en Vitte. Utilisable dans boucles de parcours et de calcul.

Exemple 1, construit pas a pas.

```vit
proc sum_pos(values: int[]) -> int {
  let s: int = 0
  for x in values {
    if x <= 0 { continue }
    set s = s + x
  }
  give s
}
```

Pourquoi cette etape est solide. Le filtre est localise dans la boucle et evite une imbrication supplementaire.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc checksum_skip(seed: int, n: int) -> int {
  let i: int = 0
  let acc: int = seed
  loop {
    if i >= n { break }
    if (i % 2) != 0 { set i = i + 1; continue }
    set acc = (acc * 31 + i) % 104729
    set i = i + 1
  }
  give acc
}
```

Pourquoi cette etape est solide. `continue` permet un saut propre des cas non pertinents en conservant la progression.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Utiliser `continue` sans progression d'index peut figer la boucle.

Pour prolonger la logique. Voir `docs/book/logique/boucles.md`.

Exemple 3, construit pas a pas.

```vit
proc sum_pos(values: int[]) -> int {
  let s: int = 0
  for x in values {
    if x <= 0 { continue }
    set s = s + x
  }
  give s
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle continue dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer continue sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de continue au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
