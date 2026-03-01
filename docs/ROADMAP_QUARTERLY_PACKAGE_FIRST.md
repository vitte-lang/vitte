# VITTE Quarterly Roadmap (Package-First)

## Q1 (runtime critique)
- `http` + `http_client`: hardening SSRF/TLS/CRLF, deterministic routing, retries/circuit/pool.
- `process`: strict policy gates, allowlist enforcement, profile matrix core/desktop/system.
- Shared: release doctor, security hardening score gate, semantic golden cross-package.

## Q2 (state & storage)
- `db`: tx invariants, retry/timeout contracts, migration compatibility gate.
- `fs`: path policy, workspace allowlist, atomic writes/watcher behavior.
- `log`: redaction/sampling/rate-limit/dedup with stable diagnostics.

## Q3 (surface & tooling)
- `std` + `json` + `yaml`: facade-thin zero warnings, deterministic format/parse/fuzz coverage.
- `test` + `lint`: doctor/quickfix stability, ownership/false-positive policy hardening.

## Cross-cutting deliverables per quarter
- Contracts snapshots/hash + API diff explainer.
- Perf baseline by machine profile (`dev`/`ci`) + regression gating.
- Security baseline + hardening score >= threshold.
- Plugin ABI + binary smoke + manifest policy validation.
