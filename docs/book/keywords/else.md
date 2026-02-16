# Mot-cle `else`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`else` exprime la branche alternative immediate d'un `if`.

Forme de base en Vitte. `if ... { ... } else { ... }`.

Exemple 1, construit pas a pas.

```vit
proc sign(x: int) -> int {
  if x < 0 { give -1 } else { give 1 }
}
```

Pourquoi cette etape est solide. Le flux est binaire et total pour cette regle simple.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc gate(ok: bool) -> int {
  if ok { give 200 } else { give 403 }
}
```

Pourquoi cette etape est solide. La branche `else` evite un retour implicite et fixe clairement le cas complementaire.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Empiler `else if` au lieu de modeliser un `pick` peut masquer des etats oubliEs.

Pour prolonger la logique. Voir `docs/book/keywords/otherwise.md` et `docs/book/logique/matching.md`.

Exemple 3, construit pas a pas.

```vit
proc flag(x: int) -> int {
  if x > 0 { give 1 } else { give 0 }
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle else dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer else sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de else au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
