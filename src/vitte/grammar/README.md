# Vitte Grammar Sources

Ce dossier contient les grammaires de référence de Vitte.

## Fichiers

- `vitte.ebnf` : grammaire canonique.
- `vitte.pest` : grammaire opérationnelle alignée sur `vitte.ebnf`.
- `muf.ebnf` : grammaire MUF.

## Commandes

```bash
make grammar-sync
make grammar-check
make grammar-test
make core-language-test
```

## Politique

- Toute évolution de syntaxe passe d'abord par `vitte.ebnf`.
- `vitte.pest` doit rester alignée.
- Les artefacts du dossier `docs/book/grammar` sont générés via `make grammar-sync`.
