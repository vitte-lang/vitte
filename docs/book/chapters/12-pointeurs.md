# 12. Pointeurs, références, slices

Vitte vous laisse proche de la mémoire, mais sans vous obliger à écrire des acrobaties. La clé est de rendre les accès explicites et sûrs.

## Pointeurs

Les pointeurs donnent la puissance, mais ils demandent une discipline stricte. Utilisez‑les seulement quand vous savez pourquoi. La règle simple : si une référence suffit, restez avec une référence.

## Références

Les références rendent la lecture plus simple. Elles expriment une intention claire : « j’utilise cette valeur sans en prendre la propriété ».

## Slices et buffers

Un slice est un regard sur une portion d’un buffer. Il permet de manipuler une sous‑partie sans copier. C’est un outil simple, mais essentiel pour les traitements efficaces.

## Durée de vie

La plupart des bugs mémoire viennent d’une durée de vie mal comprise. Un pointeur qui “survit” à son buffer est un bug discret et coûteux.

## Erreurs courantes

Garder un pointeur vers un buffer temporaire. Confondre taille et capacité. Exposer un buffer mutable alors qu’une vue aurait suffi.

## À retenir

Un pointeur est un outil, pas une habitude. Un slice est un contrat de taille et de vue. La durée de vie est une information de premier ordre.


## Exemple guidé : durée de vie explicite

Créez un buffer dans une fonction, renvoyez un slice, puis montrez ce qui se passe si la durée de vie est mal gérée. Ajoutez ensuite une version correcte.

## Checklist mémoire

La durée de vie est claire. Les pointeurs ne survivent pas à leur source. Les vues sont préférées aux copies.


## Exercice : durée de vie

Créez une fonction qui retourne un pointeur vers une variable locale. Comprenez pourquoi c’est un bug, puis corrigez‑la. Cet exercice est brutal, mais formateur.


## Code complet (API actuelle)

Exemple : une fonction qui lit dans un buffer existant via un pointeur.

```vit
proc fill(buf: *[u8], value: u8) {
  let i: usize = 0
  loop {
    if i >= buf.len { break }
    buf[i] = value
    i = i + 1
  }
}
```

## API idéale (future)

Un type `Span[u8]` avec invariants de durée de vie rendrait ce genre de code plus sûr et plus expressif.

