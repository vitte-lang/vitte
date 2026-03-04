# 61. Generation de diagrammes de grammaire

Niveau: Intermédiaire

Prérequis: `book/chapters/27-grammaire.md`.
Voir aussi: `book/grammar/railroad/README.md`.

## Objectif

Automatiser la generation des diagrammes railroad a partir de l'EBNF source.

## Workflow

1. Modifier la source EBNF unique.
2. Synchroniser artefacts de grammaire.
3. Regenerer diagrammes.
4. Verifier changements attendus.

## Commandes utiles

- `make grammar-sync`
- `make grammar-docs`
- `make grammar-gate`



## Exemples progressifs (N1 -> N3)

### N1 (base): règle unique

Snippet grammaire:

```vit
stmt ::= return_stmt | expr_stmt ;
```

Commande:

```bash
make grammar-docs
```

### N2 (intermédiaire): famille de règles

Snippet grammaire:

```vit
expr ::= assign_expr ;
assign_expr ::= or_expr [ "=" assign_expr ] ;
```

Commandes:

```bash
make grammar-sync
make grammar-docs
```

### N3 (avancé): régénération vérifiée

Snippet grammaire:

```vit
program ::= WS? { toplevel WS? } ;
```

Commandes:

```bash
make grammar-docs
make grammar-docs-check
```

### Anti-exemple

```text
Modifier manuellement les SVG/README générés.
```

## Validation rapide

1. Modifier la source unique.
2. Régénérer les artefacts.
3. Vérifier `--check` vert.
