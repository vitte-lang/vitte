# Mot-cle `macro`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`macro` factorise un motif syntaxique repetitif.

Forme de base en Vitte. `macro nom(params) { ... }`.

Exemple 1, construit pas a pas.

```vit
macro guard_nonzero(x) {
  if x == 0 { return -1 }
}

proc safe_div(a: int, b: int) -> int {
  guard_nonzero(b)
  give a / b
}
```

Pourquoi cette etape est solide. La macro injecte une garde de sortie dans le flux appelant et supprime une duplication classique.

Ce qui se passe a l'execution. `safe_div(10,2)=5`, `safe_div(10,0)=-1`.

Exemple 2, construit pas a pas.

```vit
macro clamp01(v) {
  if v < 0 { set v = 0 }
  if v > 1 { set v = 1 }
}

proc normalize01(x: int) -> int {
  let v: int = x
  clamp01(v)
  give v
}
```

Pourquoi cette etape est solide. Macro de mutation locale, acceptable car portee courte et predictable.

Ce qui se passe a l'execution. `normalize01(-3)=0`, `normalize01(2)=1`.

Exemple 3, construit pas a pas.

```vit
macro early_fail(code) {
  if code < 0 { return code }
}

proc pipeline_step(a: int, b: int) -> int {
  let c: int = a - b
  early_fail(c)
  give c * 2
}
```

Pourquoi cette etape est solide. Pattern d'echec rapide reutilisable dans les pipelines numeriques.

Ce qui se passe a l'execution. `pipeline_step(10,3)=14`, `pipeline_step(2,5)=-3`.

Erreur frequente et correction Vitte. Cacher de la logique metier lourde dans des macros imbriquees.

Correction recommandee en Vitte. Garder les macros pour motifs de garde/ceremonie; laisser l'algorithme dans des `proc` explicites.

Pour prolonger la logique. Voir `docs/book/chapters/14-macros.md`, `docs/book/keywords/proc.md`.
