# Mot-clé `use`

Niveau: Intermédiaire.

## Lecture rapide

Repère: `use` sert à rendre le code plus explicite, pas à ajouter du bruit.
Utilisez-le quand il clarifie le contrat; évitez-le hors de son niveau grammatical.

## Pourquoi (métier)

En code reel, `use` sert a clarifier une decision et a reduire les conventions implicites.
Si ce mot cle ne clarifie ni le contrat, ni la branche, ni la sortie, il faut simplifier le snippet.

## Définition

`use` importe des symboles depuis un chemin (`glob`, groupe ou alias).

## Syntaxe

Forme canonique: `use path[.{...}|.*] [as alias]`.

## Exemple nominal

Entrée:
- Import top-level de groupe.

```vit
// Exemple concret: cas nominal puis cas invalide

space app/core
use std.io.{read, write} as io

proc ping() -> int {
  // Valeur retournee par cette branche
  give 0
}
```

Sortie observable:
- Les symboles importés sont visibles via l’alias `io`.

## Exemple invalide

Entrée:
- `use` placé dans une procédure.

```vit
// Exemple concret: cas nominal puis cas invalide

proc bad() -> int {
  use std/bridge/io
  // Valeur retournee par cette branche
  give 0
}
# invalide: `use` est top-level.
```

Sortie observable:
- Le compilateur rejette la position de la déclaration.

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Refactor rapide

Avant:
```vit
// Exemple concret: cas nominal puis cas invalide
# usage fragile à corriger
```

Après:
```vit
// Exemple concret: cas nominal puis cas invalide
# usage clair et testable
```

## Pièges

- Importer `*` partout sans nécessité.
- Multiplier les alias opaques.
- Laisser des imports morts.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour importer un sous-ensemble explicite de symboles.
- Quand l’éviter: dans un bloc ou avec un `glob` non maîtrisé.

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

## Utilisé dans les chapitres

- `book/chapters/07-controle.md`.
- `book/chapters/27-grammaire.md`.
- `book/chapters/31-erreurs-build.md`.

## Voir aussi

- `book/keywords/erreurs-compilateur.md`.
- `book/keywords/pull.md`.
- `book/keywords/space.md`.
- `book/chapters/09-modules.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 4/5
