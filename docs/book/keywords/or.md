# Mot-cle `or`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`or` exprime une disjonction logique.

Forme de base en Vitte. `cond1 or cond2`.

Exemple 1, construit pas a pas.

```vit
proc is_admin_or_owner(admin: bool, owner: bool) -> bool {
  give admin or owner
}
```

Pourquoi cette etape est solide. La condition est vraie des qu'un des predicats est vrai.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc status_ok(code: int) -> bool {
  give code == 200 or code == 204
}
```

Pourquoi cette etape est solide. Regroupe des alternatives nominales de sortie.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Melanger `and` et `or` sans clarifier la precedence peut introduire des erreurs de logique.

Pour prolonger la logique. Voir `docs/book/keywords/and.md`.

Exemple 3, construit pas a pas.

```vit
proc ok(code: int) -> bool {
  give code == 200 or code == 204
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle or dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer or sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de or au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
