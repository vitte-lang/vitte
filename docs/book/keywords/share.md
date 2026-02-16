# Mot-cle `share`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`share` controle l'API exportee par le module courant.

Forme de base en Vitte. `share all` ou `share nom1, nom2`.

Exemple 1, construit pas a pas.

```vit
space app/math
share add

proc add(a: int, b: int) -> int { give a + b }
proc hidden(a: int) -> int { give a }
```

Pourquoi cette etape est solide. Export selectif: seule `add` est publique.

Ce qui se passe a l'execution. Un client peut appeler `app/math.add`, pas `hidden`.

Exemple 2, construit pas a pas.

```vit
space app/api
share all

proc ping() -> int { give 1 }
proc pong() -> int { give 2 }
```

Pourquoi cette etape est solide. Export complet utile pour facade stable.

Ce qui se passe a l'execution. Les deux symboles sont resolvables depuis un module client.

Exemple 3, construit pas a pas.

```vit
space app/http
share handle, validate

proc validate(code: int) -> bool { give code >= 100 and code <= 599 }
proc handle(code: int) -> int {
  if not validate(code) { give 400 }
  give code
}
proc internal_cache_key(code: int) -> int { give code * 31 }
```

Pourquoi cette etape est solide. On exporte l'API metier minimale et on garde les utilitaires internes hors surface publique.

Ce qui se passe a l'execution. Client externe voit `handle/validate`; `internal_cache_key` reste prive.

Erreur frequente et correction Vitte. `share all` par defaut sur modules internes instables.

Correction recommandee en Vitte. Passer en export selectif et traiter `share all` comme exception documentee.

Pour prolonger la logique. Voir `docs/book/keywords/all.md`, `docs/book/chapters/09-modules.md`.
