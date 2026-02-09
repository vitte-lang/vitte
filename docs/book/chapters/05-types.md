# 5. Types et valeurs

Les types sont une forme de documentation exécutable. Dans Vitte, ils servent à exprimer vos contraintes, pas à vous piéger. Un bon type est une phrase courte : il dit ce que la valeur est, pas seulement ce qu’elle contient.

## Types numériques

On retrouve des types entiers classiques (`i32`, `u64`) et des flottants. Le but est d’être explicite sur la taille et le signe. Ce détail est vital dans le code système : un mauvais type est une corruption silencieuse.

## Inférence raisonnable

Vitte infère quand c’est sans ambiguïté. Quand le doute existe, vous devez préciser. C’est un compromis qui évite les surprises dans un code long‑vivant.

## Types composés

Vous manipulerez souvent des slices, des buffers, et des structures. Le langage fournit des briques simples pour exprimer ces formes. On verra les structures en détail au chapitre 8.

## Types comme vocabulaire

Nommez vos types comme vous nommez vos fonctions. Un type bien nommé réduit le besoin de commentaires et clarifie l’intention. On préfère souvent un petit type dédié à un alias générique qui ne raconte rien.

## Les erreurs de type sont des informations

Une erreur de type est un message, pas un affront. Elle vous dit que votre modèle mental ne correspond pas au contrat du code. Lisez‑la comme un dialogue.

## Choisir un type, c’est choisir un futur

Un type trop large laisse passer trop d’états invalides. Un type trop strict ralentit l’exploration. Cherchez l’équilibre : le type doit exprimer l’invariant le plus important.

## À retenir

Le type est un contrat. Quand vous doutez, écrivez le type explicitement. Les types racontent une histoire : écrivez‑les pour des lecteurs.


## Exemple guidé : types explicites

Écrivez une fonction qui lit une taille, puis qui alloue un buffer. Écrivez‑la d’abord avec types implicites, puis avec types explicites. Comparez la lisibilité.

## Erreurs courantes

Utiliser un type signé pour une taille. Confondre `i32` et `u32` dans une API publique. Laisser l’inférence masquer un choix important.

## Checklist types

Les tailles sont non signées. Les identifiants publics ont des types explicites. Les conversions sont visibles.


## Exercice : le bon type au bon endroit

Écrivez une fonction `read_bytes(count)`.

Version A : `count` est signé. Version B : `count` est non signé.

Comparez la clarté des erreurs possibles. Le bon type évite des états invalides.


## Code complet (API actuelle)

```vit
proc read_bytes(count: usize) -> [u8] {
  let out: [u8] = []
  let i: usize = 0
  loop {
    if i >= count { break }
    set out = out.push(0 as u8)
    i = i + 1
  }
  give out
}
```

## API idéale (future)

Un constructeur `bytes.alloc(count)` éviterait la boucle manuelle.

