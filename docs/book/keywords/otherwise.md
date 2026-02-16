# Mot-cle `otherwise`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`otherwise` joue le role de fallback explicite dans `if`, `match` et `select` selon le style Vitte.

Forme de base en Vitte. `otherwise { ... }`.

Exemple 1, construit pas a pas.

```vit
proc guard(x: int) -> int {
  if x > 0 { give x }
  otherwise { give 0 }
}
```

Pourquoi cette etape est solide. Le fallback est visible et local, ce qui limite les sorties implicites.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc status(j: int) -> int {
  select j
    when 1 { return 10 }
  otherwise { return -1 }
}
```

Pourquoi cette etape est solide. Dans les structures multi-branches, `otherwise` capte les cas non traites.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Utiliser `otherwise` pour masquer un cas metier important est un anti-pattern.

Pour prolonger la logique. Voir `docs/book/logique/matching.md`.

Exemple 3, construit pas a pas.

```vit
proc route(x: int) -> int {
  if x == 1 { give 10 }
  otherwise { give -1 }
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle otherwise dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer otherwise sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de otherwise au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
