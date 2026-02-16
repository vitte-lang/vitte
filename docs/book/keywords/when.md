# Mot-cle `when`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`when` introduit chaque branche dans `select` et certaines formes de test contextuel.

Forme de base en Vitte. `when motif { ... }`.

Exemple 1, construit pas a pas.

```vit
proc map_state(s: int) -> int {
  select s
    when 0 { return 100 }
    when 1 { return 200 }
  otherwise { return -1 }
}
```

Pourquoi cette etape est solide. Chaque `when` doit representer un cas metier distinct et non un doublon de condition.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc action(job: int) -> int {
  select job
    when 10 { return 1 }
    when 20 { return 2 }
  otherwise { return 0 }
}
```

Pourquoi cette etape est solide. L'ordre des `when` influence la priorite de matching en lecture.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Empiler des dizaines de `when` peut signaler un type d'etat mal modelise.

Pour prolonger la logique. Voir `docs/book/keywords/pick.md` et `docs/book/keywords/match.md`.

Exemple 3, construit pas a pas.

```vit
proc level(v: int) -> int {
  select v
    when 1 { return 10 }
  otherwise { return 0 }
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle when dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer when sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de when au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
