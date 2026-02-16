# Mot-cle `pull`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`pull` rattache un module externe en reference directe.

Forme de base en Vitte. `pull chemin [as alias]`.

Exemple 1, construit pas a pas.

```vit
pull app/core

proc plus(x: int, y: int) -> int {
  give app/core.add(x, y)
}
```

Pourquoi cette etape est solide. Reutilisation directe d'un module sans copier la logique.

Ce qui se passe a l'execution. Si `app/core.add(10,32)=42`, alors `plus(10,32)=42`.

Exemple 2, construit pas a pas.

```vit
pull app/core as core

proc plus2(x: int, y: int) -> int {
  give core.add(x, y)
}
```

Pourquoi cette etape est solide. Alias de module pour reduire le bruit de chemin sur appels repetes.

Ce qui se passe a l'execution. `plus2(1,2)=3` via delegation `core.add`.

Exemple 3, construit pas a pas.

```vit
pull app/domain/ticket as t

proc urgent(id: int) -> bool {
  let tk = t.Ticket(id, 10)
  give t.is_critical(tk)
}
```

Pourquoi cette etape est solide. `pull` permet de composer types et procedures d'un module metier externe en gardant une frontiere explicite.

Ce qui se passe a l'execution. `urgent(5)=true` si `is_critical` est base sur seuil priorite >= 9.

Erreur frequente et correction Vitte. Melanger `pull` et `use` sans convention claire dans le meme module.

Correction recommandee en Vitte. Fixer une regle d'equipe: `pull` pour modules applicatifs, `use` pour bibliotheques et symboles cibles.

Pour prolonger la logique. Voir `docs/book/keywords/use.md`, `docs/book/chapters/03-projet.md`.
