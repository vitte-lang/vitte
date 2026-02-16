# 5. Types et valeurs

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Convertir des conventions informelles en contrats de type.

Etape 1. Type structurel.

```vit
form Point {
  x: int
  y: int
}

proc manhattan(p: Point) -> int {
  give p.x + p.y
}
```

Pourquoi cette etape est solide. La signature interdit les appels non structurels.

Ce qui se passe a l'execution. `Point(3,4)` donne `7`.

Etape 2. Type somme.

```vit
pick Result {
  case Ok(value: int)
  case Err(code: int)
}
```

Pourquoi cette etape est solide. Les etats valides sont fermes et nommes.

Ce qui se passe a l'execution. Toute valeur `Result` est `Ok` ou `Err`.

Etape 3. Traitement de variante.

```vit
proc unwrap_or_zero(r: Result) -> int {
  match r {
    case Ok(v) { give v }
    case Err(_) { give 0 }
    otherwise { give 0 }
  }
}
```

Pourquoi cette etape est solide. Les branches suivent la structure du type.

Ce qui se passe a l'execution. `Ok(42)->42`, `Err(500)->0`.

Ce que vous devez maitriser en sortie de chapitre. Types metier explicites, variantes explicites, projection explicite.
