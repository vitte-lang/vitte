# Mot-clé `use`

Niveau: Intermédiaire.

## Lecture rapide (30s)

- Ce que c’est: ce mot-clé exprime une intention précise dans le flux Vitte.
- Quand l’utiliser: quand il rend la lecture du contrat plus directe.
- Erreur classique: l’utiliser au mauvais niveau (top-level vs bloc).

## Pourquoi (métier)

`use` réduit l’ambiguïté dans le code de production.

Vous l’utilisez pour rendre la règle métier explicite dès la lecture.
Cela simplifie les revues et accélère le diagnostic en cas d’erreur.
Le but est un comportement stable, lisible et testable.

## Définition

`use` importe des symboles depuis un chemin (`glob`, groupe ou alias).

## Syntaxe

Forme canonique: `use path[.{...}|.*] [as alias]`.

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

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Différences proches

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `pull` | `use` importe des symboles; `pull` relie un module entier. |

## Refactor rapide

Avant:
```vit
# usage fragile à corriger
```

Après:
```vit
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

- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/pull.md`.
- `docs/book/keywords/space.md`.
- `docs/book/chapters/09-modules.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 4/5
