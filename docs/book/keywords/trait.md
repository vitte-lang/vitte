# Mot-cle `trait`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`trait` exprime une capacite attendue, sans imposer une representation concrete.

Forme de base en Vitte. `trait Nom[T] { ... }`.

Exemple 1, construit pas a pas.

```vit
trait Printable[T] {
  display: string
}
```

Pourquoi cette etape est solide. Le trait encode une interface de comportement que d'autres types peuvent satisfaire.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
trait Checkable[T] {
  ok: bool
}
proc pass[T](x: T) -> T { give x }
```

Pourquoi cette etape est solide. La genericite et le trait se combinent pour structurer des contrats transverses.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Confondre trait (capacite) et form (stockage) cree des modeles hybrides difficiles a maintenir.

Pour prolonger la logique. Voir `docs/book/chapters/13-generiques.md`.

Exemple 3, construit pas a pas.

```vit
trait Eq[T] { equal: bool }
proc same_int(a: int, b: int) -> bool { give a == b }
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle trait dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer trait sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de trait au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
