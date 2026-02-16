# Mot-cle `not`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`not` inverse un predicat bool.

Forme de base en Vitte. `not condition`.

Exemple 1, construit pas a pas.

```vit
proc deny(blocked: bool) -> bool {
  give not blocked
}
```

Pourquoi cette etape est solide. Inversion directe d'un etat binaire.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc can_run(ready: bool, blocked: bool) -> bool {
  give ready and not blocked
}
```

Pourquoi cette etape est solide. Pattern de garde classique: nominal exige pret et non bloque.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Empiler plusieurs `not` dans une meme expression complique l'audit de logique.

Pour prolonger la logique. Voir `docs/book/logique/conditions.md`.

Exemple 3, construit pas a pas.

```vit
proc allow(blocked: bool) -> bool {
  give not blocked
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle not dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer not sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de not au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
