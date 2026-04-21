# Mot-clé `when`

Niveau: Intermédiaire.

## Lecture rapide

Repère concret: `when` sert à décider un chemin d'exécution de façon lisible et vérifiable.
Utilisez `when` quand il sert à piloter une décision de contrôle; évitez-le s'il n'apporte aucune différence observable sur la branche ou la sortie.

## Pourquoi (métier)

En pratique algorithmique, `when` sert à transformer une condition en branche exécutable, sans ambiguïté de lecture.
Règle pratique: si retirer `when` ne change ni le chemin exécuté ni la sortie, simplifiez le bloc.

## Définition

`when` intervient dans `select` et dans la forme `when expr is pattern`.

## Syntaxe

Forme canonique: `when pattern block` ou `when expr is pattern block`.

## Lecture algorithmique

Lecture conseillée: traquez où `when` intervient dans le flux, puis vérifiez son effet sur l'exécution réelle.

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

- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/chapters/keywords/erreurs-compilateur.md`.
- `docs/book/chapters/keywords/select.md`.
- `docs/book/chapters/keywords/is.md`.
- `docs/book/chapters/27-grammaire.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
