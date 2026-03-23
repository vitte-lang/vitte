# Mot-clé `select`

Niveau: Intermédiaire.

## Lecture rapide

Repère: `select` sert à rendre le code plus explicite, pas à ajouter du bruit.
Utilisez-le quand il clarifie le contrat; évitez-le hors de son niveau grammatical.

## Pourquoi (métier)

En code reel, `select` sert a clarifier une decision et a reduire les conventions implicites.
Si ce mot cle ne clarifie ni le contrat, ni la branche, ni la sortie, il faut simplifier le snippet.

## Définition

`select` applique une suite de clauses `when` sur une expression, avec `otherwise` optionnel.

## Syntaxe

Forme canonique: `select expr {when_stmt} [otherwise block]`.

## Exemple nominal

Entrée:
- Sélection sur variantes de pattern.

```vit
// Exemple concret: cas nominal puis cas invalide

pick Status { case Ok, case NotFound, case Fail }

proc to_code(status: Status) -> int {
  select status
  when Ok { give 200 }
  when NotFound { give 404 }
  otherwise { give 500 }
}
```

Sortie observable:
- La branche choisie dépend de la valeur `status`.

## Exemple invalide

Entrée:
- Expression de sélection absente.

```vit
// Exemple concret: cas nominal puis cas invalide
select
when Ok { give 200 }
# invalide: `select` exige une expression.
```

Sortie observable:
- Le parseur rejette l’instruction.

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

- Utiliser des littéraux là où un `pattern` est attendu.
- Oublier `otherwise` pour un flux fermé.
- Mélanger `select` et `match` dans le même niveau sans raison.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour exprimer des choix multi-branches lisibles.
- Quand l’éviter: pour des cas binaires simples (`if/else`).

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near select` | Expression absente. | Écrire `select expr`. |
| `invalid when clause` | Clause `when` mal formée. | Respecter `when pattern block`. |
| `incomplete selection` | Couverture métier incomplète. | Ajouter `otherwise` si nécessaire. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `match` | `select` chaîne des `when`; `match` impose des `case` dans des accolades. |

## Utilisé dans les chapitres

- `book/chapters/07-controle.md`.
- `book/chapters/27-grammaire.md`.
- `book/chapters/31-erreurs-build.md`.

## Voir aussi

- `book/keywords/erreurs-compilateur.md`.
- `book/keywords/when.md`.
- `book/keywords/match.md`.
- `book/chapters/27-grammaire.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
