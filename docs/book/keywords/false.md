# Mot-cle `false`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`false` est la valeur logique fausse de `bool`.

Forme de base en Vitte. Utilisee en gardes, etats initiaux et predicates.

Exemple 1, construit pas a pas.

```vit
proc never() -> bool {
  give false
}
```

Pourquoi cette etape est solide. Retour constant faux, utile pour tests de branchement.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc valid_port(p: int) -> bool {
  if p < 0 { give false }
  if p > 65535 { give false }
  give true
}
```

Pourquoi cette etape est solide. Le flux exprime explicitement les cas invalides avant nominal.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Encoder `false` via `0` entier au lieu de `bool` brouille les contrats de fonctions.

Pour prolonger la logique. Voir `docs/book/keywords/bool.md`.

Exemple 3, construit pas a pas.

```vit
proc never_ok() -> bool { give false }
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle false dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer false sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de false au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
