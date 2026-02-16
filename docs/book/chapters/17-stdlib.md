# 17. Bibliotheque standard

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Utiliser la stdlib Vitte comme socle technique sans y noyer la logique metier.

Etape 1. Import minimal et procedure pure.

```vit
use std/core/types.int

proc inc(x: int) -> int {
  give x + 1
}
```

Pourquoi cette etape est solide. L'import est explicite et la transformation est deterministe. Le comportement est testable sans contexte externe.

Ce qui se passe a l'execution. `inc(41)=42`.

Etape 2. Recurrence deterministe sur bornes connues.

```vit
use std/core/types.int

proc checksum(seed: int, n: int) -> int {
  let i: int = 0
  let acc: int = seed
  loop {
    if i >= n { break }
    set acc = ((acc * 33) + i + 17) % 1000003
    set i = i + 1
  }
  give acc
}
```

Pourquoi cette etape est solide. Le calcul depend uniquement de `(seed,n)`. La borne `i>=n` rend le cout et la terminaison previsible.

Ce qui se passe a l'execution. `(seed=7,n=3)` produit `248`, puis `8202`, puis `270685`, et retourne `270685`.

Etape 3. Normalisation metier independante du socle.

```vit
form Reading {
  value: int
}

proc normalize_reading(r: Reading) -> int {
  if r.value < 0 { give 0 }
  if r.value > 100 { give 100 }
  give r.value
}
```

Pourquoi cette etape est solide. La stdlib reste un support. Les decisions de domaine demeurent dans des procedures metier dediees.

Ce qui se passe a l'execution. `Reading(-4)->0`, `Reading(50)->50`, `Reading(140)->100`.

Ce que vous devez maitriser en sortie de chapitre. Les imports sont precis, les algorithmes sont bornes et la logique de domaine reste isolee.
