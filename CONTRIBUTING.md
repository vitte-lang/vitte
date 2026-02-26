# Contributing to Vitte

Thanks for contributing.
This guide is the shortest safe path from first change to merged PR.

## 1) Before You Start

You need:

- C/C++ toolchain (`clang`/`clang++` or compatible)
- `make`
- `openssl` and `curl` development libraries

If you are unsure your environment is ready, run:

```sh
make build
```

## 2) Local Dev Loop

Recommended baseline checks:

```sh
make parse
make hir-validate
make negative-tests
make test
```

If you touched modules/packages policy or lint behavior, also run:

```sh
make modules-tests
make modules-snapshots
make modules-contract-snapshots
```

## 3) Coding Style

- C/C++ formatting: follow `.clang-format`
- Vitte files: keep style consistent with existing package/module patterns
- Prefer small, focused changes over broad refactors

## 4) PR Workflow

1. Create a branch (`feature/...` or `fix/...`).
2. Keep commits scoped and readable.
3. Add tests for behavior changes.
4. Update docs for user-visible changes.
5. Open PR with clear motivation + impact.

A good PR description includes:

- problem statement
- what changed
- why this approach
- tests run locally
- known limitations

## 5) Commit Quality

Good commit messages are short and precise:

```text
area: what changed
```

Examples:

- `frontend: tighten module loader validation`
- `docs: simplify beginner onboarding in README`
- `modules: add package governance lint checks`

## 6) Tests and Snapshots

When snapshots fail:

- investigate first
- update snapshots only when behavior change is intentional

Useful commands:

```sh
make diag-snapshots
make modules-snapshots
make modules-snapshots-update
```

## 7) Modules / Packages Rules (Important)

Current direction in this repo:

- `mod.vit` is the canonical package entrypoint
- `info.vit` metadata is required in governed areas
- `OWNERS` is required where policy enforces ownership
- legacy paths/imports are being phased out progressively

If your PR touches these rules, include migration notes in the PR.

## 8) Where to Ask Questions

- General help: `SUPPORT.md`
- Security issues: `SECURITY.md`

If unsure, open an issue with a minimal reproducible example.

## 9) Good First Contributions

- fix diagnostics clarity
- improve docs/examples
- add focused regression tests
- improve tooling scripts and CI ergonomics

Thanks again for helping Vitte improve.
