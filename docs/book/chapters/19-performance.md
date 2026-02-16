# 19. Performance

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Comparer des variantes Vitte equivalentes pour optimiser le cout sans changer le contrat fonctionnel.

Etape 1. Etablir une version de reference.

```vit
proc sum_loop(n: int) -> int {
  let i: int = 0
  let acc: int = 0
  loop {
    if i >= n { break }
    set acc = acc + i
    set i = i + 1
  }
  give acc
}
```

Pourquoi cette etape est solide. Cette baseline fixe la forme de controle et sert de repere pour les variantes.

Ce qui se passe a l'execution. `sum_loop(4)` accumule `0+1+2+3` et retourne `6`.

Etape 2. Variante avec branche de filtrage.

```vit
proc sum_even(n: int) -> int {
  let i: int = 0
  let acc: int = 0
  loop {
    if i >= n { break }
    if (i % 2) != 0 {
      set i = i + 1
      continue
    }
    set acc = acc + i
    set i = i + 1
  }
  give acc
}
```

Pourquoi cette etape est solide. Le filtrage impair ajoute une branche dans la boucle. La correction est intacte mais le profil d'execution change.

Ce qui se passe a l'execution. `sum_even(6)` traite `0,2,4` et retourne `6`.

Etape 3. Variante sans branche de parite.

```vit
proc sum_even_step(n: int) -> int {
  let i: int = 0
  let acc: int = 0
  loop {
    if i >= n { break }
    set acc = acc + i
    set i = i + 2
  }
  give acc
}
```

Pourquoi cette etape est solide. Le pas de `2` supprime une branche conditionnelle. Cette forme est souvent plus stable en cout.

Ce qui se passe a l'execution. `sum_even_step(6)` traite aussi `0,2,4` et retourne `6`.

Ce que vous devez maitriser en sortie de chapitre. Chaque optimisation preserve le resultat, change une variable de cout a la fois et reste mesurable.
