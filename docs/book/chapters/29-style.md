# 29. Style d'architecture

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Organiser une application Vitte en couches pour que chaque changement reste localisable.

Etape 1. Ecrire un domaine pur sans dependance d'infrastructure.

```vit
space app/domain

form Order {
  amount: int
}

proc approve(o: Order) -> bool {
  give o.amount > 0
}
```

Pourquoi cette etape est solide. La couche domaine exprime uniquement les regles metier et reste testable sans IO.

Ce qui se passe a l'execution. `approve(Order(10))=true` et `approve(Order(0))=false`.

Etape 2. Traduire le metier dans une couche service.

```vit
space app/service
pull app/domain as d

proc handle(amount: int) -> int {
  let ok: bool = d.approve(d.Order(amount))
  if ok { give 200 }
  give 422
}
```

Pourquoi cette etape est solide. Le service orchestre la decision metier puis la convertit en code applicatif.

Ce qui se passe a l'execution. `handle(10)=200` et `handle(0)=422`.

Etape 3. Garder un point d'entree mince dans la couche IO.

```vit
space app/io
pull app/service as s

entry main at core/app {
  let code: int = s.handle(10)
  return code
}
```

Pourquoi cette etape est solide. L'entry ne contient pas de regle metier. Elle assemble simplement les couches.

Ce qui se passe a l'execution. Le scenario montre un retour final `200`.

Ce que vous devez maitriser en sortie de chapitre. Domaine pur, service de traduction et entry minimal forment une architecture stable a long terme.
