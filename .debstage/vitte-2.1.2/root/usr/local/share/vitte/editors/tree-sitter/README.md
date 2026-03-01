# Tree-sitter VITTE

This directory hosts the real Tree-sitter grammar for VITTE.

## Files
- `grammar.js`: parser grammar
- `queries/highlights.scm`: highlight captures
- `test/corpus/*.txt`: parser corpus tests

## Local usage
From `editors/tree-sitter`:

```bash
tree-sitter generate
tree-sitter test
```

## CI behavior
`make tree-sitter-vitte-ci` runs:
1. static grammar/query validation (always)
2. parse smoke with `tree-sitter` CLI if available

If CLI is missing, CI in this repo currently enforces static validation and reports skip for parse smoke.
