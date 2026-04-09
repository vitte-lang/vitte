# VitteOS Roadmap

This roadmap is the practical sequence to make VitteOS usable in small, safe steps.
Each milestone is intentionally narrow: finish one foundation before opening the next.

Source of truth:

- `vitteos/issues.yaml` (validated by `make vitteos-issues-check`)

## How to read this roadmap

- `Goal`: what must become reliable for users/contributors.
- `Definition of done`: objective exit criteria.
- `Tracking`: issue label to use in the tracker.
- `Status`: current delivery state.

## Milestone 1: Boot Stable (M1)

Goal:

- make boot deterministic and boring across supported targets.

Definition of done:

- boot flow contracts are frozen at `boot` <-> `arch` boundary.
- minimal path always works: boot -> serial log -> halt.
- CI runs repeatable smoke checks with stable snapshots.
- failures are actionable (clear diagnostics, no flaky wording).

Tracking:

- issue label: `milestone:M1`.

Status:

- `planned`.

## Milestone 2: Minimal Memory (M2)

Goal:

- provide a small memory layer that is correct first, fast second.

Definition of done:

- early allocator and paging primitives in `mm` are stable.
- API surface is intentionally small: alloc, map, unmap, fail path.
- edge cases are covered by deterministic tests (no timing dependency).
- memory invariants are documented and checked in CI.

Tracking:

- issue label: `milestone:M2`.

Status:

- `planned`.

## Milestone 3: Basic Scheduler (M3)

Goal:

- run multiple tasks safely with predictable scheduling behavior.

Definition of done:

- runnable task model and context-switch contract in `sched` are stable.
- baseline fairness is measured and guarded by tests.
- wake/sleep semantics are tested for common and failure paths.
- minimal scheduler/syscall interaction passes in CI.

Tracking:

- issue label: `milestone:M3`.

Status:

- `planned`.

## Delivery order and guardrails

- do not start M2 work that depends on unstable M1 contracts.
- do not start M3 API expansion before M2 memory invariants are locked.
- prefer small PRs with one domain each (`boot`, `mm`, `sched`).
- every merged PR must keep snapshots and targeted checks green.

## Short operating policy

- if a change breaks determinism, fix determinism first.
- if a contract changes, update docs + snapshots in the same PR.
- if a milestone scope grows, split it; do not silently widen goals.
