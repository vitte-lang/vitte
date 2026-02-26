# Vitte

Vitte is an experimental systems language and toolchain.

The project is built around one idea: low-level code should stay readable.
No hidden magic, no overly clever syntax, and a compiler pipeline you can inspect.

If you are a beginner: this README is enough to get you from zero to first contribution.

## In One Minute

Vitte is for people building:

- systems software
- runtimes and tooling
- embedded experiments (Arduino/STM32/ESP32)
- OS-level projects (see VitteOS)

Vitte is not stable yet. Breaking changes are expected while the language evolves.

## Beginner Path (Install -> Test -> Contribute)

If you only follow one section, follow this one.

### Step 1: Install

Use Homebrew (fastest path):

```sh
brew install vitte
```

Or build from source:

```sh
make build
```

Compiler binary:

```text
bin/vitte
```

### Step 2: Test that it works

Run these commands:

```sh
vitte check examples/syntax_features.vit
vitte build examples/syntax_features.vit
```

If both commands pass, your setup is good.

### Step 3: Contribute safely

Before opening a PR:

- run `make build`
- run relevant tests (`make test`, `make parse`, `make hir-validate`)
- add/update tests for behavior changes
- update docs when behavior changes

---

## Quick Start

Documentation: https://vitte.netlify.app/

### Try a file

```sh
vitte check examples/syntax_features.vit
vitte build examples/syntax_features.vit
```

## Install Details

### macOS

Requirements: `clang`, `openssl`, `curl`.

```sh
brew install llvm openssl@3 curl
OPENSSL_DIR=/opt/homebrew/opt/openssl@3 make build
```

Install binary + editor syntax files:

```sh
OPENSSL_DIR=/opt/homebrew/opt/openssl@3 make install
```

### Linux (Ubuntu/Debian)

```sh
sudo apt-get update
sudo apt-get install -y clang libssl-dev libcurl4-openssl-dev
make build
```

Optional packaging commands:

```sh
make install-debian-2.1.1
make pkg-debian
make pkg-debian-install
```

### Windows

Windows support is experimental.
Recommended path: WSL2 (Ubuntu), then follow Linux steps.

## Core Commands

```sh
vitte parse path/to/file.vit
vitte check path/to/file.vit
vitte build path/to/file.vit
```

- `parse`: parser-level validation
- `check`: semantic validation without full build
- `build`: compile output

## Minimal Language Glimpse

```vit
proc add(a: int, b: int) -> int {
  give a + b
}

entry main at core/app {
  let result: int = add(2, 3)
  give result
}
```

- `proc` defines a function
- `give` returns a value
- `entry` is the program entrypoint

## Design Principles

- Explicitness over inference
- Readable structure over compact tricks
- Tooling-friendly syntax
- Deterministic outputs where possible
- Low-level control when needed (`unsafe`, `asm`)

## Compiler Pipeline

1. Frontend: parse + structure checks
2. HIR: normalize high-level constructs
3. MIR: normalize control flow and operations
4. Backend: emit C++ (then native toolchain)

## Reproducible Object Mode (macOS/clang)

For byte-level object comparison:

```sh
make repro
```

Manual form:

```sh
vitte build --repro --emit-obj -o build/repro/vitte.o tests/repro/min.vit
```

Useful flags:

- `--repro` enables deterministic-oriented behavior
- `--emit-obj` skips linking and emits object files
- `--repro-strict` is available for explicit control

## HTTP Runtime Dependency Mode

`libcurl` headers are optional at build time.

- without libcurl dev headers: build still succeeds
- runtime HTTP calls return explicit degraded-mode errors
- for full HTTP runtime support: install libcurl dev package and ensure `pkg-config libcurl` works

## VitteOS Scripts

VitteOS scripting is Vitte-first, with external wrappers under `vitteos/tooling/`.

Quick commands:

```sh
./vitteos/tooling/bootstrap.sh
python3 vitteos/tooling/check_vit.py
node vitteos/tooling/run_check.js
make vitteos-quick
make vitteos-doctor
make vitteos-status
```

## Completion Maintenance

When CLI completion behavior changes:

```sh
make completions-gen
make completions-snapshots
```

Useful extras:

```sh
make completions-snapshots-update
make ci-completions
```

## Contribution Checklist

Before opening a PR:

- [ ] `make build` passes
- [ ] relevant tests pass (`make test`, `make parse`, `make hir-validate`)
- [ ] behavior changes include tests
- [ ] docs are updated when needed

Tip for beginners: small PRs get reviewed faster than large rewrites.

## Project Layout

- `src/` compiler implementation
- `src/vitte/packages/` package modules
- `examples/` sample `.vit` programs
- `tests/` regression and language tests
- `tools/` scripts/utilities
- `target/` runtime/target assets

## Docs

- `docs/cli.md`
- `docs/errors.md`
- `docs/stdlib.md`
- `CONTRIBUTING.md`
- `SECURITY.md`
- `SUPPORT.md`
- `ROADMAP.md`
- `CHANGELOG.md`

## Status

Vitte is experimental. Stability is not guaranteed yet.

## License

See `LICENSE`.
