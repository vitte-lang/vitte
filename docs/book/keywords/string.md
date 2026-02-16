# Mot-cle `string`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`string` represente une chaine de caracteres.

Forme de base en Vitte. Utilisable en champs, variables, parametres et retours.

Exemple 1, construit pas a pas.

```vit
form User {
  name: string
}
proc greet(u: User) -> string {
  give u.name
}
```

Pourquoi cette etape est solide. Le type textual est explicite et separE des types numeriques.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc non_empty(s: string) -> bool {
  if s ==  { give false }
  give true
}
```

Pourquoi cette etape est solide. Validation locale de contenu textuel dans un predicat dedie.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Melanger formats techniques et texte utilisateur sans convention de normalisation cree des bugs de frontiere.

Pour prolonger la logique. Voir `docs/book/chapters/08-structures.md`.

Exemple 3, construit pas a pas.

```vit
proc non_empty(s: string) -> bool {
  give s != ""
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle string dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer string sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de string au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
