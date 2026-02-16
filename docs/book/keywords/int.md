# Mot-cle `int`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`int` est le type entier standard en Vitte pour calculs discrets et codes.

Forme de base en Vitte. Declare via `x: int`.

Exemple 1, construit pas a pas.

```vit
proc add(a: int, b: int) -> int {
  give a + b
}
```

Pourquoi cette etape est solide. Operation arithmetique homogene et predictable.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc in_port_range(p: int) -> bool {
  give p >= 0 and p <= 65535
}
```

Pourquoi cette etape est solide. Exemple de borne numerique metier explicite.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Utiliser `int` pour des concepts differents sans alias semantique deteriore la lisibilite.

Pour prolonger la logique. Voir `docs/book/keywords/type.md`.

Exemple 3, construit pas a pas.

```vit
proc mul2(x: int) -> int { give x * 2 }
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle int dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer int sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de int au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
