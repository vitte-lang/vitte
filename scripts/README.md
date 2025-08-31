# scripts/ — utilitaires dev & CI pour Vitte

Ces scripts couvrent les besoins courants : **check** (fmt+clippy+tests+deny), **bench**, **docs**, **LSP**, **WASM**, **complétions**, **release tag**.
Chaque script existe en version **Bash** (`.sh`) et **PowerShell** (`.ps1`).

## Installation rapide
- Unix/macOS :
  ```bash
  chmod +x scripts/*.sh
  ```
- Windows PowerShell (si blocage) :
  ```powershell
  Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
  ```

## Sommaire
- `check.sh` / `check.ps1` — format, lints, tests, deny, docs
- `bench.sh` / `bench.ps1` — lance Criterion (`-p vitte-benchmarks`)
- `docs.sh` / `docs.ps1` — construit la doc (`cargo doc`)
- `lsp.sh` / `lsp.ps1` — lance le serveur LSP (stdio)
- `wasm-build.sh` / `wasm-build.ps1` — build cible `wasm32-unknown-unknown`
- `completions.sh` / `completions.ps1` — installe les complétions des binaires
- `release-tag.sh` / `release-tag.ps1` — crée/pousse un tag `vX.Y.Z`
