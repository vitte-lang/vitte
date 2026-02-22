# Vitte Toolchain Scripts

This directory contains **all auxiliary scripts** used to build, test, package, and run the Vitte toolchain.  
They are designed to be **deterministic**, **CI-friendly**, and **composable**, with minimal assumptions about the host system.

All scripts follow the same principles:
- strict shell mode (`set -euo pipefail`)
- explicit configuration via environment variables
- no hidden side effects outside `target/`, `.cache/`, or `.store/`
- reusable across local development and CI

---

# Vitte Toolchain Scripts

This directory contains **all auxiliary scripts** used to build, test, package, and run the Vitte toolchain.  
They are designed to be **deterministic**, **CI-friendly**, and **composable**, with minimal assumptions about the host system.

All scripts follow the same principles:
- strict shell mode (`set -euo pipefail`)
- explicit configuration via environment variables
- no hidden side effects outside `target/`, `.cache/`, or `.store/`
- reusable across local development and CI

---

---

## CI Scripts (`ci/`)

Scripts used by continuous integration pipelines.

- `lint.sh`  
  Runs static checks (shell, formatting, invariants).

- `format.sh`  
  Applies formatting rules where applicable.

- `artifacts.sh`  
  Collects and prepares build artifacts.

- `github-actions.sh`  
  Entry-point wrapper for GitHub Actions.

All CI scripts are **non-interactive** and safe to run locally.

---

## Developer Scripts (`dev/`)

Utilities for local development and debugging.

- `debug-env.sh`  
  Prints or configures a debug-friendly environment.

- `repl.sh`  
  Launches a Vitte REPL (when available).

- `run-vittec.sh`  
  Direct invocation of the Vitte compiler.

- `run-linker.sh`  
  Manual linker invocation for debugging.

---

## Installation Scripts (`install/`)

Scripts to install or uninstall the Vitte toolchain.

- `install-local.sh`  
  Installs into a user prefix (`$HOME/.local` by default).  
  Installs `vitte` (required) and legacy binaries when available, plus editor files under `share/vitte/editors/`.

- `install-prefix.sh`  
  Installs into a given prefix (e.g. `/usr/local`).  
  Used by macOS/pkg-style installs. Installs `vitte` (required), optional legacy binaries, headers/libs/share, editor files, man pages, shell completions, and `share/vitte/env.sh`.

- `install-debian-vitte-2.1.1.sh`  
  Debian/Ubuntu one-shot installer profile for `vitte 2.1.1`: installs apt dependencies, builds, and installs via prefix/local mode.

- `permissions.sh`  
  Fixes executable permissions where required.

- `uninstall.sh`  
  Removes installed files.

---

## Packaging Scripts (`package/`)

Used to produce distributable artifacts.

- `make-archive.sh`  
  Generic archive creator (tar/zip).

- `make-tarball.sh`  
  Deterministic tarball builder.

- `bundle-runtime.sh`  
  Packages the Vitte runtime.

- `bundle-stdlib.sh`  
  Packages the Vitte standard library.

- `make-debian-deb.sh`  
  Builds a complete Debian `.deb` (binary + packages + runtime + editors + man + completions + env helper + postinst checks).

- `make-macos-pkg.sh`  
  Builds a unified macOS `.pkg` installer (binary + packages + runtime + editors + man + completions + env helper + postinstall checks).

- `make-macos-uninstall-pkg.sh`  
  Builds a macOS `.pkg` uninstaller (`--nopayload`) with optional user editor cleanup (`REMOVE_USER_EDITOR_CONFIG=1`).

- `checksum.sh`  
  Generates and verifies checksums (sha256 / sha512).

All outputs are written under `target/packages/`.

Completion generation and checks are centralized in:
- `tools/generate_completions.py` (single source -> bash/zsh/fish)
- `tools/completions/spec.json` (commands/options/value domains)
- `tools/completions_snapshots.sh` (`--check` + snapshot assertions)

---

## Steel Scripts (`steel/`)

Wrappers around the **Steel** build system.

- `steel-env.sh`  
  Initializes the Steel + Vitte environment.

- `steel-build.sh`  
  Runs a build bake.

- `steel-run.sh`  
  Runs an executable bake.

- `steel-test.sh`  
  Runs test bakes.

These scripts load optional **target definitions** and respect
`vitte_target_pre_build` / `vitte_target_post_build` hooks.

---

## Target Definitions (`targets/`)

Platform and architecture configuration scripts.

Available targets include:
- `linux-x86_64.sh`
- `linux-aarch64.sh`
- `macos-x86_64.sh`
- `macos-arm64.sh`
- `freebsd-x86_64.sh`
- `windows-x86_64.ps1`

Each target defines:
- target triple
- compiler and linker defaults
- sysroot / SDK handling
- ABI and architecture metadata

Targets are loaded via:
```bash
source toolchain/scripts/targets/linux-x86_64.sh 
