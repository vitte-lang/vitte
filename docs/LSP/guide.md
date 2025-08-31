# LSP — Guide

Serveur basé sur **tower-lsp**. Transport par défaut : **stdio**.

## Capacités
- Diagnostics, complétion, hover, go-to definition, formatage.
- Parsing incrémental; cache fichiers via `notify` (optionnel).

## Lancer en local
```bash
cargo run -p vitte-lsp --features stdio
```

## Logs
- `RUST_LOG=vitte_lsp=debug` (active `tracing-subscriber` côté binaire).
