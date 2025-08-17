# .cargo (Vitte workspace)

Dossier **.cargo** prêt à l’emploi pour optimiser builds, alias de commandes,
config Wasm, répertoires, et sources crates.io en mode *sparse*.

## Fichiers
- `config.toml` : profils, alias, net, runners Wasm, registry sparse.
- `config.local.example` : overrides locaux (à copier en `config.local.toml`).
- `.gitignore` : ignore `config.local.toml`.

⚠️ Ne mets **jamais** de tokens/credentials ici. Utilise `$CARGO_HOME/credentials` si besoin.
