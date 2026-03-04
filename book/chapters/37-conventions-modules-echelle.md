# 37. Conventions de modules a grande echelle

Niveau: Avance

Prerequis: `book/chapters/09-modules.md`.
Voir aussi: `book/chapters/32-catalogue-modules.md`.

## Objectif

Structurer une base de code importante avec `space`, `pull`, `use`, `share` sans couplage excessif.

## Conventions recommandees

1. `space` aligne sur le domaine (ex: `core/auth`, `core/http`).
2. `pull` reserve aux dependances internes claires.
3. `use` pour l'API consommee localement.
4. `share` limite aux symboles stables.

## Exemple minimal

```vit
space core/auth
pull core/crypto as crypto
use core/user.{UserRepo}
share login, logout
```

## Regles d'echelle

- Interdire les dependances circulaires entre espaces.
- Limiter la profondeur de chemin module.
- Stabiliser les points d'entree publics.

## Checklist

1. Le module a-t-il une responsabilite unique?
2. Les imports sont-ils minimaux?
3. Les symboles `share` sont-ils versionnes?


## Exemples progressifs (N1 -> N3)

### N1 (base): module minimal

```vit
space core/auth
proc login() -> int { give 0 }
share login
```

### N2 (intermediaire): imports explicites

```vit
space core/auth
pull core/crypto as crypto
use core/user.{UserRepo}
proc login() -> int { give 0 }
share login
```

### N3 (avance): surface publique stable

```vit
space core/auth
proc login() -> int { give 0 }
proc logout() -> int { give 0 }
share login, logout
```

### Anti-exemple

```vit
space core/auth
proc internal_only() -> int { give 0 }
share all
```

## Validation rapide

1. Verifier frontiere `share`.
2. Eviter cycles modules.
3. Limiter imports au necessaire.
