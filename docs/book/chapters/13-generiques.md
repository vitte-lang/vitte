# 13. Generiques

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Utiliser les generiques Vitte pour mutualiser une logique sans perdre la securite de type.

Etape 1. Fonction identite parametree par type.

```vit
proc id[T](x: T) -> T {
  give x
}
```

Pourquoi cette etape est solide. Le meme corps est specialise a la compilation selon `T`. Le contrat entree/sortie reste invariant.

Ce qui se passe a l'execution. `id[int](42)=42` et `id[string]("ok")="ok"`.

Etape 2. Homogeneite imposee sur plusieurs arguments.

```vit
proc first[T](a: T, b: T) -> T {
  give a
}
```

Pourquoi cette etape est solide. Le compilateur refuse les appels melanges. L'erreur apparait avant execution, au niveau du type-check.

Ce qui se passe a l'execution. `first[int](7,9)=7`. Un appel `first(7,"x")` est rejete.

Etape 3. Structure generique et transformation.

```vit
form Pair[T] {
  left: T
  right: T
}

proc swap_pair[T](p: Pair[T]) -> Pair[T] {
  give Pair[T](p.right, p.left)
}
```

Pourquoi cette etape est solide. Le type parametre est conserve apres transformation. La genericite couvre donnees et procedures.

Ce qui se passe a l'execution. `swap_pair(Pair[int](1,2))` retourne `Pair[int](2,1)`.

Ce que vous devez maitriser en sortie de chapitre. Le code est mutualise, les invariants de type restent garantis et les erreurs sont detectees en compilation.
