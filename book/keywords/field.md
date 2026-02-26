# Mot-clé `field`

Niveau: Intermédiaire.

## Définition

`field` est utilisé dans la forme legacy de `form`/`trait` avec la syntaxe `as`.

## Syntaxe

Forme canonique: `field nom as type` (dans `legacy_form`).

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour maintenir du code legacy `.end`.
- Quand l’éviter: dans la forme brace moderne (`name: type`).

## Exemple nominal

Entrée:
- Déclaration legacy valide.

```vit
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
form User
  field id: int
.end
# invalide: en mode legacy, `field` utilise `as`.
```

Sortie observable:
- Le parseur rejette la ligne de champ.

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

## Pièges

- Utiliser `field` avec `:`.
- Mélanger `.end` et `{ ... }`.
- Oublier la migration vers la forme moderne.

## Utilisé dans les chapitres

- `docs/book/chapters/09-modules.md`.
- `docs/book/chapters/10-diagnostics.md`.
- `docs/book/chapters/11-collections.md`.
- `docs/book/chapters/12-pointeurs.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/form.md`.
- `docs/book/keywords/trait.md`.
- `docs/book/chapters/27-grammaire.md`.
