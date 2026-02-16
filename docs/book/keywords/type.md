# Mot-cle `type`

Ce mot-cle prend sa valeur dans les decisions techniques qu'il impose. L'objectif ici est de montrer son usage reel, puis d'en expliquer le mecanisme sans raccourci.
`type` definit un alias semantique. La representation reste identique mais l'intention devient explicite.

Forme de base en Vitte. `type Alias = Type`.

Exemple 1, construit pas a pas.

```vit
type Celsius = int
proc freeze(t: Celsius) -> bool {
  give t <= 0
}
```

Pourquoi cette etape est solide. L'alias reduit les confusions de domaine en signature, meme si le stockage machine reste `int`.

Ce qui se passe a l'execution. Verifier l'exemple 1 avec un cas nominal puis un cas limite, et confirmer la branche activee ainsi que la valeur produite.

Exemple 2, construit pas a pas.

```vit
type UserId = int
proc same(a: UserId, b: UserId) -> bool {
  give a == b
}
```

Pourquoi cette etape est solide. Le code devient auto-documente au niveau metier et plus robuste en revue technique.

Ce qui se passe a l'execution. Verifier l'exemple 2 avec trois entrees contrastees pour observer clairement le flux de controle et la sortie finale.

Point de vigilance. Creer des alias sans sens metier concret n'apporte rien et augmente la dette cognitive.

Pour prolonger la logique. Voir `docs/book/chapters/05-types.md`.

Exemple 3, construit pas a pas.

```vit
type UserId = int
proc same(a: UserId, b: UserId) -> bool { give a == b }
```

Pourquoi cette etape est solide. Cet exemple 3 montre une forme de production du mot-cle type dans un flux Vitte plus proche d'un module reel, avec un contrat lisible et une frontiere explicite.

Ce qui se passe a l'execution. Executer ce bloc avec un cas nominal et un cas limite permet de verifier la branche dominante, la valeur de sortie et l'absence de comportement implicite hors contrat.

Erreur frequente et correction Vitte. Erreur frequente. Employer type sans contrat local clair, puis compenser en aval avec des gardes ad hoc.

Correction recommandee en Vitte. Fixer la responsabilite de type au point d'usage, ajouter une verification explicite de frontiere, puis couvrir le cas nominal et le cas limite par test.
