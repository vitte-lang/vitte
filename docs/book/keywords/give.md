# Mot-cle `give`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`give` retourne une valeur depuis une procedure ou un bloc de calcul.

Forme de base en Vitte. `give expr`.

Exemple 1, construit pas a pas.

```vit
proc inc(x: int) -> int {
  give x + 1
}
```

Pourquoi cette etape est solide. Retour explicite et immediat de la valeur calculee.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc clamp01(v: int) -> int {
  if v < 0 { give 0 }
  if v > 1 { give 1 }
  give v
}
```

Pourquoi cette etape est solide. `give` dans les gardes permet des sorties precoces lisibles.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Melanger conventions `give` et retours implicites degrade la coherence de module.

Pour prolonger la logique. Voir `docs/book/chapters/06-procedures.md`.

Exemple 3, construit pas a pas.

```vit
proc abs(x: int) -> int {
  if x < 0 { give -x }
  give x
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle give dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer give sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de give au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
