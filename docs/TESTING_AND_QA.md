# Tests & QA

## Tests
```bash
cargo test --workspace --all-features
```

- **Unit/integ** dans chaque crate.
- **Property tests** via `proptest` (sélectionnés par module).

## Snapshots
- `insta` pour golden tests (format YAML) dans crates qui le nécessitent.

## Linters
- `cargo fmt`, `cargo clippy -D warnings`

## Sécurité/Licences
- `cargo deny check advisories bans licenses sources`
