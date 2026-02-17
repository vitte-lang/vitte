# Mot-clé `share`

Niveau: Avancé.

## Définition

`share` exporte explicitement des symboles top-level (`all` ou liste d’identifiants).

## Syntaxe

Forme canonique: `share all` ou `share id1, id2`.

## Quand l’utiliser / Quand l’éviter

- Quand l’utiliser: pour contrôler l’API publique d’un module.
- Quand l’éviter: pour exporter implicitement des éléments non stabilisés.

## Exemple nominal

Entrée:
- Export explicite de symboles.

```vit
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
share proc add(a: int, b: int) -> int { give a + b }
# invalide: `share` n’exporte pas une déclaration inline.
```

Sortie observable:
- Le parseur rejette la forme.

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

## Pièges

- Exporter trop large avec `all` sans besoin.
- Exporter un symbole interne instable.
- Oublier d’aligner `share` avec la documentation module.

## Utilisé dans les chapitres

- `docs/book/chapters/09-modules.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/pull.md`.
- `docs/book/keywords/space.md`.
- `docs/book/chapters/09-modules.md`.
