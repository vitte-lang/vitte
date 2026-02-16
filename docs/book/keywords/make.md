# Mot-cle `make`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`make` sert a construire une valeur en fixant explicitement une forme ou un type attendu.

Forme de base en Vitte. `make nom as Type = expr` ou `make nom = expr`.

Exemple 1, construit pas a pas.

```vit
form Point { x: int, y: int }
proc build() -> int {
  make p as Point = Point(3, 4)
  give p.x + p.y
}
```

Pourquoi cette etape est solide. `make` renforce la lecture de construction d'objet metier et stabilise l'intention de type au point de creation.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc mk_int() -> int {
  make n as int = 42
  give n
}
```

Pourquoi cette etape est solide. Le type explicite a la creation limite les infErences ambigues dans des modules denses.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Utiliser `make` partout alors qu'un `let` simple suffit surcharge la syntaxe sans gain semantique.

Pour prolonger la logique. Voir `docs/book/keywords/as.md` et `docs/book/chapters/05-types.md`.

Exemple 3, construit pas a pas.

```vit
proc mkv() -> int {
  make x as int = 7
  give x
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle make dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer make sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de make au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
