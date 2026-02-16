# 11. Collections et iteration

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Construire un parcours de collection stable en Vitte en separant accumulation, filtrage et post-traitement.

Etape 1. Reduction simple sur un tableau d'entiers.

```vit
proc sum(values: int[]) -> int {
  let acc: int = 0
  for x in values {
    set acc = acc + x
  }
  give acc
}
```

Pourquoi cette etape est solide. La collection est lue en flux lineaire. L'etat mutable est limite a `acc`, ce qui rend la preuve de correction directe.

Ce qui se passe a l'execution. Avec `[2,3,4]`, l'accumulateur evolue `0 -> 2 -> 5 -> 9`, puis la procedure retourne `9`.

Etape 2. Moyenne entiere avec garde sur collection vide.

```vit
proc mean_floor(values: int[]) -> int {
  let total: int = 0
  let count: int = 0
  for x in values {
    set total = total + x
    set count = count + 1
  }
  if count == 0 { give 0 }
  give total / count
}
```

Pourquoi cette etape est solide. Le contrat interdit la division invalide. Le cas vide est traite avant l'operation arithmetique, ce qui stabilise le diagnostic.

Ce qui se passe a l'execution. `[]` retourne `0`. `[4,5,6]` donne `total=15` et `count=3`, donc sortie `5`.

Etape 3. Filtrage positif et projection finale.

```vit
proc positive_only(values: int[]) -> int[] {
  let out: int[] = []
  for x in values {
    if x <= 0 { continue }
    out.push(x)
  }
  give out
}
```

Pourquoi cette etape est solide. Le `continue` maintient une boucle peu imbriquee. Les valeurs hors contrat sont eliminees au plus pres de la source.

Ce qui se passe a l'execution. `[-1,2,0,7]` construit successivement `[]`, puis `[2]`, puis `[2,7]`.

Ce que vous devez maitriser en sortie de chapitre. L'iteration est lineaire, les gardes de securite sont explicites et la mutation est strictement locale.
