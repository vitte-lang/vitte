# Mot-cle `let`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`let` introduit une liaison locale explicite, utile pour nommer un etat intermediaire.

Forme de base en Vitte. `let nom: type = expr` ou `let nom = expr`.

Exemple 1, construit pas a pas.

```vit
proc inc(x: int) -> int {
  let y: int = x + 1
  give y
}
```

Pourquoi cette etape est solide. `let` fixe une valeur dans le scope courant; la lisibilite augmente quand chaque liaison porte une etape de raisonnement.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc sum2(a: int, b: int) -> int {
  let left: int = a
  let right: int = b
  let total: int = left + right
  give total
}
```

Pourquoi cette etape est solide. Trois liaisons explicites rendent la trace de calcul triviale pour la revue et le test.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Multiplier les `let` sans intention claire produit du bruit et dilue l'invariant utile.

Pour prolonger la logique. Voir `docs/book/logique/expressions.md`.

Exemple 3, construit pas a pas.

```vit
proc diff(a: int, b: int) -> int {
  let d: int = a - b
  give d
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle let dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer let sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de let au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
