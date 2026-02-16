# Expressions et priorites

Cette page traite la logique Vitte comme un mecanisme executable: chaque regle est rattachee a une consequence concrete dans le flux de programme.
Une expression Vitte doit rester localement evaluable. Si la priorite d'operateurs rend la lecture incertaine, l'expression doit etre decomposee.

```vit
proc expr_a(a: int) -> int {
  let v: int = (a + 1) * 2
  give v
}
```

Pourquoi cette etape est solide. Les parentheses imposent l'ordre de calcul et eliminent l'ambiguite de precedence. Le resultat est ensuite nomme pour garder une trace locale du sous-calcul.

Ce qui se passe a l'execution. `expr_a(4)` calcule `(4+1)=5`, puis `5*2=10`, puis retourne `10`.

```vit
proc expr_b(ready: bool, blocked: bool) -> bool {
  let ok: bool = ready and not blocked
  give ok
}
```

Pourquoi cette etape est solide. `not` s'applique au predicat `blocked` avant la conjonction. L'evaluation est courte-circuitee: si `ready` est faux, la seconde partie peut etre ignoree.

Ce qui se passe a l'execution. `(true,false)->true`, `(true,true)->false`, `(false,false)->false`.

```vit
proc expr_c(value: int) -> int {
  make n as int = value as int
  give n
}
```

Pourquoi cette etape est solide. Conversion explicite `as` maintenue visible meme quand les types semblent compatibles. Ce choix est utile a la frontiere de modules ou les conversions doivent etre auditees.

Ce qui se passe a l'execution. `expr_c(9)` retourne `9`; la trace de conversion reste presente au niveau source.

La regle Vitte sur les expressions est stricte. Priorite implicite acceptable pour les cas triviaux, decomposition obligatoire des que la densite d'operateurs nuit a la verification.
