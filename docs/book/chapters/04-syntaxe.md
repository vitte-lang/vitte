# 4. Syntaxe essentielle

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Lire et ecrire une syntaxe Vitte qui laisse le flux observable.

Etape 1. Signature + sortie.

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Pourquoi cette etape est solide. Signature de type statique, sortie explicite.

Ce qui se passe a l'execution. `add(1,2)=3`.

Etape 2. Condition + garde.

```vit
proc clamp01(v: int) -> int {
  if v < 0 { give 0 }
  if v > 1 { give 1 }
  give v
}
```

Pourquoi cette etape est solide. Gardes sequentielles, chemin nominal final.

Ce qui se passe a l'execution. `-2->0`, `0->0`, `9->1`.

Etape 3. Boucle + mutation explicite.

```vit
proc count(n: int) -> int {
  let i: int = 0
  loop {
    if i >= n { break }
    set i = i + 1
  }
  give i
}
```

Pourquoi cette etape est solide. Etat unique, progression monotone, sortie visible.

Ce qui se passe a l'execution. `count(4)=4`.

Ce que vous devez maitriser en sortie de chapitre. Signatures nettes, gardes nettes, mutations nettes.
