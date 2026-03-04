# Mot-cle `all`

Niveau: Debutant.

Prerequis: `book/chapters/keywords/share.md`, `book/chapters/09-modules.md`.
Voir aussi: `book/chapters/27-grammaire.md`, `book/chapters/31-erreurs-build.md`.

## Lecture rapide (30s)

- Ce que c'est: `all` sert a exporter tous les symboles via `share all`.
- Ou il apparait: au top-level, dans une declaration `share`.
- Erreur classique: essayer d'utiliser `all` comme expression (`give all`), ce qui est invalide.

## Definition operationnelle

`all` est un mot-cle reserve pour l'export de module dans la forme:

```vit
share all
```

Il ne remplace pas un identifiant et ne s'utilise pas dans les expressions.

## Syntaxe

Forme canonique:

```vit
share all
```

Rappel grammaire:

```ebnf
share_decl ::= "share" WS1 ( "all" | ident_list ) ;
```

## Exemple nominal (module public complet)

```vit
space core/math

proc add(a: int, b: int) -> int { give a + b }
proc sub(a: int, b: int) -> int { give a - b }
const version: int = 1

share all
```

Effet observable:
1. Le module expose tout son contrat public declare.
2. Le consommateur n'a pas a maintenir une liste d'exports manuelle.

## Exemple invalide (mauvais niveau)

```vit
proc bad() -> int {
  give all
}
```

Pourquoi c'est invalide:
1. `all` n'est pas une expression.
2. `all` est reserve au contexte `share`.

## Exemple invalide (forme share incomplete)

```vit
space core/math
share
```

Pourquoi c'est invalide:
1. `share` attend `all` ou une liste d'identifiants.
2. La declaration est syntaxiquement incomplete.

## Différences proches

| Mot-cle | Difference operationnelle |
| --- | --- |
| `share` | `share` porte la declaration; `all` est un argument possible de `share`. |
| `use` | `use` importe depuis un autre module; `share all` exporte depuis le module courant. |
| `pull` | `pull` recupere une dependance; `share all` definit la surface publique locale. |

## Quand utiliser `share all`

1. Module interne stable avec surface volontairement large.
2. Prototype rapide avant verrouillage fin des exports.
3. Phase d'exploration ou l'API change encore souvent.

## Quand eviter `share all`

1. Bibliotheque publique avec exigences de compatibilite stricte.
2. Module sensible ou l'on veut minimiser la surface exposee.
3. Codebase avec gouvernance d'API explicite par symbole.

## Refactor rapide (de `all` vers liste explicite)

Avant:

```vit
share all
```

Apres:

```vit
share add, sub, version
```

Benefice:
1. Surface API explicite.
2. Evolution plus controlee.
3. Revue de breaking changes facilitee.

## Erreurs frequentes

| Message type | Cause | Correction |
| --- | --- | --- |
| `unexpected token near all` | `all` utilise hors declaration `share`. | Replacer `all` dans `share all`. |
| `expected identifier or all after share` | `share` sans argument. | Ajouter `all` ou une liste `id1, id2`. |
| `expected top-level declaration` | `share all` place dans un bloc `proc`. | Remonter la declaration au top-level. |

## Checklist de revue

1. `share all` est-il vraiment justifie (vs liste explicite)?
2. Le module est-il top-level correct (`space`, declarations, `share`)?
3. L'impact compatibilite est-il documente si surface exposee large?

## Utilise dans les chapitres

1. `book/chapters/09-modules.md`
2. `book/chapters/27-grammaire.md`
3. `book/chapters/31-erreurs-build.md`

## Voir aussi

1. `book/chapters/keywords/share.md`
2. `book/chapters/keywords/use.md`
3. `book/chapters/keywords/pull.md`
4. `book/chapters/60-documentation-technique-durable.md`

## Score de completude

coverage: syntaxe/exemples/invalides/diagnostics/liens = 5/5
