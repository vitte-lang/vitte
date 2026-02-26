# Mot-clé `is`

Niveau: Intermédiaire.

## Définition

`is` associe une expression à un `pattern` (cast/pattern test dans `cast_expr` ou `when ... is ...`).

## Syntaxe

Forme canonique: `expr is pattern`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour tester une forme de valeur via pattern.
- Quand l’éviter: pour un simple test d’égalité scalaire (`==`).

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

## Pièges

- Employer `is` pour remplacer `==` partout.
- Utiliser un pattern non déclaré.
- Oublier la couverture complète du flux après test.

## Utilisé dans les chapitres

- Aucun chapitre principal ne l’emploie encore explicitement.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/as.md`.
- `docs/book/keywords/when.md`.
- `docs/book/chapters/27-grammaire.md`.
