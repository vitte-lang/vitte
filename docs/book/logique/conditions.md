# Conditions en Vitte

Cette page traite la logique Vitte comme un mecanisme executable: chaque regle est rattachee a une consequence concrete dans le flux de programme.
Une condition Vitte est un coupe-circuit semantique. Elle doit delimite un cas, pas simplement changer de chemin visuellement.

```vit
proc safe_div(num: int, den: int) -> int {
  if den == 0 { give 0 }
  give num / den
}
```

Pourquoi cette etape est solide. La condition protegee retire un etat invalide avant l'operation sensible. En Vitte, ce style est preferE a une gestion implicite tardive parce qu'il fixe la frontiere au point exact de risque.

Ce qui se passe a l'execution. `safe_div(12,3)` suit la branche nominale et retourne `4`. `safe_div(12,0)` prend la garde, saute la division et retourne `0`.

```vit
proc classify(temp: int) -> int {
  if temp < 0 { give -1 }
  if temp > 100 { give 2 }
  give 1
}
```

Pourquoi cette etape est solide. Ce pattern en gardes sequentielles limite l'imbrication et rend chaque classe d'entree directement testable. La premiere condition retire le sous-domaine bas, la seconde retire le sous-domaine haut, le reste est nominal.

Ce qui se passe a l'execution. `classify(-2)=-1`, `classify(50)=1`, `classify(120)=2`.

```vit
proc can_start(ready: bool, valid: bool, blocked: bool) -> int {
  if ready and valid and not blocked { give 1 }
  give 0
}
```

Pourquoi cette etape est solide. Condition composee en conjonction courte-circuitee. Le predicat s'evalue de gauche a droite et peut s'arreter des qu'un terme est faux.

Ce qui se passe a l'execution. `(true,true,false)->1`, `(true,false,false)->0`, `(true,true,true)->0`.

Cette page se rattache aux mecanismes de `if`, `and`, `or`, `not` et `otherwise`, details dans `docs/book/keywords/if.md`, `docs/book/keywords/and.md`, `docs/book/keywords/or.md`, `docs/book/keywords/not.md` et `docs/book/keywords/otherwise.md`.
