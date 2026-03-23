# Mot-clé `pull`

Niveau: Avancé.

## Lecture rapide

Repère concret: `pull` sert à décider un chemin d'exécution de façon lisible et vérifiable.
Utilisez `pull` quand il sert à organiser les modules et les symboles; évitez-le s'il n'apporte aucune différence observable sur la branche ou la sortie.

## Pourquoi (métier)

En pratique algorithmique, `pull` sert à transformer une condition en branche exécutable, sans ambiguïté de lecture.
Règle pratique: si retirer `pull` ne change ni le chemin exécuté ni la sortie, simplifiez le bloc.

## Définition

`pull` importe un module en top-level, avec alias optionnel.

## Syntaxe

Forme canonique: `pull module/path [as alias]`.

## Lecture algorithmique

Lecture conseillée: traquez où `pull` intervient dans le flux, puis vérifiez son effet sur l'exécution réelle.

## Exemple nominal

Entrée:
- Import top-level avec alias.

```vit
// Exemple concret: cas nominal puis cas invalide

space api/v1
pull core/math as math

proc sum(a: int, b: int) -> int {
  // Valeur retournee par cette branche
  give math.add(a, b)
}
```

Sortie observable:
- Le module `core/math` est résolu sous l’alias `math`.

## Exemple invalide

Entrée:
- Ordre syntaxique invalide.

```vit
// Exemple concret: cas nominal puis cas invalide
pull as core/math
# invalide: `as` vient après le chemin module.
```

Sortie observable:
- La déclaration est rejetée par le parseur.

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

- Importer avec alias trop génériques.
- Déclarer `pull` dans un bloc.
- Utiliser un chemin relatif non documenté.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour importer un module projet clairement nommé.
- Quand l’éviter: dans un bloc d’instructions ou avec un alias ambigu.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near pull` | Ordre `pull`/chemin/alias invalide. | Respecter `pull path [as alias]`. |
| `unknown module` | Chemin module introuvable. | Vérifier le `module_path` importé. |
| `duplicate import alias` | Alias déjà utilisé. | Renommer l’alias pour éviter collision. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `use` | `pull` importe un module; `use` sélectionne des symboles depuis un chemin. |

## Utilisé dans les chapitres

- `book/chapters/07-controle.md`.
- `book/chapters/27-grammaire.md`.
- `book/chapters/31-erreurs-build.md`.

## Voir aussi

- `book/keywords/erreurs-compilateur.md`.
- `book/keywords/use.md`.
- `book/keywords/space.md`.
- `book/chapters/09-modules.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 4/5
