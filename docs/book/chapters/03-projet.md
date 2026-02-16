# 3. Structure d'un projet

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Construire une architecture de modules avec couplage faible.

Etape 1. Domaine pur.

```vit
space app/domain

form Ticket {
  id: int
  priority: int
}
```

Pourquoi cette etape est solide. Le domaine ne depend ni d'I/O ni de transport.

Ce qui se passe a l'execution. Verification compile-time de la forme `Ticket`.

Etape 2. Service qui consomme le domaine.

```vit
space app/service
pull app/domain as d

proc is_critical(t: d.Ticket) -> bool {
  give t.priority >= 9
}
```

Pourquoi cette etape est solide. Dependance orientee domaine -> service, pas l'inverse.

Ce qui se passe a l'execution. `priority=10` donne `true`.

Etape 3. Entree qui orchestre.

```vit
space app/io
pull app/service as s

entry main at core/app {
  let code: int = s.is_critical(app/domain.Ticket(1, 9)) as int
  return code
}
```

Pourquoi cette etape est solide. `entry` assemble les couches sans les fusionner.

Ce qui se passe a l'execution. Chemin `io -> service -> domain`, sortie `1`.

Ce que vous devez maitriser en sortie de chapitre. Domaine pur, service compose, entree mince.
