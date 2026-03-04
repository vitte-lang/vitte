# Mot-clé `is`

Niveau: Intermédiaire.

## Lecture rapide (30s)

- Ce que c’est: ce mot-clé exprime une intention précise dans le flux Vitte.
- Quand l’utiliser: quand il rend la lecture du contrat plus directe.
- Erreur classique: l’utiliser au mauvais niveau (top-level vs bloc).

## Pourquoi (métier)

`is` réduit l’ambiguïté dans le code de production.

Vous l’utilisez pour rendre la règle métier explicite dès la lecture.
Cela simplifie les revues et accélère le diagnostic en cas d’erreur.
Le but est un comportement stable, lisible et testable.

## Définition

`is` associe une expression à un `pattern` (cast/pattern test dans `cast_expr` ou `when ... is ...`).

## Syntaxe

Forme canonique: `expr is pattern`.

## Exemple nominal

Entrée:
- Pattern check sur variante.

```vit
pick Resp { case Ok, case Err }
proc is_ok(r: Resp) -> bool {
  give r is Ok
}
```

Sortie observable:
- Retourne `true` si `r` correspond au pattern `Ok`.

## Exemple invalide

Entrée:
- Opérande droit absent.

```vit
proc bad(r: int) -> bool {
  give r is
}
# invalide: pattern manquant.
```

Sortie observable:
- Le parseur rejette l’expression.

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Différences proches

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `as` | `is` teste un pattern; `as` exprime un cast de type. |

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

- Employer `is` pour remplacer `==` partout.
- Utiliser un pattern non déclaré.
- Oublier la couverture complète du flux après test.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour tester une forme de valeur via pattern.
- Quand l’éviter: pour un simple test d’égalité scalaire (`==`).

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near is` | Pattern absent/invalide. | Fournir un pattern valide après `is`. |
| `invalid pattern` | Pattern non conforme. | Utiliser identifiant qualifié ou pattern composé valide. |
| `type mismatch` | Expression et pattern incompatibles. | Vérifier les variantes attendues. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `as` | `is` teste un pattern; `as` exprime un cast de type. |

## Utilisé dans les chapitres

- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/as.md`.
- `docs/book/keywords/when.md`.
- `docs/book/chapters/27-grammaire.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
