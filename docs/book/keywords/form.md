# Mot-cle `form`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`form` modele une structure de donnees stable avec champs nommes.

Forme de base en Vitte. `form Nom { champ: type, ... }`.

Exemple 1, construit pas a pas.

```vit
form Ticket {
  id: int
  priority: int
  assignee: string
}
proc is_critical(t: Ticket) -> bool { give t.priority >= 9 }
```

Pourquoi cette etape est solide. Le type structurel porte le domaine et supprime les tuples anonymes fragiles.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
form Reading { value: int }
proc normalize(r: Reading) -> int {
  if r.value < 0 { give 0 }
  if r.value > 100 { give 100 }
  give r.value
}
```

Pourquoi cette etape est solide. Les invariants metier se codent proprement sur des champs nommes et testables.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Des champs vagues comme `value1`, `value2` detruisent la valeur semantique de `form`.

Pour prolonger la logique. Voir `docs/book/chapters/08-structures.md`.

Exemple 3, construit pas a pas.

```vit
form Reading { value: int }
proc clip(r: Reading) -> int { if r.value < 0 { give 0 } give r.value }
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle form dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer form sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de form au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
