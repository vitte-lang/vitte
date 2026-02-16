# Mot-cle `if`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`if` ouvre une branche conditionnelle basee sur un predicat bool.

Forme de base en Vitte. `if condition { ... }` avec branch alternatives possibles.

Exemple 1, construit pas a pas.

```vit
proc classify(x: int) -> int {
  if x < 0 { give -1 }
  give 1
}
```

Pourquoi cette etape est solide. Le test est evalue, puis la branche est choisie sans ambiguite.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc clamp01(v: int) -> int {
  if v < 0 { give 0 }
  if v > 1 { give 1 }
  give v
}
```

Pourquoi cette etape est solide. Deux gardes sequentielles forment un flux de validation lisible et testable.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Imbrication profonde de `if` sans extraction de predicats degrade la comprehension.

Pour prolonger la logique. Voir `docs/book/logique/conditions.md`.

Exemple 3, construit pas a pas.

```vit
proc non_zero(x: int) -> int {
  if x == 0 { give -1 }
  give x
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle if dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer if sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de if au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
