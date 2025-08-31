# CI — Checks requis

- Format : `cargo fmt --all -- --check`
- Lints : `cargo clippy --workspace --all-features -- -D warnings`
- Sécurité : `cargo deny check advisories bans licenses sources`
- Tests : `cargo test --workspace --all-features`
- Docs : `cargo doc --workspace --all-features --no-deps` (optionnel)

Astuce : lance ces commandes **localement** avant d’ouvrir une PR.
