# Mot-clé `trait`

Niveau: Avancé.

## Lecture rapide (30s)

- Ce que c’est: ce mot-clé exprime une intention précise dans le flux Vitte.
- Quand l’utiliser: quand il rend la lecture du contrat plus directe.
- Erreur classique: l’utiliser au mauvais niveau (top-level vs bloc).

## Pourquoi (métier)

`trait` réduit l’ambiguïté dans le code de production.

Vous l’utilisez pour rendre la règle métier explicite dès la lecture.
Cela simplifie les revues et accélère le diagnostic en cas d’erreur.
Le but est un comportement stable, lisible et testable.

## Définition

`trait` suit la même structure grammaticale que `form` dans la grammaire de surface.

## Syntaxe

Forme canonique: `trait Name { champ: type }` ou forme legacy `.end`.

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

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Différences proches

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `form` | `trait` et `form` partagent la même forme syntaxique dans cette grammaire. |

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

- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/form.md`.
- `docs/book/keywords/field.md`.
- `docs/book/chapters/27-grammaire.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
