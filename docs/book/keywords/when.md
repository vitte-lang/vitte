# Mot-clé `when`

Niveau: Intermédiaire.

## Définition

`when` intervient dans `select` et dans la forme `when expr is pattern`.

## Syntaxe

Forme canonique: `when pattern block` ou `when expr is pattern block`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour exprimer une branche basée sur un pattern.
- Quand l’éviter: pour un test booléen direct simple (`if`).

## Exemple nominal

Entrée:
- Forme `when expr is pattern`.

```vit
pick Resp { case Ok, case Err }
proc on_resp(r: Resp) -> int {
  when r is Ok { give 0 }
  give 1
}
```

Sortie observable:
- La branche `when` s’active seulement si le pattern correspond.

## Exemple invalide

Entrée:
- Clause incomplète.

```vit
when r is { give 0 }
# invalide: pattern manquant après `is`.
```

Sortie observable:
- Le parseur rejette la clause.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near when` | Pattern manquant. | Fournir `when pattern { ... }` ou `when expr is pattern { ... }`. |
| `invalid pattern` | Pattern non conforme. | Utiliser un identifiant/pattern qualifié valide. |
| `when outside statement context` | Position invalide. | Utiliser `when` uniquement dans un contexte `stmt`. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `select` | `select` porte la séquence; `when` porte chaque branche. |

## Pièges

- Écrire `when` sans bloc.
- Mettre un littéral non valide comme pattern.
- Confondre `when` et `case`.

## Utilisé dans les chapitres

- Aucun chapitre principal ne l’emploie encore explicitement.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/select.md`.
- `docs/book/keywords/is.md`.
- `docs/book/chapters/27-grammaire.md`.
