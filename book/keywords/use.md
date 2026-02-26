# Mot-clé `use`

Niveau: Intermédiaire.

## Définition

`use` importe des symboles depuis un chemin (`glob`, groupe ou alias).

## Syntaxe

Forme canonique: `use path[.{...}|.*] [as alias]`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour importer un sous-ensemble explicite de symboles.
- Quand l’éviter: dans un bloc ou avec un `glob` non maîtrisé.

## Exemple nominal

Entrée:
- Import top-level de groupe.

```vit
space app/core
use std.io.{read, write} as io
proc ping() -> int {
  give 0
}
```

Sortie observable:
- Les symboles importés sont visibles via l’alias `io`.

## Exemple invalide

Entrée:
- `use` placé dans une procédure.

```vit
proc bad() -> int {
  use std/bridge/io
  give 0
}
# invalide: `use` est top-level.
```

Sortie observable:
- Le compilateur rejette la position de la déclaration.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near use` | Groupe/glob mal formé. | Utiliser `.*` ou `.{a,b}` correctement. |
| `unexpected statement` | `use` hors top-level. | Déplacer l’import en tête de module. |
| `unknown import symbol` | Symbole absent du module source. | Corriger la liste importée. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `pull` | `use` importe des symboles; `pull` relie un module entier. |

## Pièges

- Importer `*` partout sans nécessité.
- Multiplier les alias opaques.
- Laisser des imports morts.

## Utilisé dans les chapitres

- `book/chapters/17-stdlib.md`.
- `book/chapters/30-faq.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/pull.md`.
- `docs/book/keywords/space.md`.
- `docs/book/chapters/09-modules.md`.
