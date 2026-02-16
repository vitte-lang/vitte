# Mot-cle `all`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`all` est principalement utilise avec `share` pour publier l'ensemble des symboles d'un module.

Forme de base en Vitte. Forme standard: `share all`.

Exemple 1, construit pas a pas.

```vit
space app/api
share all

proc ping() -> int { give 1 }
proc pong() -> int { give 2 }
```

Pourquoi cette etape est solide. `share all` transforme le module en facade complete. Toute declaration exportable devient visible a l'exterieur sans liste explicite.

Ce qui se passe a l'execution. Un module client qui `pull app/api as api` peut appeler `api.ping()` et `api.pong()` sans restriction supplementaire.

Exemple 2, construit pas a pas.

```vit
space app/internal
share all

const VERSION: int = 1
proc helper() -> int { give VERSION }
proc calc(x: int) -> int { give x + VERSION }
```

Pourquoi cette etape est solide. Techniquement valide, mais strategiquement risquE. En interne, `share all` expose aussi les symboles qui auraient du rester prives, ce qui durcit toute evolution future.

Ce qui se passe a l'execution. Les appels externes peuvent desormais dependre de `helper()` et `calc()`. Un changement de signature devient une rupture publique potentielle.

Point de vigilance. `share all` convient aux modules facade stables. Pour les modules en evolution, preferer `share symbol1, symbol2`.

Pour prolonger la logique. Voir `docs/book/keywords/share.md`, `docs/book/chapters/09-modules.md`, `docs/book/chapters/29-style.md`.

Exemple 3, construit pas a pas.

```vit
space app/public
share all
proc health() -> int { give 1 }
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle all dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer all sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de all au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
