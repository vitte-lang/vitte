# Lexer Parser Diagnostics Coverage 100

This directory anchors the tracked frontend coverage contract used by:

```sh
make lexer-parser-coverage-100
```

The gate expands the contract to generated artifacts in `target/frontend/lexer_parser_100/`:

- `manifest.json`
- `valid_corpus.vit`
- `invalid_corpus.vit`
- `snapshots/text.snapshot`
- `snapshots/json.snapshot`
- `snapshots/lsp.snapshot`

Every covered item must keep evidence, a stable diagnostic code, an explanation, a suggestion, one valid example, one invalid example, and deterministic snapshots.
