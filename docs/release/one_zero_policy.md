# Vitte 1.0 Release Policy

Vitte must not be declared `1.0` until every target platform gate is green.

Required green gates:

- self-hosting: `make selfhost-hard-strict`
- diagnostics: `make diagnostic-quality`
- stdlib: `make stdlib-gate`
- backend: `make backend-gate`
- packaging: `scripts_build/build-all-installers.sh all`
- CI platforms: `.github/workflows/ci.yml` and `.github/workflows/bootstrap.yml`

Versioning:

- language versions use semantic versioning through `language_semver`
- stdlib versions use semantic versioning through `stdlib_semver`
- compatibility policy is defined by `docs/release/compatibility_matrix.md`
- release evidence must include release notes, checksums, and reproducible artifacts

The release manager must refuse a 1.0 tag when any required gate is missing, skipped, or red on a target platform.
