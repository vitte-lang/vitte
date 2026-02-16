# Mot-cle `select`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`select` structure une decision multi-branches lisible avec `when` et fallback.

Forme de base en Vitte. `select value when ... otherwise ...`.

Exemple 1, construit pas a pas.

```vit
proc status(code: int) -> int {
  select code
    when 1 { return 10 }
    when 2 { return 20 }
  otherwise { return -1 }
}
```

Pourquoi cette etape est solide. Le flux est lineaire et chaque branche est nommee par une valeur de comparaison.

Ce qui se passe a l'execution. `status(1)=10`, `status(9)=-1`.

Exemple 2, construit pas a pas.

```vit
proc route(mode: int) -> int {
  select mode
    when 0 { return 200 }
    when 9 { return 403 }
  otherwise { return 404 }
}
```

Pourquoi cette etape est solide. Le fallback capture tous les modes non pris en charge sans brancher sur une cascade `if`.

Ce qui se passe a l'execution. `route(0)=200`, `route(9)=403`, `route(7)=404`.

Exemple 3, construit pas a pas.

```vit
proc qos(level: int) -> int {
  select level
    when 1 { return 100 }
    when 2 { return 200 }
    when 3 { return 300 }
  otherwise { return 0 }
}
```

Pourquoi cette etape est solide. `select` devient un mapping compact pour tables de routage petites et stables.

Ce qui se passe a l'execution. `qos(2)=200`, `qos(4)=0`.

Erreur frequente et correction Vitte. Utiliser `select` sur des etats qui devraient etre modelises en `pick`.

Correction recommandee en Vitte. Pour etats metier fermes, preferer `pick + match`; garder `select` pour routage valeur/commande.

Pour prolonger la logique. Voir `docs/book/logique/matching.md`, `docs/book/keywords/when.md`, `docs/book/keywords/match.md`.
