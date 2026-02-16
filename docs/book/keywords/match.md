# Mot-cle `match`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`match` decompose une valeur selon ses variantes ou motifs.

Forme de base en Vitte. `match valeur { case ... otherwise ... }`.

Exemple 1, construit pas a pas.

```vit
pick Auth {
  case Granted(user: int)
  case Denied(code: int)
}

proc code(a: Auth) -> int {
  match a {
    case Granted(_) { give 200 }
    case Denied(c) { give c }
    otherwise { give 500 }
  }
}
```

Pourquoi cette etape est solide. Le type somme pilote le branchement. La lecture est orientee variantes et non codes implicites.

Ce qui se passe a l'execution. `code(Granted(1))=200`. `code(Denied(403))=403`.

Exemple 2, construit pas a pas.

```vit
proc is_denied(a: Auth) -> bool {
  match a {
    case Denied(_) { give true }
    otherwise { give false }
  }
}
```

Pourquoi cette etape est solide. Predicat derive de la structure de type. Aucun entier magique.

Ce qui se passe a l'execution. `is_denied(Granted(1))=false`. `is_denied(Denied(9))=true`.

Exemple 3, construit pas a pas.

```vit
pick Parse {
  case Num(value: int)
  case End
}

proc read_num(p: Parse) -> int {
  match p {
    case Num(v) { give v }
    case End { give 0 }
    otherwise { give -1 }
  }
}
```

Pourquoi cette etape est solide. `match` couvre les cas de flux de parsing en mode explicite, utile pour etats de lexer/parser.

Ce qui se passe a l'execution. `read_num(Num(12))=12`. `read_num(End)=0`.

Erreur frequente et correction Vitte. Remplacer un `match` par une cascade `if` sur codes numeriques.

Correction recommandee en Vitte. Conserver un `pick` metier et decomposer par `match` pour verrouiller les cas.

Pour prolonger la logique. Voir `docs/book/logique/matching.md`, `docs/book/keywords/pick.md`, `docs/book/keywords/case.md`.
