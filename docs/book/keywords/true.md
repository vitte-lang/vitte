# Mot-cle `true`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`true` est la valeur logique vraie de `bool`.

Forme de base en Vitte. Utilisee dans affectations et predicats.

Exemple 1, construit pas a pas.

```vit
proc always() -> bool {
  give true
}
```

Pourquoi cette etape est solide. Retour nominal constant utile pour tests ou stubs.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc check(x: int) -> bool {
  if x > 0 { give true }
  give false
}
```

Pourquoi cette etape est solide. Le predicat expose explicitement la decision binaire.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Comparer `bool` a `true` partout alourdit le code quand la condition directe suffit.

Pour prolonger la logique. Voir `docs/book/keywords/bool.md`.

Exemple 3, construit pas a pas.

```vit
proc always_ok() -> bool { give true }
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle true dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer true sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de true au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
