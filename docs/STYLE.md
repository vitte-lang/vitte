# Style

- **rustfmt** (voir `rustfmt.toml`) — largeur 100, imports groupés, etc.
- **Clippy** — règles dans `.clippy.toml` + `[workspace.lints]`.
- **Noms** : pas de `foo/bar/tmp`, voir `disallowed-names`.
- **Types** : préférez `parking_lot` aux locks std (voir `disallowed-types`).

> L’objectif : code **lisible**, **prévisible**, **sans unsafe**.
