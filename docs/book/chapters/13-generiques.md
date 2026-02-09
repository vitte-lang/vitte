# 13. Génériques

Les génériques servent à écrire une logique une fois, puis à la réutiliser sans perdre la sécurité des types. Ils sont utiles, mais doivent rester lisibles.

## Quand les utiliser

Quand une fonction est conceptuellement la même pour plusieurs types. Quand une structure a un rôle identique pour plusieurs formes de données.

## Quand les éviter

Quand le code devient opaque. Quand l’interface générique cache une logique trop spécifique.

## Lire un code générique

Le lecteur doit pouvoir comprendre l’intention sans apprendre un mini‑langage. Si un générique demande trop de contexte, c’est qu’il est trop ambitieux.

## Contrainte et clarté

Si votre générique nécessite des contraintes complexes, il est peut‑être trop large. Une bonne contrainte est une phrase courte : « T doit être comparable », « T doit être copiable ».

## Erreurs courantes

Exposer un générique quand une version concrète suffit. Trop généraliser trop tôt. Utiliser des noms de type confus.

## À retenir

Les génériques sont puissants, mais la clarté reste le premier objectif.


## Exemple guidé : un générique raisonnable

Implémentez une fonction `max` générique, puis une version concrète. Comparez le coût cognitif. Le but est d’apprendre quand généraliser.

## Checklist génériques

Le générique réduit vraiment la duplication. Les contraintes sont simples. Les noms de type sont lisibles.


## Exercice : généraliser ou pas

Écrivez deux versions d’une fonction de tri : une pour `int`, une générique. Comparez la lisibilité et la maintenance.


## Code complet (API actuelle)

Exemple : une fonction `max` générique simple.

```vit
proc max[T](a: T, b: T, cmp: proc(T, T) -> bool) -> T {
  if cmp(a, b) { give a }
  give b
}
```

## API idéale (future)

Une contrainte `T: Comparable` éviterait de passer un comparateur à chaque appel.

