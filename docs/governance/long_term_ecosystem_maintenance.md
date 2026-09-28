# 213. Long-Term Ecosystem Maintenance

Objective: make Vitte maintainable over multiple years with explicit lifecycle rules.

Date: 2026-05-24
Scope: compiler, stdlib, backends, package manager, IDE tooling, docs contracts.

## 1) Deprecation Workflow

Lifecycle states:

1. `active`: fully supported, default path.
2. `soft-deprecated`: warning only, migration path available.
3. `hard-deprecated`: warning by default, error in strict mode.
4. `removed`: no longer available.

Minimum timeline:

- First deprecation notice in release `N`.
- Removal not before `N+2` minor releases, or 6 months minimum, whichever is longer.
- LTS-covered surfaces remain available until end of their support window.

Required artifacts per deprecation:

- changelog entry (`CHANGELOG.md`),
- migration note with before/after examples,
- diagnostics code and stable message,
- tracking issue labeled `deprecation`.

## 2) Compatibility Policy

Compatibility classes:

- `C0 Internal`: no stability promise, may change anytime.
- `C1 Experimental`: public but unstable; requires explicit opt-in.
- `C2 Stable`: semver-governed compatibility across minor releases.
- `C3 Contract-critical`: bootstrap, diagnostics schema, deterministic build contracts; changes require RFC + staged rollout.

Rules:

- Stable CLI flags, diagnostic codes, and documented stdlib APIs are `C2` unless explicitly marked otherwise.
- Bootstrap contracts and reproducibility surfaces are `C3`.
- Breaking change to `C2/C3` must include migration path and compatibility note.

## 3) Migration Policy

Migration levels:

1. `M1 docs-only`: no code rewrite needed.
2. `M2 assisted`: mechanical rewrite script available.
3. `M3 manual`: semantic change requiring manual edits.

Requirements:

- each breaking or hard-deprecation change must declare `M1/M2/M3`,
- `M2` must ship with script/tool and fixture coverage,
- release notes must list all active migrations and their deadline.

## 4) Release Cadence

Target cadence:

- patch releases: as needed for regressions/security.
- minor releases: every 8 weeks.
- major releases: only for explicit platform boundary changes (expected low frequency).

Freeze model per minor release:

1. week 1-5: feature and refactor window,
2. week 6: feature freeze, only fixes/docs/tests,
3. week 7: release candidate and compatibility validation,
4. week 8: release + migration communication.

## 5) Support Windows

Support tiers:

- `Current`: latest minor release, full support.
- `Previous`: previous minor release, bug/security fixes only.
- `LTS`: optional designated line, critical/security only until published end date.

Default window:

- Current + Previous are supported.
- An LTS line, when designated, is supported for 12 months.

## 6) Issue Triage Process

Severity:

- `S0`: crash, corruption, security, deterministic build break.
- `S1`: major regression, incorrect diagnostics with unsafe guidance, backend miscompile risk.
- `S2`: non-critical correctness/perf/docs/tooling regression.
- `S3`: enhancement or low-risk maintenance.

SLO targets:

- `S0`: acknowledge within 24h, mitigation plan within 72h.
- `S1`: acknowledge within 3 days, decision within 7 days.
- `S2/S3`: triaged in weekly batch.

Triage loop:

1. reproduce (or request reproducer),
2. classify severity + compatibility class impact,
3. assign owner + target milestone,
4. link tests/docs/contracts affected,
5. close only with evidence (test, snapshot, or gate report).

## 7) Operational Gates

Maintenance policy is considered enforced when:

- deprecations include migration artifacts and deadlines,
- compatibility class is declared for new public surfaces,
- issue labels and severity are present on all open defects,
- release notes include migration + support window section,
- deterministic/reproducibility checks remain green in release gates.
