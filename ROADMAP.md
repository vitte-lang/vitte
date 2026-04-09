# Vitte Roadmap

This file is the shortest public view of where the project is going.
It complements the detailed roadmaps in:

- `docs/ROADMAP_QUARTERLY_PACKAGE_FIRST.md`
- `docs/vitteos/ROADMAP.md`

The goal here is simple: show what matters now, what is next, and what is later.

## Now

These are the current priorities that should stay visible to contributors and users.

| Area | Status | Current objective | Exit signal |
| --- | --- | --- | --- |
| Language core | active | keep the protected core small, documented, and regression-gated | `make core-language-gate` and `make core-release-gate` remain aligned with docs |
| Package governance | active | harden `mod.vit`, `info.vit`, `OWNERS`, and package contract checks | governed packages validate cleanly and snapshots stay stable |
| Modules/packages CI | active | keep package snapshots, reports, and ownership policy deterministic | modules suites stay green without snapshot churn |
| Beginner onboarding | active | reduce time from clone to first successful `check` and `build` | a new contributor can follow `README.md` and `docs/GETTING_STARTED.md` without guesswork |
| Completions/tooling quality | active | keep shell completions and tooling outputs reproducible | `make ci-completions` and related snapshots remain stable |
| VitteOS foundations | planned | keep OS work staged behind narrow milestones | VitteOS work follows milestone contracts in `docs/vitteos/ROADMAP.md` |

## Next

These are the next improvements that should become more visible once the current core work is boring.

| Area | Next objective |
| --- | --- |
| Performance | publish a small baseline for compile time, binary size, and selected package benches |
| Docs architecture | document the compiler pipeline and contribution entry points more explicitly |
| Package maturity | make package maturity more obvious across `stable`, `experimental`, and `internal` surfaces |
| Release hygiene | define a lightweight snapshot/release candidate flow without over-claiming stability |
| Health reporting | surface CI, contract, and package health in one human-readable dashboard |

## Later

These are important, but should not outrank the current stabilization work.

| Area | Later objective |
| --- | --- |
| Broader stable stdlib surface | promote more packages only after contracts and diagnostics are boring |
| Stronger performance gates | move from ad hoc benches to tracked regression thresholds |
| Wider platform story | expand platform support once core build, diagnostics, and package policy are stable |
| Richer IDE/editor flows | deepen official editing workflows after core compiler/docs priorities are in place |
| Broader release process | define versioning and support policy once the public contract is clearer |

## Detailed Sources

- `docs/ROADMAP_QUARTERLY_PACKAGE_FIRST.md`
- `docs/vitteos/ROADMAP.md`
- `docs/LANGUAGE_CORE.md`
- `docs/LANGUAGE_CORE_GUARANTEES.md`
