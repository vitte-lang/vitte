# Mot-cle `const`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`const` declare une valeur stable du module ou du bloc.

Forme de base en Vitte. `const NOM: type = expr`.

Exemple 1, construit pas a pas.

```vit
const MAX_RETRY: int = 3
proc ok(n: int) -> bool { give n < MAX_RETRY }
```

Pourquoi cette etape est solide. La constante elimine les valeurs magiques dispersees.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
const MIN_TEMP: int = -40
const MAX_TEMP: int = 125
proc in_range(t: int) -> bool {
  give t >= MIN_TEMP and t <= MAX_TEMP
}
```

Pourquoi cette etape est solide. Les bornes sont centralisees et reutilisables.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Constantes mal nommees ou sans unite recreent l'ambiguite des literals.

Pour prolonger la logique. Voir `docs/book/chapters/05-types.md`.

Exemple 3, construit pas a pas.

```vit
const MAX: int = 10
proc ok(x: int) -> bool { give x < MAX }
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle const dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer const sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de const au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
