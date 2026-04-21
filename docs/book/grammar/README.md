# Référence de grammaire VITTE

Source de vérité : `src/vitte/grammar/vitte.ebnf`.

Les artefacts générés ne doivent pas être modifiés à la main :
- `docs/book/grammar/grammar-surface.ebnf`
- `docs/book/grammar/vitte.ebnf`
- `docs/book/grammar-surface.ebnf`

## Commandes standard

```bash
make grammar-sync
make grammar-check
make grammar-test
make core-language-test
make core-semantic-success
make grammar-docs
make grammar-gate
make core-language-gate
bin/vitte grammar check
```

## Maturité

La couverture grammaticale et la surface d'implémentation sont plus larges que le cœur de langage stabilisé.

- `stable` : comportement protégé par la politique du noyau du langage et le core gate.
- `experimental` : implémenté ou documenté, mais pas encore protégé comme partie du cœur du langage.
- `internal` : détail d'implémentation, pas un contrat de langage public.

References:

- `docs/LANGUAGE_CORE.md`
- `docs/LANGUAGE_CORE_CHECKLIST.md`
- `docs/LANGUAGE_CORE_COMPATIBILITY.md`
- `docs/LANGUAGE_CORE_GUARANTEES.md`
- `docs/LANGUAGE_CORE_TEST_PLAN.md`
- `docs/API_LIFECYCLE.md`

Gate sémantique ciblé :

- `make core-semantic-success`
- `make core-semantic-snapshots`
- `tests/core_semantic_success_manifest.txt`
- `tests/diag_snapshots/core_semantic_manifest.txt`

## Politique de changement

- Changement de grammaire cassant : modifie la syntaxe acceptée ou la forme de l'arbre d'analyse.
- Changement de grammaire non cassant : refactorisation/factorisation avec la même surface acceptée.
- Changement limité au diagnostic : grammaire identique, texte/code de l'erreur de parse modifiés.

Consigner chaque changement de grammaire dans `docs/book/grammar/grammar-changelog.md`.

## Checklist PR (grammaire)

- Règle modifiée uniquement dans `src/vitte/grammar/vitte.ebnf`.
- `make grammar-sync` exécutée.
- `make core-language-test` est verte si le changement touche le cœur du langage.
- Corpus valide/invalide mis à jour dans `tests/grammar`.
- Diagnostics attendus mis à jour dans `docs/book/grammar/diagnostics/expected`.
- Instantanés rafraîchis si nécessaire.
- `make grammar-gate` est vert.
