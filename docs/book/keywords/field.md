# Mot-cle `field`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`field` sert a declarer un champ dans la syntaxe legacy de structure.

Forme de base en Vitte. `field nom as type` dans un bloc legacy `form ... .end`.

Exemple 1, construit pas a pas.

```vit
form Point
  field x as int
  field y as int
.end
```

Pourquoi cette etape est solide. Chaque champ est declare explicitement avec son type, ce qui preserve la lisibilite sur anciens fichiers.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc norm1(p: Point) -> int {
  give p.x + p.y
}
```

Pourquoi cette etape est solide. Le reste du module consomme ensuite la structure comme n'importe quelle forme reguliere.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Melanger styles legacy et style brace sans convention d'equipe augmente la variance de code.

Pour prolonger la logique. Voir `src/vitte/grammar/vitte.ebnf` section `legacy_form`.

Exemple 3, construit pas a pas.

```vit
form Pair
  field left as int
  field right as int
.end
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle field dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer field sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de field au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
