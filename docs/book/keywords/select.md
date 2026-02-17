# Mot-clé `select`

Niveau: Intermédiaire.

## Définition

`select` applique une suite de clauses `when` sur une expression, avec `otherwise` optionnel.

## Syntaxe

Forme canonique: `select expr {when_stmt} [otherwise block]`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour exprimer des choix multi-branches lisibles.
- Quand l’éviter: pour des cas binaires simples (`if/else`).

## Exemple nominal

Entrée:
- Sélection sur variantes de pattern.

```vit
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
select
when Ok { give 200 }
# invalide: `select` exige une expression.
```

Sortie observable:
- Le parseur rejette l’instruction.

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

## Pièges

- Utiliser des littéraux là où un `pattern` est attendu.
- Oublier `otherwise` pour un flux fermé.
- Mélanger `select` et `match` dans le même niveau sans raison.

## Utilisé dans les chapitres

- `docs/book/chapters/26-projet-editor.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/when.md`.
- `docs/book/keywords/match.md`.
- `docs/book/chapters/27-grammaire.md`.
