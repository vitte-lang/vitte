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
- `build.sh` / `build.ps1` — build ciblé du workspace ou d'un crate
- `test.sh` / `test.ps1` — orchestrateur de tests (support nextest)
- `lint.sh` / `lint.ps1` — fmt/clippy/deny avec options fines
- `pipeline.sh` / `pipeline.ps1` — pipeline couleur lint → tests → docs → build (`--pipeline-target` pour cross-build)
- `Windows/package.ps1` — archive zip multi-cible (x64/ARM64) pour distribution Windows
- `syntax.sh` / `syntax.ps1` — contrôle syntaxe des sources Vitte (`vitte fmt --check`) + `cargo check` colorisé
- `BSD/package.sh` — génère un paquet pkg(8) (`pkg create`) pour FreeBSD/DragonFly
- `scripts/pro/coverage.sh` — rapport de couverture (cargo tarpaulin)
- `scripts/pro/doclint.sh` — documentation complète avec diagnostics colorés
- `scripts/pro/deps-audit.sh` — cargo deny + cargo outdated (audit dépendances)
- `scripts/pro/vitte-audit.sh` — audit du fichier `vitte.toml`
- `scripts/pro/ci-report.sh` — enchaîne pipeline, audit, doc et couverture (rapport global)
