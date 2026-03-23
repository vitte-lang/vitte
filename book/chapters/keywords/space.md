# Mot-clé `space`

Niveau: Intermédiaire.

## Lecture rapide

Repère concret: `space` sert à décider un chemin d'exécution de façon lisible et vérifiable.
Utilisez `space` quand il sert à organiser les modules et les symboles; évitez-le s'il n'apporte aucune différence observable sur la branche ou la sortie.

## Pourquoi (métier)

En pratique algorithmique, `space` sert à transformer une condition en branche exécutable, sans ambiguïté de lecture.
Règle pratique: si retirer `space` ne change ni le chemin exécuté ni la sortie, simplifiez le bloc.

## Définition

`space` déclare l’espace de module courant au niveau top-level.

## Syntaxe

Forme canonique: `space module/path`.

## Lecture algorithmique

Lecture conseillée: traquez où `space` intervient dans le flux, puis vérifiez son effet sur l'exécution réelle.

## Exemple nominal

Entrée:
- Déclaration top-level valide.

```vit
// Exemple concret: cas nominal puis cas invalide

space app/core

proc run() -> int {
  // Valeur retournee par cette branche
  give 0
}
```

Sortie observable:
- Le module courant est `app/core`; les déclarations suivantes y sont rattachées.

## Exemple invalide

Entrée:
- Cas hors grammaire (nom manquant).

```vit
// Exemple concret: cas nominal puis cas invalide
space

proc run() -> int {
  // Valeur retournee par cette branche
  give 0
}
# invalide: `space` exige un `module_path`.
```

Sortie observable:
- Le parseur rejette la déclaration.

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

- Écrire `space` comme un bloc.
- Changer de module au milieu d’un fichier sans le documenter.
- Mélanger séparateurs de chemin sans cohérence.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour fixer explicitement le module propriétaire des déclarations qui suivent.
- Quand l’éviter: dans un bloc (`proc`, `entry`), car `space` n’est pas une instruction.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near space` | Chemin module absent ou mal formé. | Fournir un `module_path` valide (`a/b` ou `a.b`). |
| `unexpected statement` | `space` placé dans un bloc. | Remonter la déclaration au niveau top-level. |
| `invalid module path` | Segments invalides. | Utiliser uniquement des identifiants valides. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `pull` | `space` fixe le module courant; `pull` importe un autre module. |

## Utilisé dans les chapitres

- `book/chapters/07-controle.md`.
- `book/chapters/27-grammaire.md`.
- `book/chapters/31-erreurs-build.md`.

## Voir aussi

- `book/keywords/erreurs-compilateur.md`.
- `book/keywords/pull.md`.
- `book/keywords/use.md`.
- `book/chapters/09-modules.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 4/5
