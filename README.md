# Vitte

Vitte is a compiler project written in Vitte.

This repository is now Vitte-only on the source side. The main compiler surface lives in `src/vitte/compiler`, and the bootstrap driver is kept small and easy to follow.

## What this repo is for

- Build and test the Vitte compiler.
- Keep the compiler bootstrap stable.
- Document the language, the compiler, and the repo layout.
- Track the move away from old host-language sources.

## Simple picture

The compiler flow is:

1. Read Vitte source.
2. Parse it into a simple module view.
3. Lower it through IR.
4. Send it to the backend.
5. Emit output and notes.

The current code keeps that flow easy to inspect and safe to bootstrap.

## Main source areas

- `src/vitte/compiler` - compiler front, IR, backend, and driver facades.
- `src/vitte/packages/compiler/driver` - package-level compiler driver support.
- `src/vitte/stdlib` - standard library modules in Vitte.
- `docs` - site pages, book pages, news, and reference notes.
- `man` - manual pages.
- `tools` - repo checks, helpers, audits, and CI scripts.

## Compiler entry points

The most useful files to start with are:

- `src/vitte/compiler/mod.vit`
- `src/vitte/compiler/driver/compiler.vit`
- `src/vitte/compiler/driver/pipeline.vit`
- `src/vitte/compiler/frontend/parser.vit`
- `src/vitte/compiler/frontend/lexer.vit`
- `src/vitte/compiler/ir/pipeline.vit`
- `src/vitte/compiler/backends/backend.vit`

## What the compiler currently does

- It keeps a bootstrap-friendly compiler surface.
- It parses simple top-level forms like `space`, `use`, `const`, `global`, and `proc`.
- It lowers a small IR path from AST to HIR to MIR.
- It has a backend facade that produces simple summaries and output notes.
- It has smoke tests that check the main flow.

## Quick start

Build and check the project with the repo tools:

```bash
make build
make ci-fast
make ci-strict
```

Run the self-host audit:

```bash
tools/selfhost_audit.sh
```

Run compiler checks directly on a file:

```bash
bin/vitte check src/vitte/compiler/driver/compiler.vit
bin/vitte check src/vitte/compiler/ir/pipeline.vit
bin/vitte check src/vitte/compiler/backends/backend.vit
```

Run the compiler smoke tests:

```bash
bin/vitte check src/vitte/compiler/tests/smoke.vit
```

## Validation targets

These are the most common project checks:

- `make build`
- `make ci-fast`
- `make ci-strict`
- `make docs-paths-check`
- `tools/selfhost_audit.sh`

## Bootstrap local (stage0 -> stage2)

Current local bootstrap flow:

1. Install the trusted stage0 seed (`vittec0`).
2. Rebuild stage1 from `toolchain/stage1/src/main.vit` (`vittec1`).
3. Run stage2 bootstrap from stage1 (`vittec`).
4. Install stage2 as `bin/vitte`.

Useful commands:

```bash
make bootstrap-all
make bootstrap-verify
```

Bootstrap checks and reports:

```bash
make seed-gate
make seed-syntax-test
make seed-compat-report
```

Generated compatibility report:

- `target/reports/seed_compat_report.txt`

## Recovery

If local bootstrap artifacts get out of sync:

```bash
make bootstrap-seed
toolchain/scripts/bootstrap/stage1.sh
VITTE_SELF_CHECK=0 toolchain/scripts/bootstrap/stage2.sh
cp bin/vittec bin/vitte
chmod +x bin/vitte
make build
```

If only stage0 needs refresh:

```bash
scripts/seed/install_seed.sh
```

## Troubleshooting

- `vitte-bootstrap-check` fails:
  - ensure `bin/vitte` exists and is executable,
  - rerun `make bootstrap-all`.
- `seed-gate` fails:
  - inspect first failing file,
  - run `bin/vittec0 check <file>` directly,
  - check `target/reports/seed_compat_report.txt`.
- bootstrap scripts fail on missing stage binaries:
  - rerun stage chain in order (`seed`, `stage1`, `stage2`).
- syntax regression suspected:
  - run `make seed-syntax-test`.

## Audit policy

`vitte-source-audit` enforces Vitte-only sources in the workspace. There is no repo-hosted host-language source exception; stage0 recovery uses the checked seed artifact under `toolchain/seed`.

## Seed Trust Chain

The bootstrap seed source is `toolchain/seed/src/main.vit`. The generated seed artifact is tracked at `toolchain/seed/vittec0.seed` and pinned by `toolchain/seed/manifest.txt`.

- version: `vittec0 stage0-vitte-seed 0.1.0`
- verification: `scripts/seed/verify_seed.sh`
- install path: `bin/vittec0`
- contract: `docs/bootstrap_seed.md`
- native IR: `docs/bootstrap_native_ir.md`
- contracts index: `docs/bootstrap_contracts.md`
- full local contract: `make bootstrap-native-contract`
- fast local contract: `make bootstrap-native-fast-contract`
- fixture matrix: `tools/bootstrap_native_fixture_matrix.sh`

The manifest ties the readable source and generated artifact together. Rotating the seed requires updating the source, the seed artifact, its SHA-256, and the expected version together.

## Documentation

Useful docs pages:

- `docs/index.html`
- `docs/news.html`
- `docs/doc.html`
- `docs/source.html`
- `docs/download.html`
- `docs/diagnostics.html`
- `docs/community.html`
- `docs/suggestions.html`

The manual pages are also a good starting point:

- `man/vitte.1`
- `man/vittec.1`

## Repository layout

```text
src/vitte/compiler        compiler source and bootstrap facades
src/vitte/packages        package-level compiler support
src/vitte/stdlib          standard library in Vitte
docs                      site, book, news, and reference docs
man                       manual pages
tools                     checks, generators, audits, and CI helpers
tests                     smoke tests and diagnostics snapshots
completions               shell completion files
```

## Notes for contributors

- Keep new compiler work bootstrap-friendly.
- Prefer Vitte files over legacy host-language files.
- Keep smoke tests small and easy to read.
- When adding links in docs, prefer local paths that the docs checks can validate.

## Status

The workspace is in the middle of a migration toward a fully Vitte-based compiler and support tree.

The current goal is simple:

- keep bootstrap checks green,
- keep the compiler surface understandable,
- and expand the compiler step by step without bringing back legacy host-language sources.
