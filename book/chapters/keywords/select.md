# Mot-clé `select`

Niveau: Intermédiaire.

## Lecture rapide (30s)

- Ce que c’est: ce mot-clé exprime une intention précise dans le flux Vitte.
- Quand l’utiliser: quand il rend la lecture du contrat plus directe.
- Erreur classique: l’utiliser au mauvais niveau (top-level vs bloc).

## Pourquoi (métier)

`select` réduit l’ambiguïté dans le code de production.

Vous l’utilisez pour rendre la règle métier explicite dès la lecture.
Cela simplifie les revues et accélère le diagnostic en cas d’erreur.
Le but est un comportement stable, lisible et testable.

## Définition

`select` applique une suite de clauses `when` sur une expression, avec `otherwise` optionnel.

## Syntaxe

Forme canonique: `select expr {when_stmt} [otherwise block]`.

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

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Différences proches

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `match` | `select` chaîne des `when`; `match` impose des `case` dans des accolades. |

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

- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/when.md`.
- `docs/book/keywords/match.md`.
- `docs/book/chapters/27-grammaire.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
