# Diagnostic Vocabulary Audit

This audit records the canonical Vitte vocabulary used by diagnostics,
documentation, snapshots, and code actions.

## Canonical Terms

- Use `procedure` for the user-facing callable concept.
- Use `proc` only for the Vitte keyword.
- Use `give` for producing a value from a procedure.
- Use `form` for record-like declarations.
- Use `pick` for variant declarations.
- Use `binding` for a `let` name and its value.
- Use `symbol` for resolver-visible names.
- Use `span` for source locations.

## Rejected Synonyms

- Do not use `function` for the user-facing callable concept.
- Do not use `routine` for the user-facing callable concept.
- Do not use `return` when the diagnostic is describing Vitte syntax.
- Do not alternate between lexer, parser, resolver, sema, typeck, borrowck,
  MIR, backend, and linker phase names.

## Audit Scope

The audit covers:

- catalog documentation;
- terminal snapshots;
- JSON snapshots;
- LSP code-action text;
- diagnostic examples;
- compiler diagnostic style documentation.

Any new diagnostic must use the canonical term in its title, labels, notes,
helps, suggestions, and explanation text.
