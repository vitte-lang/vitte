# 8. Structures de donnees

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Placer la complexite dans la modelisation plutot que dans les conditions.

Etape 1. Structure ticket.

```vit
form Ticket {
  id: int
  priority: int
  assignee: string
}
```

Pourquoi cette etape est solide. Entite metier explicite a trois attributs.

Ce qui se passe a l'execution. Verification compile-time des champs.

Etape 2. Etat de cycle de vie.

```vit
pick TicketState {
  case Open
  case Assigned(user: string)
  case Closed(code: int)
}
```

Pourquoi cette etape est solide. Etats exclusifs et payload cible.

Ce qui se passe a l'execution. Toute valeur est exactement une variante.

Etape 3. Composition de regles.

```vit
proc is_critical(t: Ticket) -> bool {
  give t.priority >= 9
}

proc route(t: Ticket, s: TicketState) -> int {
  if is_critical(t) and not (match s { case Closed(_) { give true } otherwise { give false } }) {
    give 1
  }
  give 0
}
```

Pourquoi cette etape est solide. Regle metier composee a partir de structure + etat.

Ce qui se passe a l'execution. Priorite haute + non ferme -> `1`, sinon `0`.

Ce que vous devez maitriser en sortie de chapitre. Donnee et etat separes, regles courtes, predicates reutilisables.
