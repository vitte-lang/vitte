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
