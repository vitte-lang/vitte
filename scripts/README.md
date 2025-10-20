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
- `bootstrap.sh` / `bootstrap.ps1` — vérifie/installe toolchain & dépendances (voir `make bootstrap`)
- `check.sh` / `check.ps1` — format, lints, tests, deny, docs
- `doctor.sh` — diagnostic rapide de l’environnement (toolchains, outils optionnels)
- `bench.sh` / `bench.ps1` — lance Criterion (`-p vitte-benchmarks`)
- `docs.sh` / `docs.ps1` — construit la doc (`cargo doc`)
- `lsp.sh` / `lsp.ps1` — lance le serveur LSP (stdio)
- `wasm-build.sh` / `wasm-build.ps1` — build cible `wasm32-unknown-unknown`
- `completions.sh` / `completions.ps1` — installe les complétions des binaires
- `release-tag.sh` / `release-tag.ps1` — crée/pousse un tag `vX.Y.Z`
- `build.sh` / `build.ps1` — build ciblé du workspace ou d'un crate, avec pré-analyse stricte (`scripts/check`) avant compilation (désactivable via `--skip-analysis` ou `VITTE_BUILD_SKIP_ANALYSIS=1`)
- `test.sh` / `test.ps1` — orchestrateur de tests (support nextest, option `--junit`)
- `lint.sh` / `lint.ps1` — fmt/clippy/deny avec options fines
- `pipeline.sh` / `pipeline.ps1` — pipeline couleur lint → tests → docs → build (`--pipeline-target` pour cross-build)
- `Windows/package.ps1` — archive zip multi-cible (x64/ARM64) pour distribution Windows
- `syntax.sh` / `syntax.ps1` — contrôle syntaxe des sources Vitte (`vitte fmt --check`) + `cargo check` colorisé
- `BSD/package.sh` — génère un paquet pkg(8) (`pkg create`) pour FreeBSD/DragonFly
- `scripts/pro/coverage.sh` — rapport de couverture (cargo tarpaulin)
- `scripts/pro/doclint.sh` — documentation complète avec diagnostics colorés
- `scripts/pro/deps-audit.sh` — cargo deny + cargo outdated (audit dépendances)
- `scripts/pro/arch-lint.py` — vérifie les dépendances entre couches (frontend/IR/backend/runtime/outils)
- `scripts/pro/vitte-audit.sh` — audit du fichier `vitte.toml`
- `scripts/pro/ci-report.sh` — agrège lint, tests et arch-lint avec journalisation
- `scripts/pro/graph-deps.sh` — graphe de dépendances (DOT si `jq`/`dot` disponibles, sinon `cargo tree`)
- `scripts/pro/coverage.sh` — génère un rapport de couverture via cargo tarpaulin

## Options globales
- `VITTE_STRICT_WARNINGS=0` — désactive le mode warnings→erreurs (actif par défaut).
- `VITTE_BUILD_SKIP_ANALYSIS=1` — saute l'analyse stricte pré-build.

## Voir aussi
- `Makefile` à la racine pour des alias rapides (`make help`).
- `docs/development.md` pour un guide pas-à-pas de l’environnement.
