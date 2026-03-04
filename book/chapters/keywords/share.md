# Mot-clé `share`

Niveau: Avancé.

## Lecture rapide (30s)

- Ce que c’est: ce mot-clé exprime une intention précise dans le flux Vitte.
- Quand l’utiliser: quand il rend la lecture du contrat plus directe.
- Erreur classique: l’utiliser au mauvais niveau (top-level vs bloc).

## Pourquoi (métier)

`share` réduit l’ambiguïté dans le code de production.

Vous l’utilisez pour rendre la règle métier explicite dès la lecture.
Cela simplifie les revues et accélère le diagnostic en cas d’erreur.
Le but est un comportement stable, lisible et testable.

## Définition

`share` exporte explicitement des symboles top-level (`all` ou liste d’identifiants).

## Syntaxe

Forme canonique: `share all` ou `share id1, id2`.

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

Diagnostic attendu:
- Code: `VITTE-XXXX` (ou code compilateur `E000X` correspondant).
- Position: `ligne 1, colonne 1` (ajustez selon le snippet réel).
- Message: motif stable orienté correction.

## Différences proches

| Mot-clé | Différence opérationnelle |
| --- | --- |
| `pull` | `share` définit ce que l’on exporte; `pull` définit ce que l’on importe. |

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

- `docs/book/chapters/07-controle.md`.
- `docs/book/chapters/27-grammaire.md`.
- `docs/book/chapters/31-erreurs-build.md`.

## Voir aussi

- `docs/book/keywords/erreurs-compilateur.md`.
- `docs/book/keywords/pull.md`.
- `docs/book/keywords/space.md`.
- `docs/book/chapters/09-modules.md`.

## Score de complétude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 3/5
