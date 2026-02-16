# Mot-cle `is`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`is` sert a tester une forme ou un motif dans le flux conditionnel.

Forme de base en Vitte. Usage courant: `when expr is Pattern`.

Exemple 1, construit pas a pas.

```vit
pick Token { case Num(v: int), case End }
proc parse(t: Token) -> int {
  when t is Num(v) { give v }
  give 0
}
```

Pourquoi cette etape est solide. Le test de forme se fait sans cast aveugle: la variante est verifiee avant extraction.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc is_end(t: Token) -> bool {
  when t is End { give true }
  give false
}
```

Pourquoi cette etape est solide. `is` fournit un predicat de structure lisible pour le controle.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Confondre `is` (test de forme) et `as` (conversion/annotation) cree des bugs subtils.

Pour prolonger la logique. Voir `docs/book/keywords/as.md`.

Exemple 3, construit pas a pas.

```vit
pick Token { case Num(v: int), case End }
proc is_num(t: Token) -> bool { when t is Num(_) { give true } give false }
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle is dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer is sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de is au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
