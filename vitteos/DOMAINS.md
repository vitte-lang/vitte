# VitteOS Stable Domains

Canonical domains:

- `vitteos/boot`
- `vitteos/arch`
- `vitteos/mm`
- `vitteos/sched`
- `vitteos/fs`
- `vitteos/drivers`
- `vitteos/syscall`

Machine-readable manifest:

- `vitteos/domains.json`
- includes mandatory `owners` for every declared domain
- includes `non_domain_roots` allowlist for legacy non-domain roots

Each domain must contain:

- `mod.vit`
- `api.vit`
- `types.vit`
- `consts.vit`
- `impl.vit`
- `tests.vit`
- `doc.vit`

Layering policy:

- `mod.vit` is wiring/export only (no business logic declarations).
- Public API must be exposed from `api.vit` (not `impl.vit`).
- `boot` may depend on `arch/api` only (no direct `arch/*` internals).
- `mm` and `sched` must not depend directly on `drivers/*`; use abstractions.
- Public API signatures should prefer explicit domain types over naked primitives.
- `doc.vit` should include contract notes: role, allowed dependencies, invariants, error paths.

Quality checks:

- `make vitteos-issues-check`
- `make vitteos-domain-contract`
- `make vitteos-no-orphan-check`
- `make vitteos-space-naming-lint`
- `make vitteos-arch-contract-lint`
- `make vitteos-vit-header-lint`
- `make vitteos-vit-targeted-check`

Scaffolding:

- `make vitteos-new-module MODULE=vitteos/<path>`
- module registration index: `vitteos/modules.index`
