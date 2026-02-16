# 2. Philosophie du langage

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Traduire la philosophie Vitte en decisions de code verifiables.

Etape 1. Rendre l'erreur explicite.

```vit
proc safe_div(num: int, den: int) -> int {
  if den == 0 { give 0 }
  give num / den
}
```

Pourquoi cette etape est solide. La frontiere de faute est placee avant l'operation sensible.

Ce qui se passe a l'execution. `safe_div(10,2)=5`, `safe_div(10,0)=0`.

Etape 2. Modeliser les etats au lieu de coder des nombres magiques.

```vit
pick Auth {
  case Granted(user: int)
  case Denied(code: int)
}

proc can_access(a: Auth) -> bool {
  match a {
    case Granted(_) { give true }
    case Denied(_) { give false }
    otherwise { give false }
  }
}
```

Pourquoi cette etape est solide. Les cas sont portes par le type, pas par convention.

Ce qui se passe a l'execution. `Granted` donne `true`, `Denied` donne `false`.

Etape 3. Garder `entry` mince.

```vit
proc run() -> int { give 0 }

entry main at core/app {
  return run()
}
```

Pourquoi cette etape est solide. Orchestration separee du metier.

Ce qui se passe a l'execution. `run` est appelee, puis code retourne.

Ce que vous devez maitriser en sortie de chapitre. Erreurs explicites, etats modelises, orchestration separee.
