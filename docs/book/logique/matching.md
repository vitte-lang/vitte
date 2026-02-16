# Matching et decomposition d'etat

Cette page traite la logique Vitte comme un mecanisme executable: chaque regle est rattachee a une consequence concrete dans le flux de programme.
Le matching Vitte remplace les conventions d'etat implicites par des variantes explicites et verifiees.

```vit
pick Job {
  case Ready
  case Running
  case Failed(code: int)
}

proc status(job: Job) -> int {
  match job {
    case Ready { give 0 }
    case Running { give 1 }
    case Failed(code) { give code }
    otherwise { give -1 }
  }
}
```

Pourquoi cette etape est solide. `pick` formalise l'espace des etats, `match` force la lecture par variante et payload. La branche `otherwise` joue un role de garde de robustesse.

Ce qui se passe a l'execution. `status(Ready)=0`, `status(Running)=1`, `status(Failed(503))=503`.

```vit
proc is_terminal(job: Job) -> bool {
  match job {
    case Failed(_) { give true }
    otherwise { give false }
  }
}
```

Pourquoi cette etape est solide. Predicat de terminalite derive d'une variante. L'intention est centralisee en une fonction courte et reusable.

Ce qui se passe a l'execution. `is_terminal(Failed(1))=true`, `is_terminal(Ready)=false`.

```vit
proc status2(job: Job) -> int {
  select job
    when Ready { return 0 }
    when Running { return 1 }
  otherwise { return -1 }
}
```

Pourquoi cette etape est solide. Variante de style avec `select/when`. Le flux est lisible en branches successives. La semantique reste une decomposition d'etat.

Ce qui se passe a l'execution. `status2(Ready)=0`, `status2(Running)=1`, `status2(Failed(9))=-1`.

Cette page se connecte aux mots-cles `pick`, `case`, `match`, `select`, `when`, `is` dans `docs/book/keywords/pick.md`, `docs/book/keywords/case.md`, `docs/book/keywords/match.md`, `docs/book/keywords/select.md`, `docs/book/keywords/when.md`, `docs/book/keywords/is.md`.
