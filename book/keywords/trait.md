# Mot-clé `trait`

Niveau: Avancé.

## Définition

`trait` suit la même structure grammaticale que `form` dans la grammaire de surface.

## Syntaxe

Forme canonique: `trait Name { champ: type }` ou forme legacy `.end`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour modéliser une forme déclarative partagée.
- Quand l’éviter: si vous attendez une sémantique de méthodes implicites non décrite par la grammaire.

## Exemple nominal

Entrée:
- Déclaration brace valide.

```vit
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
trait Pair {
  left: int,
  right
}
# invalide: type manquant pour `right`.
```

Sortie observable:
- Le parseur rejette la déclaration.

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

## Pièges

- Projeter une sémantique OO non portée par la grammaire.
- Mettre des procédures dans le corps.
- Oublier la cohérence brace/legacy.

## Utilisé dans les chapitres

- Aucun chapitre principal ne l’emploie encore explicitement.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/form.md`.
- `docs/book/keywords/field.md`.
- `docs/book/chapters/27-grammaire.md`.
