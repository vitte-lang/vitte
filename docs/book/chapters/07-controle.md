# 7. Instructions de controle

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Maîtriser decisions, iterations et sorties sans ambiguité de flux.

Etape 1. Garde de classification.

```vit
proc classify(temp: int) -> int {
  if temp < 0 { give -1 }
  if temp > 100 { give 2 }
  give 1
}
```

Pourquoi cette etape est solide. Retours precoces pour cas hors plage.

Ce qui se passe a l'execution. `-2->-1`, `50->1`, `120->2`.

Etape 2. Boucle somme.

```vit
proc sum_to(limit: int) -> int {
  let i: int = 0
  let acc: int = 0
  loop {
    if i > limit { break }
    set acc = acc + i
    set i = i + 1
  }
  give acc
}
```

Pourquoi cette etape est solide. Invariant: `acc` est somme de `0..i-1`.

Ce qui se passe a l'execution. `sum_to(3)=6`.

Etape 3. Filtrage en parcours.

```vit
proc filtered_sum(values: int[]) -> int {
  let acc: int = 0
  for x in values {
    if x < 0 { continue }
    set acc = acc + x
  }
  give acc
}
```

Pourquoi cette etape est solide. Filtre local, accumulation monotone.

Ce qui se passe a l'execution. `[5,-2,7] -> 12`.

Ce que vous devez maitriser en sortie de chapitre. Sortie de boucle explicite, invariant verbalizable, effets localises.
