# 28. Conventions de code

Ce chapitre avance comme un atelier de code Vitte: on pose une idee, on la fait vivre dans le code, puis on verifie precisement ce qui se passe a l'execution.
Ce chapitre poursuit un objectif simple: Uniformiser l'ecriture Vitte pour reduire le cout de relecture et accelerer la correction en equipe.

Etape 1. Nommer les procedures selon leur intention.

```vit
proc parse_port(x: int) -> int {
  if x < 0 { give -1 }
  if x > 65535 { give -1 }
  give x
}
```

Pourquoi cette etape est solide. Le verbe `parse` signale un contrat de validation. Les gardes sont placees au debut pour une lecture lineaire.

Ce qui se passe a l'execution. `parse_port(8080)=8080`, `parse_port(70000)=-1`.

Etape 2. Nommer les structures selon le domaine.

```vit
form User {
  id: int
  name: string
}
```

Pourquoi cette etape est solide. Les champs portent des noms metier, pas des details d'implementation. Le schema devient auto-documente.

Ce qui se passe a l'execution. Cette declaration n'execute rien, mais elle fixe un contrat de donnees clair au compile-time.

Etape 3. Nommer les predicats en booleen explicite.

```vit
proc is_admin(role: int) -> bool {
  give role == 9
}
```

Pourquoi cette etape est solide. Le prefixe `is_` rend la lecture des conditions immediate dans tout le code appelant.

Ce qui se passe a l'execution. `is_admin(9)=true` et `is_admin(1)=false`.

Ce que vous devez maitriser en sortie de chapitre. Les noms expriment l'intention, les gardes sont en tete et les predicats booleens sont reconnaissables instantanement.
