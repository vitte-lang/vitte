# 10. Diagnostics et erreurs

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Convertir les fautes en sorties diagnostiques exploitables.

Etape 1. Garde de division.

```vit
proc safe_div(num: int, den: int) -> int {
  if den == 0 { give 0 }
  give num / den
}
```

Pourquoi cette etape est solide. Frontiere d'erreur au plus pres de l'operation risquee.

Ce qui se passe a l'execution. `(12,3)->4`, `(12,0)->0`.

Etape 2. Erreur typee.

```vit
pick ParsePort {
  case Ok(value: int)
  case Err(code: int)
}

proc parse_port(x: int) -> ParsePort {
  if x < 0 { give Err(400) }
  if x > 65535 { give Err(422) }
  give Ok(x)
}
```

Pourquoi cette etape est solide. Le diagnostic est encode dans la variante de retour.

Ce qui se passe a l'execution. `-1->Err(400)`, `8080->Ok(8080)`.

Etape 3. Projection technique.

```vit
proc to_exit(p: ParsePort) -> int {
  match p {
    case Ok(_) { give 0 }
    case Err(c) { give c }
    otherwise { give 70 }
  }
}
```

Pourquoi cette etape est solide. Separation metier/exit-code.

Ce qui se passe a l'execution. `Ok->0`, `Err(422)->422`.

Ce que vous devez maitriser en sortie de chapitre. Cause localisee, typee, projetee proprement.
