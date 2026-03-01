# Core Platform Contract

## Scope
This contract applies to platform packages governed by `tools/facade_packages.json`:
`core`, `std`, `log`, `fs`, `db`, `http`, `http_client`, `process`, `json`, `yaml`, `test`, `lint`.

## Non-Negotiable Invariants
1. Facade-thin:
- `src/vitte/packages/<pkg>/mod.vit` is a public facade, not an implementation container.
- Heavy logic must live in `internal/*`.
- Gate is package-specific via `facade_thin` in `tools/facade_packages.json`.

2. Additive-only public evolution:
- Stable facade APIs can add symbols but cannot remove/rename behavior without a major lifecycle transition.
- Enforced by contract snapshots (`*.exports`, `*.facade.api`, `*.sha256`, lockfiles, contract diff).

3. Diagnostic namespace ownership:
- Each package owns one diagnostic namespace prefix.
- No cross-prefix leakage inside package sources.
- Enforced by `tools/lint_diagnostic_namespace_ownership.py`.

4. No side effects at import time:
- No `entry` and no runtime actions on import in public facades.
- Enforced by package and global no-side-effects lint targets.

## ROLE-CONTRACT Requirements (mod.vit)
Every governed package `mod.vit` must include `ROLE-CONTRACT` with at least:
- `package`
- `role`
- `input_contract`
- `output_contract`
- `boundary`
- `versioning`
- `api_surface_stable`
- `diagnostics`
- `compat_policy` (`additive-only`)
- `internal_exports` (`forbidden`)
- `api_version`

Validation: `tools/lint_facade_role_contracts.py`.

## Maturity Index
Each package declares a maturity in `tools/facade_packages.json`:
- `experimental`
- `stable`
- `hardened`

This maturity is reported in `target/reports/contracts_dashboard.md`.

## CI Enforcement
Primary gates:
- `make packages-only-ci`
- `make packages-strict-ci`
- `make release-readiness`

Incremental gate:
- `make packages-changed-ci`

## Change Policy
Any change that affects these invariants must update:
1. Relevant code and lints.
2. Snapshots/baselines if expected.
3. This contract document if policy semantics changed.
