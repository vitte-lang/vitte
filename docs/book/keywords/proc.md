# Mot-cle `proc`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`proc` definit une unite de comportement avec contrat d'entree et de sortie en Vitte.

Forme de base en Vitte. `proc nom(params) -> type { ... }`.

Exemple 1, construit pas a pas.

```vit
proc clamp(x: int, lo: int, hi: int) -> int {
  if x < lo { give lo }
  if x > hi { give hi }
  give x
}
```

Pourquoi cette etape est solide. La procedure est pure ici: aucune mutation externe, branchement bornant, resultat deterministe.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc checksum(seed: int, n: int) -> int {
  let i: int = 0
  let acc: int = seed
  loop {
    if i >= n { break }
    set acc = (acc * 31 + i) % 104729
    set i = i + 1
  }
  give acc
}
```

Pourquoi cette etape est solide. Le contrat repose sur une progression d'etat locale et une sortie clairement borneE par la recurrence.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Une `proc` qui lit, transforme, ecrit et journalise simultanement est un anti-pattern de couplage.

Pour prolonger la logique. Voir `docs/book/chapters/06-procedures.md`.

Exemple 3, construit pas a pas.

```vit
proc square(x: int) -> int { give x * x }
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle proc dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer proc sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de proc au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
