# Make Targets

This page indexes the most useful `make` entrypoints.

For the full list, run:

```sh
make help
```

## Beginner / Local Loop

- `make build`
- `make quickstart-check`
- `make doctor`
- `make test`
- `make parse`
- `make hir-validate`

## Core Language

- `make grammar-check`
- `make grammar-gate`
- `make core-language-gate`
- `make core-release-gate`

## Modules / Packages

- `make modules-tests`
- `make modules-snapshots`
- `make modules-contract-snapshots`
- `make packages-gate`

## CI / Reports

- `make ci-fast`
- `make ci-strict`
- `make ci-completions`
- `make dx-adoption`
- `make public-benchmark-dashboard`
- `make release-proof-notes`
- `make all-tests`
- `make reports-index`
- `make release-doctor`

## VitteOS

- `make vitteos-doctor`
- `make vitteos-quick`
- `make vitteos-ci`
- `make vitteos-ci-strict`

## Packaging

- `make pkg-debian`
- `make pkg-debian-audit`
- `make pkg-macos`
- `make release-check`
