# Mot-clé `when`

Niveau: Intermédiaire.

## Lecture rapide

Repère: `when` sert à rendre le code plus explicite, pas à ajouter du bruit.
Utilisez-le quand il clarifie le contrat; évitez-le hors de son niveau grammatical.

## Pourquoi (métier)

En code reel, `when` sert a clarifier une decision et a reduire les conventions implicites.
Si ce mot cle ne clarifie ni le contrat, ni la branche, ni la sortie, il faut simplifier le snippet.

## Définition

`when` intervient dans `select` et dans la forme `when expr is pattern`.

## Syntaxe

Forme canonique: `when pattern block` ou `when expr is pattern block`.

## Exemple nominal

Entrée:
- Forme `when expr is pattern`.

```vit
// Exemple concret: cas nominal puis cas invalide

pick Resp { case Ok, case Err }

proc on_resp(r: Resp) -> int {
  when r is Ok { give 0 }
  // Valeur retournee par cette branche
  give 1
}
```

Sortie observable:
- La branche `when` s’active seulement si le pattern correspond.

## Exemple invalide

Entrée:
- Clause incomplète.

```vit
// Exemple concret: cas nominal puis cas invalide
when r is { give 0 }
# invalide: pattern manquant après `is`.
```

Sortie observable:
- Le parseur rejette la clause.

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

- Écrire `when` sans bloc.
- Mettre un littéral non valide comme pattern.
- Confondre `when` et `case`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour exprimer une branche basée sur un pattern.
- Quand l’éviter: pour un test booléen direct simple (`if`).

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

## Utilisé dans les chapitres

- `book/chapters/07-controle.md`.
- `book/chapters/27-grammaire.md`.
- `book/chapters/31-erreurs-build.md`.

## Voir aussi

- `book/keywords/erreurs-compilateur.md`.
- `book/keywords/select.md`.
- `book/keywords/is.md`.
- `book/chapters/27-grammaire.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
