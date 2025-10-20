# Guide de développement Vitte

Ce document détaille la préparation de l’environnement et les commandes de
développement quotidiennes. L’objectif est d’offrir une expérience de
développement au niveau des toolchains professionnelles (Rust, C/C++, C#).

## Prérequis

- `rustup` (https://rustup.rs/)
- Toolchain `stable` avec les composants listés dans `rust-toolchain.toml`
  (`rustfmt`, `clippy`, `rust-src`, `llvm-tools-preview`, `rust-analyzer`).
- Pour les cibles web et outils front : `node`, `npm`, `wasm-pack`.
- Outils CLI recommandés : `cargo-deny`, `cargo-nextest`, `just`.

## Bootstrap automatique

```bash
# Vérifie l’environnement
make bootstrap

# Installe automatiquement les composants/targets manquants
INSTALL=1 make bootstrap

# Ignore les vérifications Node/WASM (ex: CI minimaliste)
SKIP_NODE=1 SKIP_WASM=1 make bootstrap

# Note: en mode INSTALL=1, le bootstrap installe aussi la toolchain `nightly`
# (avec rustfmt) afin d'activer les options avancées du `rustfmt.toml`.
```

Le script `scripts/bootstrap.sh` s’appuie sur `rust-toolchain.toml` pour
vérifier les composants/targets attendus. Utilisez `--skip-node` ou
`--skip-wasm` (ou les variables `SKIP_NODE=1`, `SKIP_WASM=1`) si vous n’avez
pas besoin des outils web.

## Commandes Makefile

Le `Makefile` à la racine offre des aliases cohérents pour les tâches courantes
(toutes strictes sur les warnings) :

```bash
make build           # build strict (fmt/clippy/test/doc exécutés via scripts)
make build-release   # build --release, warnings → erreurs
make build-analysis  # n’exécute que l’analyse pré-build
make check           # fmt + clippy + tests + cargo-deny + docs
make lint            # fmt + clippy
make fmt             # cargo fmt --all
make fmt-check       # cargo fmt --all -- --check
make test            # scripts/test.sh --workspace
make doc             # cargo doc --workspace --all-features --no-deps
make pipeline        # pipeline complet (lint → test → doc → build)
make bench           # scripts/bench.sh
make wasm            # scripts/wasm-build.sh
make clean           # cargo clean
make arch            # scripts/pro/arch-lint.py (enforce architecture layers)
make coverage        # scripts/pro/coverage.sh (cargo tarpaulin)
make doctor          # scripts/doctor.sh (vérifie toolchains/outils)
```

Chaque cible accepte des options supplémentaires via des variables, par exemple :

```bash
make build BUILD_OPTS="--skip-analysis --release"
make test TEST_OPTS="--use-nextest --ignored"
make pipeline PIPELINE_OPTS="--skip-docs"
```

`make help` affiche la liste complète.

## Mode strict et variables d’environnement

La configuration `.cargo/config.toml` et les scripts forcent `-Dwarnings` pour
les builds et `RUSTDOCFLAGS`. Les variables suivantes permettent d’assouplir
temporairement le comportement :

- `VITTE_STRICT_WARNINGS=0` : autorise les warnings (déconseillé, par défaut strict).
- `VITTE_BUILD_SKIP_ANALYSIS=1` : saute l’analyse pré-build dans les scripts.
- `VITTE_STRICT_WARNINGS=0 make build` : exemple d’override ponctuel.

## Outils additionnels

- `cargo-deny` : audit des dépendances (`cargo install cargo-deny --locked`).
- `cargo-nextest` : runner de tests parallèle (`cargo install cargo-nextest --locked`).
- `wasm-pack` : build WebAssembly (`cargo install wasm-pack`).
- `just` : alias de commandes supplémentaires (`cargo install just --locked`).
- `scripts/pro/ci-report.sh` : exécute lint, tests et arch-lint avec logs agrégés (CI_REPORT_KEEP=1 pour conserver les journaux).

Gardez la toolchain et ces outils synchronisés via `make bootstrap`.
