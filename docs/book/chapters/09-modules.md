# 9. Modules et organisation

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Construire une frontiere de module qui limite le couplage.

Etape 1. Module source.

```vit
space app/core

proc add(a: int, b: int) -> int {
  give a + b
}
```

Pourquoi cette etape est solide. Noyau fonctionnel local, sans dependance inutile.

Ce qui se passe a l'execution. `add(10,32)=42`.

Etape 2. Module consommateur avec alias.

```vit
space app/math
pull app/core as core
share add_pair

proc add_pair(x: int, y: int) -> int {
  give core.add(x, y)
}
```

Pourquoi cette etape est solide. Delegation explicite, surface publique limitee.

Ce qui se passe a l'execution. `add_pair(1,2)=3`.

Etape 3. Module domaine pur.

```vit
space app/domain

form Ticket { id: int, priority: int }
proc is_critical(t: Ticket) -> bool { give t.priority >= 9 }
```

Pourquoi cette etape est solide. Le domaine reste indépendant des couches d'orchestration.

Ce qui se passe a l'execution. `priority=9` retourne `true`.

Ce que vous devez maitriser en sortie de chapitre. Dependances orientees, exports limites, domaine pur.
