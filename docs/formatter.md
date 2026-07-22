# Vitte Formatter

Status: official formatter surface for Vitte 0.1.0.

The formatter is deterministic by edition. The default edition is `2026`; the compatibility edition is `2024`.

Release-gated behavior:

- stable output for the same source and edition
- comments preserved
- single-line imports normalized in deterministic order
- text snapshots generated under `target/formatter/`
- CI enforced through `make formatter-gate`

Use:

```sh
python3 tools/vitte_format.py --check --changed
python3 tools/vitte_format.py --edition 2026 src/vitte/tools/lsp/mod.vit
```
