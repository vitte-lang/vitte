# Mot-cle `bool`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`bool` est le type logique binaire en Vitte.

Forme de base en Vitte. Valeurs associees: `true` et `false`.

Exemple 1, construit pas a pas.

```vit
proc is_pos(x: int) -> bool {
  give x > 0
}
```

Pourquoi cette etape est solide. Le type retour explicite rend le contrat de predicate immediate.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc gate(ready: bool, valid: bool) -> bool {
  give ready and valid
}
```

Pourquoi cette etape est solide. Les predicats composes restent lisibles quand ils sont courts.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Coder des drapeaux logiques en `int` masque l'intention et fragilise les conditions.

Pour prolonger la logique. Voir `docs/book/keywords/true.md` et `docs/book/keywords/false.md`.

Exemple 3, construit pas a pas.

```vit
proc is_zero(x: int) -> bool {
  give x == 0
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle bool dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer bool sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de bool au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
