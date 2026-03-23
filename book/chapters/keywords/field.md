# Mot-clé `field`

Niveau: Intermédiaire.

## Lecture rapide

Repère concret: `field` sert à décider un chemin d'exécution de façon lisible et vérifiable.
Utilisez `field` quand il sert à définir un modèle de données; évitez-le s'il n'apporte aucune différence observable sur la branche ou la sortie.

## Pourquoi (métier)

En pratique algorithmique, `field` sert à transformer une condition en branche exécutable, sans ambiguïté de lecture.
Règle pratique: si retirer `field` ne change ni le chemin exécuté ni la sortie, simplifiez le bloc.

## Définition

`field` est utilisé dans la forme legacy de `form`/`trait` avec la syntaxe `as`.

## Syntaxe

Forme canonique: `field nom as type` (dans `legacy_form`).

## Lecture algorithmique

Lecture conseillée: traquez où `field` intervient dans le flux, puis vérifiez son effet sur l'exécution réelle.

## Exemple nominal

Entrée:
- Déclaration legacy valide.

```vit
// Exemple concret: cas nominal puis cas invalide

form User
field id as int
field name as string
.end
```

Sortie observable:
- La structure est acceptée avec deux champs typés.

## Exemple invalide

Entrée:
- Forme mélangeant syntaxes.

```vit
// Exemple concret: cas nominal puis cas invalide

form User
field id: int
.end
# invalide: en mode legacy, `field` utilise `as`.
```

Sortie observable:
- Le parseur rejette la ligne de champ.

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

- Utiliser `field` avec `:`.
- Mélanger `.end` et `{ ... }`.
- Oublier la migration vers la forme moderne.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour maintenir du code legacy `.end`.
- Quand l’éviter: dans la forme brace moderne (`name: type`).

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near field` | Champ hors contexte legacy. | Utiliser `field ... as ...` dans `.end`. |
| `missing type after as` | Type absent. | Ajouter `as type_expr`. |
| `mixed form styles` | Mélange brace/legacy. | Choisir un style unique par déclaration. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `form` | `form` déclare le conteneur; `field` déclare un champ legacy. |

## Utilisé dans les chapitres

- `book/chapters/07-controle.md`.
- `book/chapters/27-grammaire.md`.
- `book/chapters/31-erreurs-build.md`.

## Voir aussi

- `book/keywords/erreurs-compilateur.md`.
- `book/keywords/form.md`.
- `book/keywords/trait.md`.
- `book/chapters/27-grammaire.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 4/5
