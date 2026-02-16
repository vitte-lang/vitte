# 6. Procedures et contrats

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Ecrire des procedures testables par cas de contrat.

Etape 1. Contrat bornant.

```vit
proc clamp(x: int, lo: int, hi: int) -> int {
  if x < lo { give lo }
  if x > hi { give hi }
  give x
}
```

Pourquoi cette etape est solide. Contrat total sur les entrees sous hypothese de bornes coherentes.

Ce qui se passe a l'execution. `(-1,0,10)->0`, `(5,0,10)->5`, `(99,0,10)->10`.

Etape 2. Extraire la precondition.

```vit
proc validate_bounds(lo: int, hi: int) -> bool {
  give lo <= hi
}
```

Pourquoi cette etape est solide. Regle de bornes centralisee, reutilisable.

Ce qui se passe a l'execution. `(0,10)->true`, `(10,0)->false`.

Etape 3. Composer la procedure finale.

```vit
proc normalize(temp: int, lo: int, hi: int) -> int {
  if not validate_bounds(lo, hi) { give lo }
  if temp < lo { give lo }
  if temp > hi { give hi }
  give temp
}
```

Pourquoi cette etape est solide. Cas invalides traites avant nominal.

Ce qui se passe a l'execution. `(50,80,20)->80`, `(130,0,100)->100`, `(60,0,100)->60`.

Ce que vous devez maitriser en sortie de chapitre. Precondition explicite, branches testables, sortie stable.
