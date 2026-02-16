# Boucles en Vitte

Cette page traite la logique Vitte comme un mecanisme executable: chaque regle est rattachee a une consequence concrete dans le flux de programme.
Une boucle Vitte est correcte quand son etat, sa progression et sa sortie sont visibles dans le texte sans deduction externe.

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

Pourquoi cette etape est solide. Invariant principal: avant chaque test de sortie, `acc` contient la somme des entiers deja parcourus. La boucle se termine par condition explicite et mutation monotone de `i`.

Ce qui se passe a l'execution. `sum_to(3)` produit la sequence `acc=0,1,3,6` puis retourne `6`.

```vit
proc positive_sum(values: int[]) -> int {
  let acc: int = 0
  for x in values {
    if x < 0 { continue }
    set acc = acc + x
  }
  give acc
}
```

Pourquoi cette etape est solide. Le parcours `for` deleste la gestion d'index, `continue` retire les valeurs hors contrat local. La mutation d'etat reste unique.

Ce qui se passe a l'execution. `[5,-3,4]` conduit a `acc=0->5->5->9`, sortie `9`.

```vit
proc find_first_positive(values: int[]) -> int {
  for x in values {
    if x > 0 { give x }
  }
  give 0
}
```

Pourquoi cette etape est solide. Sortie anticipee `give` dans la boucle. Le premier match ferme le parcours; sinon la fonction retourne la valeur neutre de fin.

Ce qui se passe a l'execution. `[-2,-1,7,8]` retourne `7` au troisieme element. `[-2,-1]` retourne `0`.

Cette page prolonge `docs/book/chapters/07-controle.md` et renvoie aux mots-cles `loop`, `for`, `break`, `continue` dans `docs/book/keywords/loop.md`, `docs/book/keywords/for.md`, `docs/book/keywords/break.md`, `docs/book/keywords/continue.md`.
