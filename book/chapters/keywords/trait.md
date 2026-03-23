# Mot-clé `trait`

Niveau: Avancé.

## Lecture rapide

Repère concret: `trait` sert à décider un chemin d'exécution de façon lisible et vérifiable.
Utilisez `trait` quand il sert à rendre le comportement du programme lisible; évitez-le s'il n'apporte aucune différence observable sur la branche ou la sortie.

## Pourquoi (métier)

En pratique algorithmique, `trait` sert à transformer une condition en branche exécutable, sans ambiguïté de lecture.
Règle pratique: si retirer `trait` ne change ni le chemin exécuté ni la sortie, simplifiez le bloc.

## Définition

`trait` suit la même structure grammaticale que `form` dans la grammaire de surface.

## Syntaxe

Forme canonique: `trait Name { champ: type }` ou forme legacy `.end`.

## Lecture algorithmique

Lecture conseillée: traquez où `trait` intervient dans le flux, puis vérifiez son effet sur l'exécution réelle.

## Exemple nominal

Entrée:
- Déclaration brace valide.

```vit
// Exemple concret: cas nominal puis cas invalide
trait Pair {
  left: int,
  right: int
}
```

Sortie observable:
- La déclaration est conforme à `form_decl` avec `field_list`.

## Exemple invalide

Entrée:
- Forme hors grammaire de surface.

```vit
// Exemple concret: cas nominal puis cas invalide
trait Pair {
  left: int,
  right
}
# invalide: type manquant pour `right`.
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

- Projeter une sémantique OO non portée par la grammaire.
- Mettre des procédures dans le corps.
- Oublier la cohérence brace/legacy.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour modéliser une forme déclarative partagée.
- Quand l’éviter: si vous attendez une sémantique de méthodes implicites non décrite par la grammaire.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near trait` | Corps non conforme (`proc`, instruction...). | Respecter `field_list` ou legacy `field ... as ...`. |
| `mixed declaration style` | Mélange brace/legacy. | Garder un seul style de déclaration. |
| `missing field type` | Champ sans type. | Ajouter `: type_expr` (brace) ou `as type_expr` (legacy). |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `form` | `trait` et `form` partagent la même forme syntaxique dans cette grammaire. |

## Utilisé dans les chapitres

- `book/chapters/07-controle.md`.
- `book/chapters/27-grammaire.md`.
- `book/chapters/31-erreurs-build.md`.

## Voir aussi

- `book/keywords/erreurs-compilateur.md`.
- `book/keywords/form.md`.
- `book/keywords/field.md`.
- `book/chapters/27-grammaire.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
