# Mot-cle `emit`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`emit` sert a produire un signal/valeur dans des contextes de flux ou sortie intermediaire.

Forme de base en Vitte. `emit expr`.

Exemple 1, construit pas a pas.

```vit
proc stream_one(x: int) {
  emit x
}
```

Pourquoi cette etape est solide. L'instruction signale une emission explicite sans confusion avec la valeur de retour principale.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
proc stream_pos(values: int[]) {
  for x in values {
    if x <= 0 { continue }
    emit x
  }
}
```

Pourquoi cette etape est solide. La boucle emet uniquement les cas valides selon le predicat local.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Confondre `emit` avec `give` dans une meme API rend l'intention de sortie ambiguE.

Pour prolonger la logique. Voir `docs/book/keywords/give.md`.

Exemple 3, construit pas a pas.

```vit
proc stream(values: int[]) {
  for x in values { emit x }
}
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle emit dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer emit sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de emit au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
