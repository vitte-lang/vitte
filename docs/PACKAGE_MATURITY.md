# Package Maturity

This page defines the maturity labels used by `docs/PACKAGE_INDEX.md`.

## Labels

- `documented-experimental`: visible public surface with documentation, but still expected to change until explicitly promoted.
- `internal-by-default`: repository-local package surface that should not be treated as a public compatibility promise.

## Rule

Packages are internal by default. A package becomes documented experimental only when it has an intentional docs or API index entry and is listed by the package index generator.
