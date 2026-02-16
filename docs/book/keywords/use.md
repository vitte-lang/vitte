# Mot-cle `use`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`use` importe des symboles/module paths dans le scope courant.

Forme de base en Vitte. `use chemin` avec options de groupe ou alias.

Exemple 1, construit pas a pas.

```vit
use std/core/types.int

proc inc(x: int) -> int { give x + 1 }
```

Pourquoi cette etape est solide. Dependance visible en tete de fichier, resolution statique explicite.

Ce qui se passe a l'execution. `inc(1)=2`.

Exemple 2, construit pas a pas.

```vit
use std/core/types.int as i32

proc id(x: i32) -> i32 { give x }
```

Pourquoi cette etape est solide. Alias local pour simplifier la lecture sans changer le type reel.

Ce qui se passe a l'execution. `id(42)=42`.

Exemple 3, construit pas a pas.

```vit
use app/domain/user.{User, UserId}

proc same(a: UserId, b: UserId) -> bool {
  give a == b
}

proc mk(id: UserId, name: string) -> User {
  give User(id, name)
}
```

Pourquoi cette etape est solide. Import cible de symboles pour limiter la surface de dependance au strict necessaire.

Ce qui se passe a l'execution. `same(7,7)=true`; `mk(7,"ana")` construit un `User` valide.

Erreur frequente et correction Vitte. Importer large sans tri puis utiliser 5% des symboles.

Correction recommandee en Vitte. Preferer imports cibles et alias locaux explicites.

Pour prolonger la logique. Voir `docs/book/keywords/pull.md`, `docs/book/chapters/09-modules.md`.
