# 214. Vitte Platform Governance Complete

Objective: stabilize technical governance so platform evolution remains predictable.

Date: 2026-05-24

## 1) Architecture Review Process

Architecture review is required for:

- new compiler stage or major pipeline change,
- new runtime contract,
- new backend or backend ABI shift,
- stateful package manager behavior changes,
- deterministic/reproducibility contract changes.

Review checklist:

1. problem statement and alternatives,
2. compatibility class impact (`C0..C3`),
3. observability/diagnostics impact,
4. rollback strategy,
5. tests and gate coverage plan.

Decision states: `accepted`, `accepted-with-conditions`, `rejected`, `superseded`.

## 2) RFC Process

RFC required for:

- `C2/C3` public surface changes,
- deprecation/removal of stable interfaces,
- language syntax/semantics changes,
- governance policy updates.

RFC lifecycle:

1. Draft (`rfc:draft`)
2. Review (`rfc:review`)
3. Decision (`rfc:accepted` or `rfc:rejected`)
4. Implementation tracking (`rfc:implemented`)
5. Post-release validation (`rfc:validated`)

Minimum RFC sections:

- motivation,
- design,
- compatibility and migration,
- diagnostics and tooling effects,
- test/reproducibility plan,
- rollout/rollback plan.

## 3) Diagnostics Policy

Diagnostics must be:

- deterministic in ordering and formatting under deterministic mode,
- tied to stable diagnostic codes for stable surfaces,
- actionable (cause + minimal fix path),
- covered by snapshot tests for regressions.

Policy constraints:

- new diagnostic code requires docs entry and fixture,
- changing stable diagnostic wording requires migration note if scripts rely on text,
- diagnostics JSON schema changes are `C3` and require staged rollout.

## 4) Stdlib Acceptance Policy

A stdlib module can be promoted from experimental to stable only if:

1. API docs exist and examples compile,
2. deterministic behavior requirements are explicit,
3. profile/runtime constraints are documented,
4. tests include nominal + edge + failure fixtures,
5. migration impact is evaluated.

Promotion labels:

- `stdlib:experimental`,
- `stdlib:candidate`,
- `stdlib:stable`.

## 5) Backend Acceptance Policy

A backend is accepted as platform-supported only if:

1. it passes deterministic build/rebuild checks on supported targets,
2. contract snapshots exist for ABI/IR boundaries,
3. failure modes produce structured diagnostics,
4. reproducibility and stress tests run in CI,
5. rollback path to previous backend remains available.

Backend maturity states:

- `backend:experimental`,
- `backend:supported`,
- `backend:contract-critical`.

## 6) Stability Guarantees

Guaranteed stable by default:

- documented CLI commands/options (except explicit experimental flags),
- diagnostic code identifiers,
- stable stdlib module APIs,
- reproducible build behavior covered by release gate.

Not guaranteed unless promoted:

- internal module layout,
- internal compiler IR details not documented as contract,
- experimental modules/backends/features.

## 7) Governance Cadence

- Weekly: issue triage and policy drift scan.
- Bi-weekly: architecture and RFC review batch.
- Per release: compatibility and deprecation audit.
- Quarterly: support window + maturity baseline review.
