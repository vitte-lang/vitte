# Mot-cle `return`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`return` termine la procedure courante avec une valeur optionnelle selon la signature.

Forme de base en Vitte. `return expr` ou `return` selon contexte.

Exemple 1, construit pas a pas.

```vit
entry main at core/app {
  return 0
}
```

Pourquoi cette etape est solide. Sortie explicite du point d'entree avec code de statut.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc guard(x: int) -> int {
  if x < 0 { return -1 }
  return x
}
```

Pourquoi cette etape est solide. Retours precoces clairs pour cas d'erreur puis chemin nominal.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Des retours multiples sans structure de garde lisible compliquent l'audit.

Pour prolonger la logique. Voir `docs/book/chapters/10-diagnostics.md`.

Exemple 3, construit pas a pas.

```vit
proc guard(x: int) -> int {
  if x < 0 { return -1 }
  return x
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle return dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer return sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de return au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
