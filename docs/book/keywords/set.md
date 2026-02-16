# Mot-cle `set`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`set` declare une mutation explicite d'etat sur une liaison deja existante. En Vitte, cette instruction rend les effets de bord visibles dans le texte et facilite l'audit des transitions d'etat.

Forme de base en Vitte. `set nom = expr`.

Exemple 1, construit pas a pas.

```vit
proc counter(limit: int) -> int {
  let i: int = 0
  loop {
    if i >= limit { break }
    set i = i + 1
  }
  give i
}
```

Pourquoi cette etape est solide. `set` signale la mutation de `i` a chaque tour. La progression est monotone et la condition d'arret `i >= limit` garantit la terminaison du flux.

Ce qui se passe a l'execution. `counter(0)=0`, `counter(3)=3`, `counter(7)=7`.

Exemple 2, construit pas a pas.

```vit
proc acc(values: int[]) -> int {
  let s: int = 0
  for x in values {
    set s = s + x
  }
  give s
}
```

Pourquoi cette etape est solide. Le parcours `for` expose chaque element, et `set` maintient un accumulateur unique. Cette combinaison limite la complexite du raisonnement sur l'etat.

Ce qui se passe a l'execution. `acc([])=0`, `acc([1,2,3])=6`, `acc([10,-5,2])=7`.

Exemple 3, construit pas a pas.

```vit
proc clamp01(v0: int) -> int {
  let v: int = v0
  if v < 0 { set v = 0 }
  if v > 1 { set v = 1 }
  give v
}
```

Pourquoi cette etape est solide. `set` exprime ici une normalisation par paliers. Les gardes portent la politique de saturation, et la mutation reste strictement locale au scope de la procedure.

Ce qui se passe a l'execution. `clamp01(-3)=0`, `clamp01(0)=0`, `clamp01(2)=1`.

Erreur frequente et correction Vitte. Erreur frequente. Utiliser `set` sur plusieurs variables dans un bloc long sans ordre de mutation explicite, ce qui rend les invariants illisibles.

Correction recommandee en Vitte. Conserver une mutation principale par section logique, nommer les etats intermediaires avec `let`, puis verifier chaque branche de mutation avec un test nominal et un test limite.

Pour prolonger la logique. Voir `docs/book/logique/boucles.md`, `docs/book/chapters/06-procedures.md`, `docs/book/chapters/11-collections.md`.
