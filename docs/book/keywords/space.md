# Mot-cle `space`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`space` fixe le namespace/module courant du fichier Vitte.

Forme de base en Vitte. `space chemin/module`.

Exemple 1, construit pas a pas.

```vit
space app/domain
form Ticket { id: int, priority: int }
```

Pourquoi cette etape est solide. Le module est positionne explicitement dans l'arborescence logique.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
space app/service
pull app/domain as d
proc critical(t: d.Ticket) -> bool { give t.priority >= 9 }
```

Pourquoi cette etape est solide. Separation claire entre couches domaine et service.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Des chemins `space` incoherents avec le repo rendent la resolution fragile.

Pour prolonger la logique. Voir `docs/book/chapters/03-projet.md`.

Exemple 3, construit pas a pas.

```vit
space app/metrics
proc ping() -> int { give 1 }
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle space dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer space sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de space au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
