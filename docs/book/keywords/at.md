# Mot-cle `at`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`at` associe `entry` au chemin de module cible.

Forme de base en Vitte. `entry nom at chemin/module { ... }`.

Exemple 1, construit pas a pas.

```vit
entry main at core/app {
  return 0
}
```

Pourquoi cette etape est solide. Le chemin module est explicite au point d'entree.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
entry tool at app/cli {
  let code: int = 0
  return code
}
```

Pourquoi cette etape est solide. Chaque entree peut cibler un chemin different selon l'organisation du projet.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Oublier de mettre a jour `at` apres refactor de structure de modules casse la resolution.

Pour prolonger la logique. Voir `docs/book/chapters/01-demarrer.md`.

Exemple 3, construit pas a pas.

```vit
entry worker at app/worker {
  return 0
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle at dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer at sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de at au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
