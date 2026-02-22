# VitteOS Roadmap

Mapping source:

- `vitteos/issues.yaml` (validated by `make vitteos-issues-check`)

## Milestone 1: Boot Stable

- Issue tracker: `M1` (create and tag issues with label `milestone:M1`).
- Status: `planned`.
- Lock boot flow contracts (`boot` + `arch` boundary).
- Keep a minimal smoke path: boot -> serial log -> halt.
- Ensure repeatable smoke validation in CI.

## Milestone 2: Minimal Memory

- Issue tracker: `M2` (create and tag issues with label `milestone:M2`).
- Status: `planned`.
- Stabilize early allocator and paging primitives in `mm`.
- Define small API for allocation, mapping, and failure paths.
- Add deterministic tests for core memory transitions.

## Milestone 3: Basic Scheduler

- Issue tracker: `M3` (create and tag issues with label `milestone:M3`).
- Status: `planned`.
- Stabilize runnable task model and context-switch contracts in `sched`.
- Add baseline fairness and wake/sleep behavior tests.
- Validate scheduler + syscall interactions at minimal scale.
