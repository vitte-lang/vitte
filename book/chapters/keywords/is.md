# Mot-clé `is`

Niveau: Intermédiaire.

## Lecture rapide

Repère concret: `is` sert à décider un chemin d'exécution de façon lisible et vérifiable.
Utilisez `is` quand il sert à piloter une décision de contrôle; évitez-le s'il n'apporte aucune différence observable sur la branche ou la sortie.

## Pourquoi (métier)

En pratique algorithmique, `is` sert à transformer une condition en branche exécutable, sans ambiguïté de lecture.
Règle pratique: si retirer `is` ne change ni le chemin exécuté ni la sortie, simplifiez le bloc.

## Définition

`is` associe une expression à un `pattern` (cast/pattern test dans `cast_expr` ou `when ... is ...`).

## Syntaxe

Forme canonique: `expr is pattern`.

## Lecture algorithmique

Lecture conseillée: traquez où `is` intervient dans le flux, puis vérifiez son effet sur l'exécution réelle.

## Exemple nominal

Entrée:
- Pattern check sur variante.

```vit
// Exemple concret: cas nominal puis cas invalide

pick Resp { case Ok, case Err }

proc is_ok(r: Resp) -> bool {
  // Valeur retournee par cette branche
  give r is Ok
}
```

Sortie observable:
- Retourne `true` si `r` correspond au pattern `Ok`.

## Exemple invalide

Entrée:
- Opérande droit absent.

```vit
// Exemple concret: cas nominal puis cas invalide

proc bad(r: int) -> bool {
  // Valeur retournee par cette branche
  give r is
}
# invalide: pattern manquant.
```

Sortie observable:
- Le parseur rejette l’expression.

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

- Employer `is` pour remplacer `==` partout.
- Utiliser un pattern non déclaré.
- Oublier la couverture complète du flux après test.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour tester une forme de valeur via pattern.
- Quand l’éviter: pour un simple test d’égalité scalaire (`==`).

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near is` | Pattern absent/invalide. | Fournir un pattern valide après `is`. |
| `invalid pattern` | Pattern non conforme. | Utiliser identifiant qualifié ou pattern composé valide. |
| `type mismatch` | Expression et pattern incompatibles. | Vérifier les variantes attendues. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `as` | `is` teste un pattern; `as` exprime un cast de type. |

## Utilisé dans les chapitres

- `book/chapters/07-controle.md`.
- `book/chapters/27-grammaire.md`.
- `book/chapters/31-erreurs-build.md`.

## Voir aussi

- `book/keywords/erreurs-compilateur.md`.
- `book/keywords/as.md`.
- `book/keywords/when.md`.
- `book/chapters/27-grammaire.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
