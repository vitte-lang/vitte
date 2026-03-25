# Changelog

All notable changes to this project will be documented here.

## Unreleased

- Modules/packages governance hardening:
  - stricter module loader and lint pipeline integration
  - package ownership/metadata conventions expanded (`OWNERS`, `info.vit`, `mod.vit`)
  - new package scaffolds added (`module_index`, `contracts_registry`, `migration_playbook`, `benchkit`, `fuzzkit`, `docsgen_modules`, `catalog`, `data`, `kernel`)
- JWT package update:
  - interop-oriented token format (`base64url(header).base64url(payload).base64url(signature)`)
  - stronger validation paths and dedicated regression test (`tests/vitte_jwt_snapshots.vit`)
- Modules snapshots/reporting updates:
  - improved snapshot stability filtering
  - refreshed `packages-gate` snapshots
  - PR/weekly workflows for modules report and snapshots
- Tooling and build metadata:
  - `steelconf` aligned with current modules/packages gates and migration targets
  - `compile_commands.json` refreshed from current build
- Documentation refresh:
  - README simplified for beginner onboarding (`install -> test -> contribute`)
  - `CONTRIBUTING.md`, `SUPPORT.md`, `SECURITY.md`, and `CODE_OF_CONDUCT.md` clarified and expanded
- DX/adoption and publication proof layer:
  - `vitte init --template` supports `cli`, `service`, `lib-native`
  - new DX gate: `make dx-adoption` (completion latency, frequent diagnostics fix guidance, hello->prod KPI)
  - public benchmark methodology + dashboard (`make public-benchmark-dashboard`)
  - proof-oriented release notes generator with tag candidate (`make release-proof-notes`)
  - Steelconf aligned with Makefile fast/release/PGO + DX/report targets

---

## FAQ

**How are versions defined?**  
No stable release process yet; entries are grouped under Unreleased.
