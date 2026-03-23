# Mot-clé `share`

Niveau: Avancé.

## Lecture rapide

Repère: `share` sert à rendre le code plus explicite, pas à ajouter du bruit.
Utilisez-le quand il clarifie le contrat; évitez-le hors de son niveau grammatical.

## Pourquoi (métier)

En code reel, `share` sert a clarifier une decision et a reduire les conventions implicites.
Si ce mot cle ne clarifie ni le contrat, ni la branche, ni la sortie, il faut simplifier le snippet.

## Définition

`share` exporte explicitement des symboles top-level (`all` ou liste d’identifiants).

## Syntaxe

Forme canonique: `share all` ou `share id1, id2`.

## Exemple nominal

Entrée:
- Export explicite de symboles.

```vit
// Exemple concret: cas nominal puis cas invalide

space core/math

proc add(a: int, b: int) -> int { give a + b }

proc sub(a: int, b: int) -> int { give a - b }
share add, sub
```

Sortie observable:
- Seuls `add` et `sub` sont exportés.

## Exemple invalide

Entrée:
- Forme d’export hors grammaire.

```vit
// Exemple concret: cas nominal puis cas invalide
share proc add(a: int, b: int) -> int { give a + b }
# invalide: `share` n’exporte pas une déclaration inline.
```

Sortie observable:
- Le parseur rejette la forme.

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

- Exporter trop large avec `all` sans besoin.
- Exporter un symbole interne instable.
- Oublier d’aligner `share` avec la documentation module.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour contrôler l’API publique d’un module.
- Quand l’éviter: pour exporter implicitement des éléments non stabilisés.

## Erreurs compilateur fréquentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near share` | Forme autre que `all` ou `ident_list`. | Utiliser `share all` ou `share a, b`. |
| `unknown symbol in share list` | Symbole non déclaré dans le module. | Déclarer le symbole avant `share`. |
| `duplicate export` | Symbole répété. | Nettoyer la liste d’exports. |

## Mot-clé voisin

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `pull` | `share` définit ce que l’on exporte; `pull` définit ce que l’on importe. |

## Utilisé dans les chapitres

- `book/chapters/07-controle.md`.
- `book/chapters/27-grammaire.md`.
- `book/chapters/31-erreurs-build.md`.

## Voir aussi

- `book/keywords/erreurs-compilateur.md`.
- `book/keywords/pull.md`.
- `book/keywords/space.md`.
- `book/chapters/09-modules.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
