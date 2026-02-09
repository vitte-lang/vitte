# 16. Backend C++ et interop

Vitte peut générer du C++ pour tirer parti d’outils existants. L’objectif n’est pas de « cacher » le C++, mais d’offrir une voie d’intégration claire.

## Interop simple

Vous pouvez appeler une fonction externe. Vous pouvez lier une bibliothèque existante.

L’important est de documenter l’ABI et la convention d’appel.

## Pensée “frontière”

L’interop est une frontière : vous passez d’un monde à l’autre. Assurez‑vous que chaque côté sait ce que l’autre attend.

## Stratégie d’intégration

Commencez par une fonction simple, documentée, et testée. Puis élargissez progressivement. L’interop réussie est une série d’étapes petites, jamais un “big bang”.

## Erreurs courantes

Oublier la convention d’appel. Mélanger des types qui ne partagent pas la même taille ou le même alignement. Ne pas tester l’interface côté C/C++ et côté Vitte.

## À retenir

Interopérer, c’est accepter deux mondes. Le contrat d’interface est ce qui évite les surprises.


## Exemple guidé : appeler une fonction C

Définissez une fonction `extern`, documentez son ABI, puis appelez‑la. Testez avec un petit programme C séparé.

## Checklist interop

ABI documentée. Types alignés. Tests croisés Vitte/C.


## Exercice : alignement

Définissez une structure côté C et côté Vitte. Vérifiez que la taille et l’alignement correspondent. C’est une erreur classique quand on débute en interop.


## Code complet (API actuelle)

```vit
#[extern]


proc c_add(x: i32, y: i32) -> i32

entry main at core/app {
  let v = c_add(1, 2)
  return v
}
```

## API idéale (future)

Un module `std/ffi` avec des helpers de conversion réduirait la friction.

