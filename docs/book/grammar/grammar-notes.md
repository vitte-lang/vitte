# Notes de grammaire

## Ambiguïtés connues et résolution

- `if` expression vs `if` statement:
  - Resolution: statement is parsed in block statement context, expression in expression context.
- `when` clause vs `when ... is` match guard:
  - Resolution: `when <expr> is <pattern>` is preferred when `is` token is present.
- `use` simple path vs grouped path:
  - Resolution: parser reads `.{` as group start and `.*` as glob import.
- appel générique vs index puis appel :
  - La syntaxe de surface autorise `foo[T](...)`.
  - Résolution actuelle : le parser ne choisit l'appel générique que lorsque le contenu entre crochets est une liste de types sans ambiguïté ; sinon il reste sur l'indexation normale suivie d'un appel.
  - Exemple : `id[int](1)` est un appel générique, tandis que `id[i](1)` reste `index` puis `call`.

## Matrice top-level / statement / expression

| Construct | Top-level | Statement | Expression |
|---|---:|---:|---:|
| `proc` | yes | no | yes (`proc_expr`) |
| `entry` | yes | no | no |
| `if` | no | yes | yes |
| `match` | no | yes | no |
| `emit` | no | yes | no |
| call (`foo()`) | no | yes (`expr_stmt`) | yes |

## Guide de lecture rapide

1. Partir de `program` et `toplevel`.
2. Valider les frontières de `stmt` avant de déboguer la priorité des expressions.
3. Utiliser la table de priorité de `precedence.md` quand l'analyse d'un opérateur semble incorrecte.
4. Reproduire avec un fichier minimal dans `tests/grammar`.
5. Vérifier le contrat des diagnostics (`docs/book/grammar/diagnostics/expected`).
